/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * FSWatcher.hpp, file system monitoring.                                            *
 *                                                                                   *
 * Copyright (C) 2018 Jonas Møller (no) <jonasmo441@gmail.com>                       *
 * All rights reserved.                                                              *
 *                                                                                   *
 * Redistribution and use in source and binary forms, with or without                *
 * modification, are permitted provided that the following conditions are met:       *
 *                                                                                   *
 * 1. Redistributions of source code must retain the above copyright notice, this    *
 *    list of conditions and the following disclaimer.                               *
 * 2. Redistributions in binary form must reproduce the above copyright notice,      *
 *    this list of conditions and the following disclaimer in the documentation      *
 *    and/or other materials provided with the distribution.                         *
 *                                                                                   *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND   *
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED     *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE            *
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE      *
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL        *
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR        *
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER        *
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,     *
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

extern "C" {
    #include <sys/inotify.h>
    #include <limits.h>
    #include <sys/stat.h>
}

#include <vector>
#include <string>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>

static constexpr size_t EVBUF_ITEMS = 10;

struct FSEvent {
    std::string path;
    uint32_t mask;
    struct stat stbuf;

    explicit FSEvent(struct inotify_event *ev, std::string path);
};

class FSWatcher {
private:
    int fd;
    char *dir_path;
    char evbuf[EVBUF_ITEMS * (sizeof(struct inotify_event) + NAME_MAX + 1)];
    std::unordered_map<std::string, int> path_to_wd;
    std::unordered_map<int, std::string> wd_to_path;
    std::mutex events_mtx;
    std::vector<FSEvent *> events;
    std::atomic<bool> running = true;

public:
    FSWatcher();
    ~FSWatcher();

    /**
     * Add a single file.
     */
    void add(std::string path);

    /**
     * Remove a single file.
     */
    void remove(std::string path);

    /**
     * Add an entire directory tree, this only adds the directories.
     */
    void addTree();

    /**
     * Adds all files in a directory tree, this does not add any
     * directories.
     */
    void addTreeFiles();

    /**
     * Add all files in a directory and the directory itself. (Does not add sub-directories.)
     * Files created in the directory after the call are automatically added.
     */
    void addFrom(std::string path);

    /**
     * Remove a directory and the files within.
     */
    void removeFrom(std::string path);

    /**
     * Watch the added files.
     */
    void watch();

    /**
     * Stop watching. This call has no effect if watch() was
     * not called beforehand.
     */
    inline void stop() {
        running = false;
    }

    /**
     * Get events.
     *
     * This function is thread safe for a single reader, race-conditions
     * occur with multiple readers.
     */
    std::vector<FSEvent *> getEvents();
};
