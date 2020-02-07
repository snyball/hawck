/** @file Daemon.hpp
 *
 * @brief Daemonization utilities.
 */

#include <string>

/**
 * Change stdout and stderr to point to different files, specified
 * by the given paths.
 *
 * Stdin will always be redirected from /dev/null, if /dev/null cannot
 * be open this will result in abort(). Note that after this call any
 * blocking reads on stdin will halt the program.
 *
 * @param stdout_path File to redirect stdout into.
 * @param stderr_path File to redirect stderr into.
 */
void dup_streams(const std::string &stdout_path,
                 const std::string &stderr_path);

/** Turn the calling process into a daemon.
 */
void daemonize();

/** Turn the calling process into a daemon.
 *
 * @param logfile_path The logfile to redirect stderr and stdout
 *                     into.
 */
void daemonize(const std::string &logfile_path);

/**
 * Kill process according to pid_file.
 *
 * @param pid_file Path to a file where the pid of each new daemon is stored.
 *
 * When `killPretender` is finished it will write `getpid()` to the pid file.
 *
 * The process corresponding to the pid in the pid file *must* have the same
 * basename exe as the process calling `killPretender`.
 */
void killPretender(std::string pid_file);

/**
 * Clear pid file, but only if it contains the pid for the process calling
 * `cleanPidFile`.
 *
 * @param pid_file Path to a file where the pid of each new daemon is stored.
 */
void clearPidFile(std::string pid_file);
