/* =====================================================================================
 * Macro daemon.
 *
 * Copyright (C) 2018 Jonas Møller (no) <jonasmo441@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS     OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */
#include <thread>
#include <iostream>

#define DEBUG_LOG_KEYS 0

extern "C" {
    #include <gtk/gtk.h>
    #include <glib.h>
    #include <libnotify/notify.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <syslog.h>
}

#include "Daemon.hpp"
#include "MacroDaemon.hpp"
#include "LuaUtils.hpp"
#include "utils.hpp"
#include "Permissions.hpp"

using namespace Lua;
using namespace Permissions;
using namespace std;

constexpr bool ALWAYS_REPEAT_KEYS = false;

static const char *const evval[] = {
    "UP",
    "DOWN",
    "REPEAT"
};
static const char *event_str[EV_CNT];

inline bool goodLuaFilename(const string& name) {
    return !(
        name.size() < 4 || name[0] == '.' || name.find(".lua") != name.size()-4
    );
}

static inline void initEventStrs()
{
    event_str[EV_SYN      ] = "SYN"       ;
    event_str[EV_KEY      ] = "KEY"       ;
    event_str[EV_REL      ] = "REL"       ;
    event_str[EV_ABS      ] = "ABS"       ;
    event_str[EV_MSC      ] = "MSC"       ;
    event_str[EV_SW       ] = "SW"        ;
    event_str[EV_LED      ] = "LED"       ;
    event_str[EV_SND      ] = "SND"       ;
    event_str[EV_REP      ] = "REP"       ;
    event_str[EV_FF       ] = "FF"        ;
    event_str[EV_PWR      ] = "PWR"       ;
    event_str[EV_FF_STATUS] = "FF_STATUS" ;
    event_str[EV_MAX      ] = "MAX"       ;
}

MacroDaemon::MacroDaemon()
    : kbd_srv("/var/lib/hawck-input/kbd.sock")
{
    auto [grp, grpbuf] = getgroup("hawck-input-share");
    chown("/var/lib/hawck-input/kbd.sock", getuid(), grp->gr_gid);
    chmod("/var/lib/hawck-input/kbd.sock", 0660);
    initEventStrs();
    notify_init("Hawck");
    string HOME(getenv("HOME"));
    home_dir = HOME + "/.local/share/hawck";
    
    // Keep looping around until we get a connection.
    for (;;) {
        try {
            int fd = kbd_srv.accept();
            kbd_com = new UNIXSocket<KBDAction>(fd);
            break;
        } catch (SocketError &e) {
            cout << "MacroDaemon accept() error: " << e.what() << endl;
        }
        usleep(100);
    }
    remote_udev = new RemoteUDevice(kbd_com);
    initScriptDir(home_dir + "/scripts-enabled");
}

MacroDaemon::~MacroDaemon() {
    for (auto &[_, s] : scripts)
        delete s;
    delete remote_udev;
}

void MacroDaemon::initScriptDir(const std::string &dir_path) {
    auto dir = shared_ptr<DIR>(opendir(dir_path.c_str()), &closedir);
    struct dirent *entry;
    while ((entry = readdir(dir.get()))) {
        stringstream path_ss;
        path_ss << dir_path << "/" << entry->d_name;
        string path = path_ss.str();

        // Attempt to load the script:
        try {
            loadScript(path);
        } catch (exception &e) {
            cout << "Error: " << e.what() << endl;
        }
    }
    auto files = mkuniq(fsw.addFrom(dir_path));
}

void MacroDaemon::loadScript(const std::string &rel_path) {
    string bn = pathBasename(rel_path);
    if (!goodLuaFilename(bn)) {
        cout << "Wrong filename, not loading: " << bn << endl;
        return;
    }

    ChDir cd(home_dir + "/scripts");

    char *rpath_chars = realpath(rel_path.c_str(), nullptr);
    if (rpath_chars == nullptr)
        throw SystemError("Error in realpath: ", errno);
    string path(rpath_chars);
    free(rpath_chars);

    cout << "Preparing to load script: " << rel_path << endl;

    struct stat stbuf;
    if (stat(path.c_str(), &stbuf) == -1) {
        cout << "Warning: unable to stat(), not loading." << endl;
        return;
    }

    unsigned perm = stbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    // Strictly require chmod 744 on the script files.
    if (perm != 0744 && stbuf.st_uid == getuid()) {
        cout << "Warning: require chmod 744 and uid=" << getuid() <<
                " on script files, not loading." << endl;
        return;
    }

    Script *sc = new Script(path);
    sc->open(remote_udev, "udev");

    string name = pathBasename(rel_path);

    if (scripts.find(name) != scripts.end()) {
        // Script already loaded, reload it
        delete scripts[name];
        scripts.erase(name);
    }

    cout << "Loaded script: " << name << endl;
    scripts[name] = sc;
}

void MacroDaemon::unloadScript(const std::string &rel_path) {
    string name = pathBasename(rel_path);
    if (scripts.find(name) != scripts.end()) {
        cout << "delete scripts[" << name << "]" << endl;
        delete scripts[name];
        scripts.erase(name);
    }
}

struct script_error_info {
    lua_Debug ar;
    char path[];
};

#if 0
// FIXME: See FIXME in MacroDaemon::notify
// TODO: My idea for this would be to run "gedit $info->path +$info->ar.currentline",
//       then figure out what other editors support opening a file on a specific line
//       and let the user choose which editor to use.
void viewSource(NotifyNotification *n, char *action, script_error_info *info) {
    pid_t pid;
    fprintf(stderr, "Line nr: %d\n", info->ar.currentline);
    fprintf(stderr, "path: %s\n", info->path);
    fflush(stderr);
}
#endif

void f(void *) {
    fprintf(stderr, "GOT EM\n"); fflush(stderr);
}

void MacroDaemon::notify(string title, string msg) {
    NotifyNotification *n;
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    string fire_icon_path(cwd);
    fire_icon_path += "/icons/fire.svg";
    n = notify_notification_new(title.c_str(), msg.c_str(), fire_icon_path.c_str());
    notify_notification_set_timeout(n, 12000);
    notify_notification_set_urgency(n, NOTIFY_URGENCY_CRITICAL);
    notify_notification_set_app_name(n, "Hawck");

    // FIXME: The viewSource callback is not executed.
    // Apparently I need to call g_main_loop_run, I tried it and nothing
    // happened, I have no idea how to get this working.
    // Furthermore g_main_loop_run is a blocking call so I'd have to
    // restructure this so that the notification stuff happens in another
    // thread.
    #if 0
    script_error_info *info =
        (script_error_info *) malloc(sizeof(script_error_info) + script->abs_src.size() + 1);
    memcpy(&info->ar, ar, sizeof(info->ar));
    strcpy(info->path, script->abs_src.c_str());
    notify_notification_add_action(n,
                                   "view_source",
                                   "View source",
                                   NOTIFY_ACTION_CALLBACK(viewSource),
                                   info,
                                   free);
    g_signal_connect(n, "closed", free, info);
    #endif
    
    if (!notify_notification_show(n, nullptr)) {
        fprintf(stderr, "Failed to show notification: %s\n", msg.c_str());
    }
}

static void handleSigPipe(int) {
    fprintf(stderr, "MacroD aborting due to SIGPIPE\n");
    abort();
}

bool MacroDaemon::runScript(Lua::Script *sc, const struct input_event &ev) {
    bool repeat = true;

    const char *ev_val = (ev.value <= 2) ? evval[ev.value] : "?";
    const char *ev_type = event_str[ev.type];

    sc->set("__event_type",            ev_type);
    sc->set("__event_type_num",  (int) ev.type);
    sc->set("__event_code",      (int) ev.code);
    sc->set("__event_value",           ev_val);
    sc->set("__event_value_num", (int) ev.value);

    try {
        auto [succ] = sc->call<bool>("__match");
        repeat = !succ;
    } catch (const LuaError &e) {
        std::string report = e.fmtReport();
        notify("Lua error", report);
        syslog(LOG_ERR, "LUA:%s", report.c_str());
        repeat = true;
    }

    // lua_pop(L, lua_gettop(L));

    return repeat;
}

void MacroDaemon::run() {
    signal(SIGPIPE, handleSigPipe);

    KBDAction action;
    struct input_event &ev = action.ev;

    fsw.setWatchDirs(true);
    fsw.setAutoAdd(false);
    fsw.begin([&](FSEvent &ev) {
                  lock_guard<mutex> lock(scripts_mtx);
                  try {
                      if (ev.mask & IN_DELETE) {
                          cout << "Deleting script: " << ev.name << endl;
                          unloadScript(ev.name);
                      } else if (ev.mask & IN_MODIFY) {
                          cout << "Reloading script: " << ev.path << endl;
                          if (!S_ISDIR(ev.stbuf.st_mode)) {
                              unloadScript(pathBasename(ev.path));
                              loadScript(ev.path);
                          }
                      } else if (ev.mask & IN_CREATE) {
                          loadScript(ev.path);
                      } else {
                          cout << "Received unhandled event" << endl;
                      }
                  } catch (exception &e) {
                      cout << e.what() << endl;
                  }
                  return true;
              });

    for (;;) {
        bool repeat = true;

        kbd_com->recv(&action);

        {
            lock_guard<mutex> lock(scripts_mtx);
            // Look for a script match.
            for (auto &[_, sc] : scripts)
                if (sc->isEnabled() && !(repeat = runScript(sc, ev)))
                    break;
        }
        
        if (repeat || ALWAYS_REPEAT_KEYS)
            remote_udev->emit(&ev);

        remote_udev->done();
    }
}

