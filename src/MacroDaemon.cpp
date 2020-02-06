/* =====================================================================================
 * Macro daemon.
 *
 * Copyright (C) 2018 Jonas Møller (no) <jonas.moeller2@protonmail.com>
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
#include "LuaConfig.hpp"
#include "XDG.hpp"

using namespace Lua;
using namespace Permissions;
using namespace std;

static const char *event_str[EV_CNT];

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
    XDG xdg("hawck");

    notify_on_err = true;
    stop_on_err = false;
    eval_keydown = true;
    eval_keyup = true;
    eval_repeat = true;
    disabled = false;

    auto [grp, grpbuf] = getgroup("hawck-input-share");
    (void) grpbuf;
    chown("/var/lib/hawck-input/kbd.sock", getuid(), grp->gr_gid);
    chmod("/var/lib/hawck-input/kbd.sock", 0660);
    initEventStrs();
    notify_init("Hawck");
    xdg.mkpath(0700, XDG_DATA_HOME, "scripts-enabled");
    initScriptDir(xdg.path(XDG_DATA_HOME, "scripts-enabled"));
}

void MacroDaemon::getConnection() {
    if (kbd_com)
        delete kbd_com;
    syslog(LOG_INFO, "Listening for a connection ...");

    // Keep looping around until we get a connection.
    for (;;) {
        try {
            int fd = kbd_srv.accept();
            kbd_com = new UNIXSocket<KBDAction>(fd);
            syslog(LOG_INFO, "Got a connection");
            break;
        } catch (SocketError &e) {
            cout << "MacroDaemon accept() error: " << e.what() << endl;
        }
        // Wait for 0.1 seconds
        usleep(100000);
    }

    remote_udev.setConnection(kbd_com);
}

MacroDaemon::~MacroDaemon() {
    for (auto &[_, s] : scripts) {
        (void) _;
        delete s;
    }
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

    if (!checkFile(path, "frwxr-xr-x ~:*"))
        return;

    auto sc = mkuniq(new Script());
    sc->call("require", "init");
    sc->open(&remote_udev, "udev");
    sc->from(path);

    string name = pathBasename(rel_path);

    if (scripts.find(name) != scripts.end()) {
        // Script already loaded, reload it
        delete scripts[name];
        scripts.erase(name);
    }

    cout << "Loaded script: " << name << endl;
    scripts[name] = sc.release();
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
extern "C" void viewSource(NotifyNotification *n, char *action, script_error_info *info) {
    pid_t pid;
    fprintf(stderr, "Line nr: %d\n", info->ar.currentline);
    fprintf(stderr, "path: %s\n", info->path);
    fflush(stderr);
}
#endif

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
    // fprintf(stderr, "MacroD aborting due to SIGPIPE\n");
    // abort();
}

bool MacroDaemon::runScript(Lua::Script *sc, const struct input_event &ev) {
    bool repeat = true;

    try {
        auto [succ] = sc->call<bool>("__match", (int)ev.value, (int)ev.code, (int)ev.type);
        repeat = !succ;
    } catch (const LuaError &e) {
        if (stop_on_err)
            sc->setEnabled(false);
        std::string report = e.fmtReport();
        if (notify_on_err)
            notify("Lua error", report);
        syslog(LOG_ERR, "LUA:%s", report.c_str());
        repeat = true;
    }

    return repeat;
}

void MacroDaemon::reloadAll() {
    lock_guard<mutex> lock(scripts_mtx);
    ChDir cd(home_dir + "/scripts");
    for (auto &[_, sc] : scripts) {
        (void) _;
        try {
            sc->setEnabled(true);
            sc->reset();
            sc->call("require", "init");
            sc->open(&remote_udev, "udev");
            sc->reload();
        } catch (const LuaError& e) {
            syslog(LOG_ERR, "Error when reloading script: %s", e.what());
            sc->setEnabled(false);
        }
    }
}

void MacroDaemon::run() {
    signal(SIGPIPE, handleSigPipe);

    KBDAction action;
    struct input_event &ev = action.ev;

    // Setup/start LuaConfig
    LuaConfig conf(home_dir + "/lua-comm.fifo", home_dir + "/json-comm.fifo", home_dir + "/cfg.lua");
    conf.addOption("notify_on_err", &notify_on_err);
    conf.addOption("stop_on_err", &stop_on_err);
    conf.addOption("eval_keydown", &eval_keydown);
    conf.addOption("eval_keyup", &eval_keyup);
    conf.addOption("eval_repeat", &eval_repeat);
    conf.addOption("disabled", &disabled);
    conf.addOption<string>("keymap", [this](string) {reloadAll();});
    conf.start();

    fsw.setWatchDirs(true);
    fsw.setAutoAdd(false);
    fsw.asyncWatch([this](FSEvent &ev) {
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

    getConnection();

    cout << "Running MacroD mainloop ..." << endl;

    for (;;) {
        try {
            bool repeat = true;

            kbd_com->recv(&action);

            if (!( (!eval_keydown && ev.value == 1) ||
                   (!eval_keyup && ev.value == 0) ) && !disabled)
            {
                lock_guard<mutex> lock(scripts_mtx);
                // Look for a script match.
                for (auto &[_, sc] : scripts) {
                    (void) _;
                    if (sc->isEnabled() && !(repeat = runScript(sc, ev)))
                        break;
                }
            }

            if (repeat)
                remote_udev.emit(&ev);

            remote_udev.done();
        } catch (const SocketError& e) {
            // Reset connection
            syslog(LOG_ERR, "Socket error: %s", e.what());
            notify("Socket error", "Connection to InputD timed out, reconnecting ...");
            getConnection();
        }
    }
}
