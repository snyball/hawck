/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * FSWatcher.hpp, file system monitoring.                                            *
 *                                                                                   *
 * Copyright (C) 2018 Jonas Møller (no) <jonas.moeller2@protonmail.com>                       *
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
 * @brief File system watcher
 *
 * Exposes the OS file watching API.
 *
 * Currently only the inotify API from Linux is supported.
 */

extern "C" {
    #include <limits.h>
    #include <sys/stat.h>
    #include <unistd.h>
}

#if __linux
    extern "C" {
        #include <sys/inotify.h>
    }
    using OSEvent = struct inotify_event;
    using OSAPIHandle = int;
    using OSFileStat = struct stat;
#else
    #error "This class currently only supports the Linux inotify API."
#endif

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

/** Time before runtime_error is thrown in stop() if it cannot stop
 *  the thread. */
static constexpr int FSW_THREAD_STOP_TIMEOUT_SEC = 15;

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
    /** True if this event was sent as a result of file deletion. */
    bool deleted = false;

    /** Initialize an FSEvent from an inotify event */
    FSEvent(struct inotify_event *ev, std::string path);

    /** Initialize an FSEvent from an absolute path, assumed to
     *  be an `added` event. */
    explicit FSEvent(std::string path);
};

using FSWatchFn = std::function<bool(FSEvent &ev)>;

enum RunState {
    STOPPED,
    RUNNING,
    STOPPING,
};

/** File system watcher.
 *
 * Uses the Linux inotify API to listen for file system
 * events.
 *
 * Example:
 *
 *   FSWatcher fsw;
 *   fsw.addFrom("/dev/input/")
 *   fsw.begin([](FSEvent ev) {
 *       if (ev.path == my_special_path) {
 *           doSpecialThing(ev);
 *           // Stop running.
 *           return false;
 *       } else if (S_ISDIR(ev.stbuf))
 *           doDirectoryThing(ev);
 *       // Keep running
 *       return true;
 *   })
 *   ...
 *   fsw.stop();
 *   ...
 *   // Will resume where the last handler quit.
 *   fsw.begin([](FSEvent ev) {
 *       ...
 *   })
 *   fsw.stop();
 *   ...
 *   fsw.begin([](FSEvent ev) {
 *       // Bad idea:
 *       sleep(100);
 *       // Other bad idea:
 *       while (never_false)
 *           ;
 *   })
 *   
 *   } // fsw now out of scope
 *   // This leads to an abort() as FSWatcher::stop() is unable
 *   // to stop the last handler in time.
 */
class FSWatcher {
private:
    /** Inotify main file descriptor. */
    OSAPIHandle fd;
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
    /** Set to RUNNING when \link FSWatcher::watch() \endlink is called,
     *  is set to STOPPED by calling \link FSWatcher::stop() \endlink */
    std::atomic<RunState> running = RunState::STOPPED;
    /** Whether or not to automatically add files that are created in
     *  watched directories. */
    bool auto_add = true;
    /** Whether or not to receive events about directories. */
    bool watch_dirs = false;
    /** Used for backing up the number of unread evbuf elements
     *  present when a callback function terminated, the watcher
     *  can then be restarted without missing any events.
     *  XXX: I'm guessing the kernel will eventually start to drop
     *       events if there are too many unread ones. So if you don't
     *       want to miss out then you should probably not leave the
     *       FSW instance hanging around for too long. */
    size_t backup_num_read = 0;

    static std::atomic<int> num_instances;

    /** Handle an event. */
    FSEvent *handleEvent(struct inotify_event *ev);

public:
    /** Initialize inotify file descriptor.
     */
    FSWatcher();

    /** FSWatcher destructor, stops the currently running thread.
     *
     * If we time out on stopping the thread a runtime_exception
     * will be thrown, this can happen if your callback ran into
     * an infinite loop, has gotten stuck in a syscall, or is doing
     * something it really shouldn't be doing like sleep()ing.
     *
     * @see FSW_THREAD_STOP_TIMEOUT_SEC For the actual amount of seconds
     *                                  you should expect.
     */
    ~FSWatcher() noexcept(false);

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

    /** Add an entire directory tree, this only adds the directories.
     */
    void addTree();

    /** Adds all files in a directory tree, this does not add any
     *  directories.
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

    /** Spawn a thread watching over the added files, this
     *  new thread calls watch() with the provided callback.
     *
     * The thread will be detached, stop the thread by calling
     * stop() on the FSWatcher instance, or by returning false
     * from the callback.
     *
     * This function can throw std::runtime_error, however these
     * errors should not be caught.
     *
     * Note: begin can only be called once, unless you call stop
     *       first.
     *
     * @param callback Passed to watch()
     */
    inline void asyncWatch(const FSWatchFn &callback) {
        if (running == RunState::RUNNING)
            throw std::runtime_error("Have already begun watching.");

        running = RunState::RUNNING;
        std::thread t0([callback, this]() {this->watch(callback);});
        t0.detach();
    }

    /** Watch the added files using a given callback.
     *
     * @param callback Callback for handling file system events, returning
     *                 false from the handler stops watch().
     */
    void watch(const FSWatchFn &callback);

    /** Stop watching.
     *
     * Note: This function will call runtime_error if it times out
     *       on stopping your handler function.
     * 
     * @see FSW_THREAD_STOP_TIMEOUT_SEC For the actual amount of seconds
     *                                  you should expect.
     */
    void stop();

    /** Check if the watcher is running.
     */
    inline bool isRunning() {
        return running == RunState::RUNNING;
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

    int getMaxWatchers() const;
};


