extern "C" {
    #include <unistd.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <sys/stat.h>
    #include <signal.h>
    #include <sys/types.h>
    #include <syslog.h>
    #include <limits.h>
}

#include <string>
#include <fstream>
#include <memory>

#include "SystemError.hpp"
#include "Daemon.hpp"
#include "utils.hpp"

#if MESON_COMPILE
#include <hawck_config.h>
#endif

using namespace std;

static constexpr size_t BD_MAX_CLOSE = 8192;

void dup_streams(const string &stdout_path, const string &stderr_path) {
    using CFile = unique_ptr<FILE, decltype(&fclose)>;
    auto fopen = [](string path, string mode) -> CFile {
                     return CFile(::fopen(path.c_str(), mode.c_str()), fclose);
                 };

    // Attempt to open /dev/null
    auto dev_null = fopen("/dev/null", "r");
    if (dev_null == nullptr)
        abort();

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
 * Fork through, i.e fork and become the subprocess while the original process
 * stops.
 *
 * @throws SystemError If the fork() call fails.
 */
inline void forkThrough() {
    switch (fork()) {
        case -1: throw SystemError("Unable to fork(): ", errno);
        case 0: break;
        default: exit(0);
    }
}

/**
 * Adapted to C++ from Michael Kerrisks TLPI book.
 */
void daemonize(const string &logfile_path) {
    forkThrough();

    if (setsid() == -1)
        throw SystemError("Unable to setsid(): ", errno);

    forkThrough();

    umask(0);

    if (chdir("/") == -1)
        throw SystemError("Unable to chdir(\"/\"): ", errno);

    // Close all files
    int maxfd = sysconf(_SC_OPEN_MAX);
    maxfd = (maxfd == -1) ? BD_MAX_CLOSE : maxfd;
    for (int fd = 0; fd < maxfd; fd++)
        close(fd);

    dup_streams(logfile_path, logfile_path);
}

/**
 * Get path to the executable
 */
static std::string pidexe(pid_t pid) {
    char buf[PATH_MAX];
    // The documentation for readlink wasn't clear on whether it would
    // write an empty string on error.
    memset(buf, '\0', sizeof(buf));
    readlink(pathJoin("/proc", pid, "exe").c_str(), buf, sizeof(buf));
    return std::string(buf);
}

void killPretender(std::string pid_file) {
    Flocka flock(pid_file);
    int old_pid = 0; {
        std::ifstream pidfile(pid_file);
        pidfile >> old_pid;
    }
    std::string old_exe = pidexe(old_pid);
    if (pathBasename(old_exe) == pathBasename(pidexe(getpid()))) {
        syslog(LOG_WARNING, "Killing previous %s instance ...",
               old_exe.c_str());
        kill(old_pid, SIGTERM);
    } else {
        syslog(LOG_INFO, "Outdated pid file with exe '%s', continuing ...",
               old_exe.c_str());
    }
    std::ofstream pidfile_out(pid_file);
    pidfile_out << getpid() << std::endl;
}

void clearPidFile(std::string pid_file) {
    Flocka flock(pid_file);
    int pid; {
        std::ifstream pidfile(pid_file);
        pidfile >> pid;
    }
    if (pid == getpid()) {
        std::ofstream pidfile(pid_file);
        pidfile << std::endl;
    }
}

void daemonize() {
    daemonize("/dev/null");
}
