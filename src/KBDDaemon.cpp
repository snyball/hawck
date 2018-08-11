/* =====================================================================================
 * Keyboard daemon.
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
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */

extern "C" {
    #include <signal.h>
}

#include <fstream>
#include <iostream>
#include <string>

#include "KBDDaemon.hpp"
#include "CSV.hpp"
#include "utils.hpp"

// #undef DANGER_DANGER_LOG_KEYS
// #define DANGER_DANGER_LOG_KEYS 1

#if DANGER_DANGER_LOG_KEYS
    #warning "Currently logging keypresses"
    #warning "DANGER_DANGER_LOG_KEYS should **only** be enabled while debugging."
#endif

using namespace std;

KBDDaemon::KBDDaemon(const char *device) :
    kbd_com("kbd.sock"),
    kbd(device)
{
    home_path = "/var/lib/hawckd-input";
    data_dirs["keys"] = home_path + "/keys";
    initPassthrough();
}

KBDDaemon::~KBDDaemon() {
}

void KBDDaemon::unloadPassthrough(std::string path) {
    if (key_sources.find(path) != key_sources.end()) {
        auto vec = key_sources[path];
        for (int code : *vec)
            passthrough_keys.erase(code);
        delete vec;
        key_sources.erase(path);

        printf("RM: %s\n", path.c_str());

        // Re-add keys
        for (const auto &[_, vec] : key_sources)
            for (int code : *vec)
                passthrough_keys.insert(code);
    }
}

void KBDDaemon::loadPassthrough(std::string rel_path) {
    // FIXME: Actually handle these errors.
    try {
        // The CSV file is being reloaded after a change,
        // remove the old keys.
        char *rpath = realpath(rel_path.c_str(), nullptr);
        if (rpath == nullptr) {
            throw SystemError("Error in realpath() unable to get path for: " + rel_path);
        }
        string path(rpath);
        free(rpath);

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
            if (i >= 0) {
                passthrough_keys.insert(i);
                cells_i->push_back(i);
            }
        }
        key_sources[path] = cells_i.release();
        fsw.add(path);
        printf("LOADED: %s\n", path.c_str());
    } catch (const CSV::CSVError &e) {
        cout << "loadPassthrough error: " << e.what() << endl;
    } catch (const SystemError &e) {
        cout << "loadPassthrough error: " << e.what() << endl;
    }
}

void KBDDaemon::loadPassthrough(FSEvent *ev) {
    unsigned perm = ev->stbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);

    // Require that the file permission mode is 644 and that the file is
    // owned by the daemon user.
    if (perm == 0644 && ev->stbuf.st_uid == getuid()) {
        printf("OK: %s\n", ev->path.c_str());
        loadPassthrough(ev->path);
    }
}

void KBDDaemon::initPassthrough() {
    auto files = mkuniq(fsw.addFrom(data_dirs["keys"]));
    cout << "Added data_dir" << endl;
    for (auto &file : *files)
        loadPassthrough(&file);
}

static void handleSigPipe(int) {
    fprintf(stderr, "KBDDaemon aborting due to SIGPIPE\n");
    abort();
}

void KBDDaemon::run() {
    signal(SIGPIPE, handleSigPipe);

    KBDAction action;

    kbd.lock();

    // 10 consecutive socket errors will lead to the keyboard daemon
    // aborting.
    static const int MAX_ERRORS = 10;
    int errors = 0;
    thread fsw_thread([&]() -> void {fsw.watch();});
    fsw_thread.detach();

    for (;;) {
        action.done = 0;
        try {
            kbd.get(&action.ev);
        } catch (KeyboardError &e) {
            // Close connection to let the macro daemon know it should
            // terminate.
            kbd_com.close();
            abort();
        }

        vector<FSEvent*> fs_events = fsw.getEvents();
        for (auto ev : fs_events) {
            if (ev->mask & IN_DELETE_SELF)
                unloadPassthrough(ev->path);
            else if (ev->mask & (IN_CREATE | IN_MODIFY))
                loadPassthrough(ev);
            delete ev;
        }

#if DANGER_DANGER_LOG_KEYS
        cout << "Received keyboard action ." << endl;
        fflush(stdout);
        fprintf(stderr, "GOT EVENT %d WITH KEY %d\n", action.ev.value, (int)action.ev.code);
        fflush(stderr);
#endif

        // Check if the key is listed in the passthrough set.
        if (passthrough_keys.count(action.ev.code)) {
            // Pass key to Lua executor
            try {
                kbd_com.send(&action);

                // Receive keys to emit from the macro daemon.
                for (;;) {
                    kbd_com.recv(&action);
                    if (action.done)
                        break;
                    udev.emit(&action.ev);
                }
                // Flush received keys and continue on.
                udev.flush();
                errors = 0;
                // Skip emmision of the key if everything went OK
                continue;
            } catch (SocketError &e) {
                cout << e.what() << endl;
                // If we're getting constant errors then the daemon needs
                // to be stopped, as the macro daemon might have crashed
                // or run into an error.
                if (errors++ > MAX_ERRORS) {
                    kbd_com.close();
                    abort();
                }
                // On error control flow continues on to udev.emit()
            }
        }

#if DANGER_DANGER_LOG_KEYS
        fprintf(stderr, "RE-EMIT KEY\n");
#endif

        udev.emit(&action.ev);
        udev.flush();
    }

    fprintf(stderr, "QUIT LOOP\n"); fflush(stderr);
}
