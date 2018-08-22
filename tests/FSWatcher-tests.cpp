#include <iostream>

#include <catch2/catch.hpp>
#include "FSWatcher.hpp"
#include "utils.hpp"

extern "C" {
    #include <unistd.h>
}

using namespace std;

static inline FSWatchFn getTestFn(vector<string>& paths,
                                  string *err,
                                  atomic<size_t> &idx) {
    return [&](FSEvent &ev) {
               REQUIRE(idx < paths.size());
               auto cur_path = paths[idx++];

               // Test case mismatch.
               if (ev.path != cur_path) {
                   sstream ss;
                   ss << "Got " << ev.path << " but expected " << cur_path;
                   *err = ss.str();
                   return false;
               }

               // Test cases done.
               if (idx == paths.size())
                   return false;

               return true;
           };
}

static inline void system_s(string cmd) {
    int ret = system(cmd.c_str());
    if (ret != 0) {
        sstream ss;
        ss << "Command `" << cmd << "` exited with " << ret;
        throw SystemError(ss.str());
    }
}

static inline void runTestsCMD(FSWatcher *w, vector<string>& paths, vector<string> cmds) {
    const int MAX_SLEEPS = 500;
    const int SLEEP_TIME = 10000;
    int sleeps = 0;
    string err = "";
    atomic<size_t> idx = 0;
    auto fn = getTestFn(paths, &err, idx);
    w->begin(fn);

    for (string c : cmds)
        system_s(c);

    while (w->isRunning()) {
        if (sleeps++ > MAX_SLEEPS) {
            w->stop();
	    while (w->isRunning()) continue;
            FAIL("Timeout");
        }
        usleep(SLEEP_TIME);
    }
    if (err != "") {
        FAIL(err);
    }
}

static inline vector<string> mkTestFiles(int num_files, bool create) {
    system("rm -r /tmp/hwk-tests");
    system_s("mkdir -p /tmp/hwk-tests");

    vector<string> paths;
    for (int i = 0; i < num_files; i++) {
        sstream ss;
        ss << "/tmp/hwk-tests/" << i << ".txt.test";
        paths.push_back(ss.str());
    }

    if (create)
        for (const string& path : paths)
            system_s("touch '" + path + "'");

    return paths;
}

static inline vector<string> mkModCMDs(vector<string> &paths) {
    vector<string> cmds;
    for (const string& path : paths)
        cmds.push_back("echo 'test line' >> '" + path + "'");
    return cmds;
}

static inline vector<string> mkCreateCMDs(vector<string> &paths) {
    vector<string> cmds;
    for (const string& path : paths)
        cmds.push_back("touch '" + path + "'");
    return cmds;
}

static inline vector<string> mkRmCMDs(vector<string> &paths) {
    vector<string> cmds;
    for (const string& path : paths)
        cmds.push_back("rm '" + path + "'");
    return cmds;
}

// Test whether or not we receive file change notifications
// for a single file.
#if 1
TEST_CASE("Explicitly watching files, not directories.", "[FSWatcher]") {
    vector<string> paths = mkTestFiles(30, true);
    FSWatcher watcher;
    for (auto path : paths)
        watcher.add(path);
    vector<string> cmds = mkModCMDs(paths);
    runTestsCMD(&watcher, paths, cmds);
}
#endif

// Test whether or not new files are being watched.
TEST_CASE("Directory file addition", "[FSWatcher]") {
    vector<string> paths = mkTestFiles(30, false);
    FSWatcher watcher;
    watcher.add("/tmp/hwk-tests");
    vector<string> cmds = mkCreateCMDs(paths);
    vector<string> cmds_mod = mkModCMDs(paths);
    vector<string> cmds_rm = mkRmCMDs(paths);
    for (string cmd : cmds_mod)
        cmds.push_back(cmd);
    for (string cmd : cmds_rm)
        cmds.push_back(cmd);
    // Cmds: Create files, then modify files, then delete files.

    // Expect three times paths.
    vector<string> paths_cpy = paths;
    for (auto path : paths_cpy)
        paths.push_back(path);
    for (auto path : paths_cpy)
        paths.push_back(path);

    runTestsCMD(&watcher, paths, cmds);
}

// Test FSWatcher::addFrom
#if 1
TEST_CASE("Watch whole directory", "[FSWatcher]") {
    int num_tests = 30;
    vector<string> paths = mkTestFiles(num_tests, true);
    FSWatcher watcher;
    vector<FSEvent> *added = watcher.addFrom("/tmp/hwk-tests");
    REQUIRE(added->size() == num_tests);
    delete added;
    vector<string> cmds = mkModCMDs(paths);
    runTestsCMD(&watcher, paths, cmds);
}
#endif
