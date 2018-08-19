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

#pragma once

/** @file FSWatcher.hpp
 *
 * @brief File system watcher (inotify)
 *
 * This class exposes the Linux kernel inotify API,
 * allowing programs to listen for file system events.
 */

extern "C" {
    #include <sys/inotify.h>
    #include <limits.h>
    #include <sys/stat.h>
    #include <unistd.h>
}

#include <vector>
#include <string>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <stdexcept>

/** Number of items inside the event buffer of \link FSWatcher \endlink */
static constexpr size_t EVBUF_ITEMS = 10;

/** File system event */
struct FSEvent {
    /** Absolute path to file. */
    std::string path;
    /** Mask received from inotify. */
    uint32_t mask = 0;
    /** Name of file (relevent for directory events.) */
    std::string name;
    /** stat() of the file. */
    struct stat stbuf;
    /** True if this event was sent as a result of FSWatcher::add() */
    bool added = false;

    /** Initialize an FSEvent from an inotify event */
    FSEvent(struct inotify_event *ev, std::string path);

    /** Initialize an FSEvent from an absolute path, assumed to
     *  be an `added` event. */
    explicit FSEvent(std::string path);
};

using FSWatchFn = std::function<bool(FSEvent &ev)>;

/** File system watcher.
 *
 * Uses the Linux inotify API to listen for file system
 * events.
 */
class FSWatcher {
private:
    /** Inotify main file descriptor. */
    int fd;
    /** Event buffer used to receive inotify events. */
    char evbuf[EVBUF_ITEMS * (sizeof(struct inotify_event) + NAME_MAX + 1)];
    /** Maps paths to watch descriptors. */
    std::unordered_map<std::string, int> path_to_wd;
    /** Maps ids received from inotify to paths, ids are referred to
     *  as wd (watch-descriptor.) */
    std::unordered_map<int, std::string> wd_to_path;
    /** Protects \link FSWatcher::events \endlink */
    std::mutex events_mtx;
    /** Holds received events, is emptied by calling
     *  \link FSWatcher::getEvents() \endlink */
    std::vector<FSEvent *> events;
    /** Set to true when \link FSWatcher::watch() \endlink is called,
     *  is set to false by calling \link FSWatcher::stop() \endlink */
    std::atomic<bool> running = false;
    /** Whether or not to automatically add files that are created in
     *  watched directories. */
    bool auto_add = true;
    /** Whether or not to receive events about directories. */
    bool watch_dirs = false;

    /** Handle an event. */
    FSEvent *handleEvent(struct inotify_event *ev);

public:
    /**
     * Initialize inotify file descriptor.
     */
    FSWatcher();

    ~FSWatcher();

    /** Add a single file.
     *
     * Attempting to add a file twice will result in the second call
     * failing silently.
     *
     * @param path Path to file.
     */
    void add(std::string path);

    /** Remove a single file.
     *
     * Trying to remove a file that isn't being watched will fail silently.
     * 
     * @param path Path to file.
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

    /** Add files from a directory.
     * 
     * Add all files in a directory and the directory itself. (Does not add sub-directories.)
     * Files created in the directory after the call are automatically added.
     *
     * @param path Path to directory.
     * @return Vector of files that were added.
     */
    std::vector<FSEvent> *addFrom(std::string path);

    /** Remove a directory and the files within.
     *
     * @param path Path to directory.
     */
    void removeFrom(std::string path);

    /** Watch the added files. */
    [[deprecated]]
    void watch();

    /** Watch the added files using a given callback.
     *
     * @param callback Callback for handling file system events, returning
     *                 false from the handler stops watch().
     */
    void watch(const std::function<bool(FSEvent &ev)> &callback);

    /**
     * Spawn a thread watching over the added files, this
     * new thread calls watch() with the provided callback.
     *
     * The thread will be detached, stop the thread by calling
     * stop() on the FSWatcher instance, or by returning false
     * from the callback.
     *
     * @param callback Passed to watch()
     */
    inline void begin(const FSWatchFn &callback) {
        if (running)
            throw std::logic_error("Have already begun watching.");

        std::thread t0([&]() {
                            watch(callback);
                       });
        t0.detach();

        // Wait for the watch() to begin.
        while (!running)
            usleep(10);
    }

    /** Stop watching.
     * 
     * This call has no effect if watch() was
     * not called beforehand.
     */
    inline void stop() {
        running = false;
    }

    /** Check if the watcher is running.
     */
    inline bool isRunning() {
        return running;
    }

    /** Set whether or not to receive events about directories */
    inline void setWatchDirs(bool w) {
        this->watch_dirs = w;
    }

    /** Set whether or not to automatically add new files to the
     *  watch list */
    inline void setAutoAdd(bool auto_add) {
        this->auto_add = auto_add;
    }

    /** Get events.
     *
     * This function is thread safe for a single reader, race-conditions
     * occur with multiple readers.
     *
     * It is an error to use getEvents() while using watch(callback),
     * it is only intended to be used when calling watch() without any
     * arguments.
     */
    [[deprecated]]
    std::vector<FSEvent *> getEvents();
};
