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

#pragma once

#include <unordered_map>
#include <set>
#include <thread>

#include "KBDConnection.hpp"
#include "UNIXSocket.hpp" 

#include "UDevice.hpp"
#include "LuaUtils.hpp"
#include "Keyboard.hpp"
#include "SystemError.hpp"
#include "FSWatcher.hpp"

extern "C" {
    #include <fcntl.h>
    #include <sys/stat.h>
}

// XXX: DO NOT ENABLE THIS FOR NON-DEBUGGING BUILDS
//      This will log keypresses to stdout
#define DANGER_DANGER_LOG_KEYS 0

class KBDDaemon {
private:
    std::set<int> passthrough_keys;
    std::mutex passthrough_keys_mtx;
    std::string home_path = "/var/lib/hawck-input";
    std::unordered_map<std::string, std::string> data_dirs = {
        {"keys", home_path + "/keys"}
    };
    std::unordered_map<std::string, std::vector<int>*> key_sources;
    UNIXSocket<KBDAction> kbd_com;
    UDevice udev;
    Keyboard kbd;
    std::vector<Keyboard> kbds;
    FSWatcher fsw;

public:
    explicit KBDDaemon(const char *device);
    KBDDaemon();
    ~KBDDaemon();

    void initPassthrough();

    /** Listen on a new device.
     *
     * @param device Full path to the device in /dev/input/
     */
    void addDevice(const char *device);

    /**
     * Load passthrough keys from a file at `path`.
     *
     * @param path Path to csv file containing a `key_codes` column.
     */
    void loadPassthrough(std::string path);

    /**
     * Load passthrough keys from a file system event.
     *
     * @param ev File system event to load from.
     */
    void loadPassthrough(FSEvent *ev);

    /** Unload passthrough keys from file at `path`.
     *
     * @param path Path to csv file to remove key codes from.
     */
    void unloadPassthrough(std::string path);

    /**
     * Start running the daemon.
     */
    void run();
};
