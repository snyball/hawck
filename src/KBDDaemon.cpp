/* =====================================================================================
 * Keyboard daemon.
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
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */

#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <regex>

extern "C" {
    #include <syslog.h>
    #include <grp.h>
}

#include "KBDDaemon.hpp"
#include "CSV.hpp"
#include "utils.hpp"
#include "Daemon.hpp"
#include "Permissions.hpp"
#include "LuaUtils.hpp"

#if DANGER_DANGER_LOG_KEYS
    #warning "Currently logging keypresses"
    #warning "DANGER_DANGER_LOG_KEYS should **only** be enabled while debugging."
#endif

using namespace std;
using namespace Permissions;
using namespace Lua;

KBDDaemon::KBDDaemon() :
    kbd_com("/var/lib/hawck-input/kbd.sock")
{
    initPassthrough();
}

KBDDaemon::~KBDDaemon() {}

void KBDDaemon::unloadPassthrough(std::string path) {
    if (key_sources.find(path) != key_sources.end()) {
        auto vec = key_sources[path];
        for (int code : *vec)
            key_visibility[code] = KEY_HIDE;
        delete vec;
        key_sources.erase(path);

        syslog(LOG_INFO, "Removing passthrough keys from: %s", path.c_str());

        // Re-add keys
        for (const auto &[_, vec] : key_sources) {
            (void) _;
            for (int code : *vec)
                key_visibility[code] = KEY_SHOW;
        }
    }
}

void KBDDaemon::loadPassthrough(std::string rel_path) {
    try {
        // The CSV file is being reloaded after a change, remove the old keys.
        string path = realpath_safe(rel_path);

        unloadPassthrough(path);

        CSV csv(path);
        auto cells = mkuniq(csv.getColCells("key_code"));
        auto cells_i = mkuniq(new vector<int>());
        for (auto *code_s : *cells) {
            int i;
            try {
                i = stoi(*code_s);
            } catch (const std::exception &e) {
                continue;
            }
            if (i >= 0 && i < KEY_MAX) {
                key_visibility[i] = KEY_SHOW;
                cells_i->push_back(i);
            } else {
                syslog(LOG_WARNING, "Key code was out of range: %d", i);
                continue;
            }
        }
        key_sources[path] = cells_i.release();
        keys_fsw.add(path);
        syslog(LOG_INFO, "Loaded passthrough keys from: %s", path.c_str());
    } catch (const CSV::CSVError &e) {
        syslog(LOG_ERR, "CSV parse error in '%s': %s",
               rel_path.c_str(), e.what());
    } catch (const SystemError &e) {
        syslog(LOG_ERR, "Unable to load csv data from '%s': %s",
               rel_path.c_str(), e.what());
    }
}

void KBDDaemon::loadPassthrough(FSEvent *ev) {
    unsigned perm = ev->stbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

    // Require that the file permission mode is 644 and that the file is owned
    // by the daemon user.
    if (perm == 0644 && ev->stbuf.st_uid == getuid()) {
        loadPassthrough(ev->path);
    } else {
        auto perm_str = fmtPermissions(ev->stbuf);
        syslog(LOG_ERR, "Invalid permissions for '%s', require rw-r--r-- hawck-input:*, "
                        "but was: %s",
               ev->path.c_str(), perm_str.c_str());
    }
}

void KBDDaemon::initPassthrough() {
    for (auto i = 0; i < KEY_MAX; i++)
        key_visibility[i] = KEY_HIDE;
    auto files = mkuniq(keys_fsw.addFrom(data_dirs["keys"]));
    for (auto &file : *files)
        loadPassthrough(&file);
}

void KBDDaemon::setup() {}

void KBDDaemon::startPassthroughWatcher() {
    keys_fsw.asyncWatch([this](FSEvent &ev) {
        syslog(LOG_INFO, "kbd file change on: %s", ev.path.c_str());
        if (ev.mask & IN_DELETE_SELF)
            unloadPassthrough(ev.path);
        else if (ev.mask & (IN_CREATE | IN_MODIFY))
            loadPassthrough(&ev);
        return true;
    });
}

void KBDDaemon::run() {
    KBDAction action;
    memset(&action, '\0', sizeof(action));
    setup();
    startPassthroughWatcher();
    kbman.setup();
    kbman.startHotplugWatcher();

    for (;;) {
        action.done = 0;
        if (!kbman.getEvent(&action))
            continue;

        // Check if the key is listed in the passthrough set.
        KeyVisibility key_vis;
        if (action.ev.code >= KEY_MAX) {
            syslog(LOG_ERR, "Received key was out of range: %d", action.ev.code);
            key_vis = KEY_HIDE;
        } else {
            key_vis = key_visibility[action.ev.code];
        }

        if (key_vis == KEY_SHOW) {
            input_event orig_ev = action.ev;

            // Pass key to Lua executor
            try {
                kbd_com.send(&action);

                // Receive keys to emit from the macro daemon.
                for (;;) {
                    kbd_com.recv(&action, timeout);
                    if (action.done)
                        break;
                    udev.emit(&action.ev);
                }
                // Flush received keys and continue on.
                udev.flush();
                continue;
            } catch (const SocketError &e) {
                syslog(LOG_INFO, "Resetting connection to MacroD");

                udev.emit(&orig_ev);
                udev.upAll();
                udev.flush();

                auto unlock = kbman.unlockAll();
                syslog(LOG_CRIT, "Unable to communicate with MacroD, reconnecting ...");
                // Reconnect.
                kbd_com.recon();

                // Skip the received event
                continue;
            }
        }

        udev.emit(&action.ev);
        udev.flush();
    }
}

void KBDDaemon::setEventDelay(int delay) {
    udev.setEventDelay(delay);
}

