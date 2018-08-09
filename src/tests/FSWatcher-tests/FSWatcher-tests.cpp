#include <iostream>

#include "../catch.hpp"
#include "../../FSWatcher.hpp"

extern "C" {
    #include <unistd.h>
}

#define BEGIN \
    do {chdir("FSWatcher-tests"); } while (false)

#define RUN(test) \
    do {system("./" #test ".sh");} while (false)

#define END \
    do {system("./" #test "-cleanup.sh");        \
        chdir("..");} while (false)

using namespace std;

static inline tuple<thread*, FSWatcher*, string> watchOn(string path) {
    FSWatcher *watcher = new FSWatcher();
    char *rpath = realpath(path.c_str(), NULL);
    string test_path(rpath);
    free(rpath);
    watcher->add(test_path);
    std::thread *t0 = new std::thread([&]() -> void {
                                          watcher->watch();
                                      });
    return make_tuple(t0, watcher, test_path);
}

static inline void finishWatch(thread *t0, FSWatcher *watch) {
    watch->stop();
    t0->join();
    delete watch;
    delete t0;
}

constexpr int MAX_RUNS = 1000;

static inline void requireEvent(FSWatcher *w, vector<string>& test_path) {
    int runs;
    int i = 0;
    for (runs = 0; runs < MAX_RUNS; runs++) {
        usleep(1000);
        vector<FSEvent *> events = w->getEvents();
        if (events.size() > 0) {
            for (FSEvent *ev : events) {
                REQUIRE(i < test_path.size());
                REQUIRE(ev->path == test_path[i++]);
                delete ev;
            }
            break;
        }
    }
    REQUIRE(runs != MAX_RUNS);
    REQUIRE(i == test_path.size());
}

TEST_CASE("Single file modification", "[FSWatcher]") {
    system("touch './test01.txt'");

    auto [t0, watcher, test_path] = watchOn("./test01.txt");

    system("echo 'test line' >> ./test01.txt");

    vector<string> paths = {test_path};
    requireEvent(watcher, paths);

    finishWatch(t0, watcher);

    remove("test01.txt");
}

TEST_CASE("Directory file addition", "[FSWatcher]") {
    BEGIN;
    auto [t0, watcher, test_path] = watchOn(".");
    system("touch 'test02.txt'");
    system("echo 'test line' >> test02.txt");
    remove("test02.txt");
    string test02_path = test_path + "/test02.txt";
    vector<string> paths = {test02_path, test02_path};
    requireEvent(watcher, paths);
    // requireEvent(watcher, test_path);
    usleep(1000);
    finishWatch(t0, watcher);
}
