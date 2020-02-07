#pragma once

/** @file utils.hpp
 *
 * @brief Miscellaneous utilities used throughout hawck.
 */

#include <memory>
#include <sstream>
#include <regex>

extern "C" {
    #include <fcntl.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <syslog.h>
    #include <unistd.h>
    #include <sys/file.h>
}

#include "SystemError.hpp"

/**
 * Create a unique_ptr<T> value.
 *
 * C++ cannot deduce the type here:
 *   std::unique_ptr<Deduced type> ptr(p);
 * But it can deduce the type if it is made into a function:
 *   auto ptr = mkuniq(p);
 * This often makes the code look a bit cleaner.
 */
template <class T>
inline std::unique_ptr<T> mkuniq(T *p) {
    return std::unique_ptr<T>(p);
}

template <class T, class F>
inline std::unique_ptr<T, F> mkuniq(T *p, F fn) {
    return std::unique_ptr<T, decltype(fn)>(p, fn);
}

template <class T>
inline std::shared_ptr<T> mkshr(T *p) {
    return std::shared_ptr<T>(p);
}

template <class T>
inline std::shared_ptr<T> mkshr(T *p, const std::function<void(T *p)>& fn) {
    return std::shared_ptr<T>(p, fn);
}

/**
 * stringstream is too much typing, use sstream (like the header)
 * instead.
 */
using sstream = std::stringstream;

/**
 * Change directory in RAII fashion.
 */
class ChDir {
private:
    std::string olddir;
    bool done = false;

public:
    /**
     * chdir() to the given directory.
     *
     * @param path Path to directory.
     *
     * @throws SystemError If unable to chdir() to the path.
     */
    explicit ChDir(const std::string& path) {
        auto cur_path = mkuniq(realpath(".", nullptr), free);
        if (cur_path == nullptr)
            throw SystemError("Unable to get current directory");
        olddir = std::string(cur_path.get());
        if (chdir(path.c_str()) == -1)
            throw SystemError("Unable to chdir() to: " + path);
    }

    /**
     * Go back to the original directory, after this call
     * ~ChDir() becomes a no-op.
     *
     * @throws SystemError If unable to chdir() back to the original
     *                     directory.
     */
    void popd() {
        done = true;

        if (chdir(olddir.c_str()) == -1)
            throw SystemError("Unable to chdir() back to: " + olddir);
    }

    /**
     * Go back to the original directory.
     *
     * If it can't go back to the original directory, it will attempt
     * to move into root (/). If it can't move into the system root,
     * the program will call abort().
     *
     * Errors are logged with syslog.
     */
    ~ChDir() {
        if (done || chdir(olddir.c_str()) != -1)
            return;

        syslog(LOG_ALERT, "Unable to chdir() back to \"%s\": %s",
               olddir.c_str(), SystemError::getErrorString().c_str());

        if (chdir("/") != -1) {
            syslog(LOG_WARNING, "Moved into system root due to chdir() failure.");
            return;
        }

        syslog(LOG_EMERG, "Unable to move into system root: %s",
               SystemError::getErrorString().c_str());
        syslog(LOG_EMERG, "Will now abort!");
        abort();
    }
};

/**
 * Flocka; I go hard in the paint, but only when I've synchronized access to the
 * file with the flock() system call.
 *
 * Usage:
 * {
 *   int thing;
 *   {
 *     Flocka flock("file.txt");
 *     ifstream stream("file.txt");
 *     stream >> thing;
 *   }
 * }
 */
class Flocka {
    int fd;

public:
    /**
     * Throws SystemError if the file cannot be opened.
     */
    inline Flocka(std::string path) noexcept(false) {
        fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd == -1) {
            throw SystemError("Unable to open file: ", errno);
        }

        if (flock(fd, LOCK_SH) == -1) {
            throw SystemError("Unable to acquire shared lock: ", errno);
        }

        if (flock(fd, LOCK_EX) == -1) {
            throw SystemError("Unable to acquire exclusive lock: ", errno);
        }
    }

    inline ~Flocka() {
        // Did not find any failure conditions for LOCK_UN in the documentation.
        flock(fd, LOCK_UN);
        close(fd);
    }
};

/**
 * Wrapper around the POSIX ::basename(char *) function.
 * 
 * @param path The path to get the basename of.
 * @return Result of basename(path).
 */
inline std::string pathBasename(const std::string& path) {
    const char *full = path.c_str();
    // Basename will return a pointer to somewhere inside `full`
    // We can safely cast it to (char *) as long as we make the
    // return value const.
    const char *bn = basename((char *) full);
    if (bn == nullptr)
        return std::string("");
    return std::string(bn);
}

inline std::string realpath_safe(const std::string& path) {
    auto rpath_chars = std::unique_ptr<char, decltype(&free)>(realpath(path.c_str(),
                                                                       nullptr),
                                                              &free);
    return std::string(rpath_chars.get());
}
