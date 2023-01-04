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
#include <filesystem>

extern "C" {
    #include <libnotify/notify.h>
    #include <syslog.h>
}

#include "Daemon.hpp"
#include "MacroDaemon.hpp"
#include "LuaUtils.hpp"
#include "utils.hpp"
#include "Permissions.hpp"
#include "LuaConfig.hpp"
#include "XDG.hpp"
#include "KBDB.hpp"
#include "Popen.hpp"

using namespace Lua;
using namespace Permissions;
using namespace std;
namespace fs = std::filesystem;

static bool macrod_main_loop_running = true;

MacroDaemon::MacroDaemon()
    : kbd_srv("/var/lib/hawck-input/kbd.sock"),
      xdg("hawck")
{
    notify_on_err = true;
    stop_on_err = false;
    eval_keydown = true;
    eval_keyup = true;
    eval_repeat = true;
    disabled = false;

    auto [grp, grpbuf] = getgroup("hawck-input-share");
    (void) grpbuf;
    if (chown("/var/lib/hawck-input/kbd.sock", getuid(), grp->gr_gid) == -1)
        throw SystemError("Unable to chown kbd.sock: ", errno);
    if (chmod("/var/lib/hawck-input/kbd.sock", 0660) == -1)
        throw SystemError("Unable to chmod kbd.sock: ", errno);
    notify_init("Hawck");
    xdg.mkpath(0755, XDG_CONFIG_HOME, "scripts");
    initScriptDir(xdg.path(XDG_CONFIG_HOME, "scripts"));
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
            syslog(LOG_ERR, "Error in accept(): %s", e.what());
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
    for (auto entry : fs::directory_iterator(dir_path)) {
        try {
            loadScript(entry.path());
        } catch (exception &e) {
            notify("Hawck Script Error", e.what(), "hawck", NOTIFY_URGENCY_CRITICAL);
            syslog(LOG_ERR, "Unable to load script '%s': %s", pathBasename(entry.path()).c_str(),
                   e.what());
        }
    }
    fsw.addFrom(dir_path);
}

void MacroDaemon::loadScript(const std::string &path) {
    if (stringStartsWith(path, ".") || !(stringEndsWith(path, ".lua") ||
                                         stringEndsWith(path, ".hwk"))) {
        syslog(LOG_NOTICE,
               "Not loading: %s, filename must end in .lua or .hwk and may not start with a leading '.'",
               path.c_str());
        return;
    }

    auto rpath = realpath_safe(path);
    if (!checkFile(rpath, "frwxr-xr-x ~:*"))
        return;

    auto sc = mkuniq(new Script());
    auto chdir = xdg.cd(XDG_DATA_HOME, "scripts");
    sc->call("require", "init");
    sc->open(&remote_udev, "udev");
    if (stringEndsWith(path, ".hwk")) {
        sc->exec(pathBasename(path), (Popen("hwk2lua", path)).readOnce());
    } else if (stringEndsWith(path, ".lua")) {
        sc->from(path);
    }
    auto name = pathBasename(path);
    if (scripts.find(name) != scripts.end()) {
        delete scripts[name];
        scripts.erase(name);
    }
    syslog(LOG_INFO, "Loaded script: %s", path.c_str());
    notify(pathBasename(path), "<i>Loaded</i> script");
    scripts[name] = sc.release();
}

void MacroDaemon::unloadScript(const std::string &rel_path) noexcept {
    string name = pathBasename(rel_path);
    if (scripts.find(name) != scripts.end()) {
        syslog(LOG_INFO, "Deleting script: %s", name.c_str());
        delete scripts[name];
        scripts.erase(name);
        notify(name, "<i>Unloaded</i> script");
    } else {
        syslog(LOG_ERR, "Attempted to delete non-existent script: %s", name.c_str());
    }
}

struct script_error_info {
    lua_Debug ar;
    char path[];
};

void MacroDaemon::notify(string title, string msg) {
    notify(title, msg, "hawck", NOTIFY_URGENCY_NORMAL);
}

void MacroDaemon::notify(string title, string msg, string icon, NotifyUrgency urgency) {
    // AFAIK you don't have to free the memory manually, but I could be wrong.
    lock_guard<mutex> lock(last_notification_mtx);
    tuple<string, string> notif(title, msg);
    if (notif == last_notification)
        return;
    last_notification = notif;

    NotifyNotification *n = notify_notification_new(title.c_str(), msg.c_str(), icon.c_str());
    notify_notification_set_timeout(n, 12000);
    notify_notification_set_urgency(n, urgency);
    notify_notification_set_app_name(n, "Hawck");

    if (!notify_notification_show(n, nullptr)) {
        syslog(LOG_INFO, "Notifications cannot be shown.");
    }
}

static void handleSigPipe(int) {}

#if 0
static void handleSigTerm(int) {
    macrod_main_loop_running = false;
}
#endif

bool MacroDaemon::runScript(Lua::Script *sc, const struct input_event &ev, string kbd_hid) {
    static bool had_stack_leak_warning = false;
    bool repeat = true;

    try {
        auto [succ] = sc->call<bool>("__match",
                                     (int)ev.value,
                                     (int)ev.code,
                                     (int)ev.type,
                                     kbd_hid);
        if (lua_gettop(sc->getL()) != 0) {
            if (!had_stack_leak_warning) {
                syslog(LOG_WARNING,
                       "API misuse causing Lua stack leak of %d elements.",
                       lua_gettop(sc->getL()));
                // Don't spam system logs:
                had_stack_leak_warning = true;
            }
            lua_settop(sc->getL(), 0);
        }
        repeat = !succ;
    } catch (const LuaError &e) {
        if (stop_on_err)
            sc->setEnabled(false);
        std::string report = e.fmtReport();
        if (notify_on_err)
            notify("Lua error", report, "hawck", NOTIFY_URGENCY_CRITICAL);
        syslog(LOG_ERR, "LUA:%s", report.c_str());
        repeat = true;
    }

    return repeat;
}

void MacroDaemon::reloadAll() {
    // Disabled due to restructuring of the script load process
    #if 0
    lock_guard<mutex> lock(scripts_mtx);
    auto chdir = xdg.cd(XDG_DATA_HOME, "scripts");
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
    #endif
}

void MacroDaemon::startScriptWatcher() {
    fsw.setWatchDirs(true);
    fsw.setAutoAdd(true);
    // TODO: Display desktop notifications for these syslogs.
    //       You'll have to evaluate the thread-safety of doing that, and you
    //       might have to push onto a shared notification queue rather than
    //       just sending the messages directly from here.
    fsw.asyncWatch([this](FSEvent &ev) {
        lock_guard<mutex> lock(scripts_mtx);
        try {
            // Don't react to the directory itself.
            if (ev.path == xdg.path(XDG_CONFIG_HOME, "scripts"))
                return true;

            if (ev.mask & IN_DELETE) {
                syslog(LOG_INFO, "Deleting script: %s", ev.path.c_str());
                unloadScript(ev.name);
            } else if (ev.mask & IN_MODIFY) {
                syslog(LOG_INFO, "Reloading script: %s", ev.path.c_str());
                if (!S_ISDIR(ev.stbuf.st_mode))
                    loadScript(ev.path);
            } else if (ev.mask & IN_CREATE) {
                syslog(LOG_INFO, "Loading new script: %s", ev.path.c_str());
                loadScript(ev.path);
            } else if (ev.mask & IN_ATTRIB) {
                if (ev.stbuf.st_mode & S_IXUSR) {
                    loadScript(ev.path);
                } else {
                    unloadScript(ev.path);
                }
            }
        } catch (exception &e) {
            notify("Script Error", e.what(), "hawck", NOTIFY_URGENCY_CRITICAL);
            syslog(LOG_ERR, "Error while loading %s: %s", ev.path.c_str(), e.what());
        }
        return true;
    });
}

void MacroDaemon::run() {
    syslog(LOG_INFO, "Setting up MacroDaemon ...");

    // FIXME: Need to handle socket timeouts before I can use this SIGTERM handler.
    //signal(SIGTERM, handleSigTerm);

    signal(SIGPIPE, handleSigPipe);


    // Setup/start LuaConfig
    xdg.mkfifo("lua-comm.fifo");
    xdg.mkfifo("json-comm.fifo");

    LuaConfig conf(xdg.path(XDG_RUNTIME_DIR, "lua-comm.fifo"),
                   xdg.path(XDG_RUNTIME_DIR, "json-comm.fifo"),
                   xdg.path(XDG_DATA_HOME, "cfg.lua"));
    conf.addOption("notify_on_err", &notify_on_err);
    conf.addOption("stop_on_err", &stop_on_err);
    conf.addOption("eval_keydown", &eval_keydown);
    conf.addOption("eval_keyup", &eval_keyup);
    conf.addOption("eval_repeat", &eval_repeat);
    conf.addOption("disabled", &disabled);
    conf.addOption<string>("keymap", [this](string) {reloadAll();});
    conf.start();

    startScriptWatcher();

    KBDAction action;
    struct input_event &ev = action.ev;
    KBDB kbdb;

    getConnection();

    syslog(LOG_INFO, "Starting main loop");

    while (macrod_main_loop_running) {
        try {
            bool repeat = true;

            kbd_com->recv(&action);
            string kbd_hid = kbdb.getID(&action.dev_id);

            if (!( (!eval_keydown && ev.value == 1) ||
                   (!eval_keyup && ev.value == 0) ) && !disabled)
            {
                lock_guard<mutex> lock(scripts_mtx);
                // Look for a script match.
                for (auto &[_, sc] : scripts) {
                    (void) _;
                    if (sc->isEnabled() && !(repeat = runScript(sc, ev, kbd_hid)))
                        break;
                }
            }

            if (repeat)
                remote_udev.emit(&ev);

            remote_udev.done();
        } catch (const SocketError& e) {
            // Reset connection
            syslog(LOG_ERR, "Socket error: %s", e.what());
            notify("Socket error", "Connection to InputD timed out, reconnecting ...", "hawck", NOTIFY_URGENCY_NORMAL);
            getConnection();
        }
    }

    syslog(LOG_INFO, "macrod exiting ...");
}
