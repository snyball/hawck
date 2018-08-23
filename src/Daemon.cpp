extern "C" {
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <sys/stat.h>
}

#include <string>
#include <memory>

#include "SystemError.hpp"
#include "Daemon.hpp"

#include <hawck_config.h>

using namespace std;

static constexpr size_t BD_MAX_CLOSE = 8192;

void dup_streams(const string &stdout_path, const string &stderr_path) {
    using CFile = unique_ptr<FILE, decltype(&fclose)>;
    auto fopen = [](string path, string mode) -> CFile {
                     return CFile(::fopen(path.c_str(), mode.c_str()), fclose);
                 };

    // Attempt to open /dev/null
    auto dev_null = fopen("/dev/null", "r");
    if (dev_null == nullptr) {
        abort();
    }

    // Open new output files
    auto stdout_new = fopen(stdout_path, "wa");
    auto stderr_new = fopen(stderr_path, "wa");
    if (stdout_new == nullptr || stderr_new == nullptr) {
        throw invalid_argument("Unable to open new stdout and stderr");
    }

    // Dup files
    ::dup2(dev_null->_fileno, STDIN_FILENO);
    ::dup2(stdout_new->_fileno, STDOUT_FILENO);
    ::dup2(stderr_new->_fileno, STDERR_FILENO);
}

/**
 * Fork, becoming sub-process.
 *
 * @throws SystemError If the fork() call fails.
 */
inline void forkThrough() {
    switch (fork()) {
        case -1: throw SystemError("Unable to fork()");
        case 0: break;
        default: exit(0);
    }
}

/*
 * Adapted to C++ from Michael Kerrisks TLPI book.
 */
void daemonize(const string &logfile_path) {
    forkThrough();

    if (setsid() == -1)
        throw SystemError("Unable to setsid(): ", errno);
    
    forkThrough();

    umask(0);
    chdir("/");

    #if REDIRECT_STD_STREAMS
    // Close all files
    int maxfd = sysconf(_SC_OPEN_MAX);
    maxfd = (maxfd == -1) ? BD_MAX_CLOSE : maxfd;
    for (int fd = 3; fd < maxfd; fd++)
        close(fd);

    dup_streams(logfile_path, logfile_path);
    #endif
}

void daemonize() {
    daemonize("/dev/null");
}
