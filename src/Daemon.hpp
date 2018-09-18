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

