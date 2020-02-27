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
    #include <limits.h>
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


/**
 * Change directory in RAII fashion.
 *
 * NOTE: Generally it is best to just always use absolute paths, however there
 * will always be the occasional library, or the occasional use-case for
 * changing the working directory and operating from there.
 *
 * ChDir lets you work with the PWD similar to how `pushd/popd` work in a shell,
 * except you usually never have to bother with the `popd`.
 *
 * Review each usage of ChDir carefully to see if calling `popd` and handling
 * errors manually is worthwhile.
 *
 * If you can, run your programs from paths like "/" and "$HOME", if they ever
 * disappear suddenly then you have bigger problems.
 */
class ChDir {
private:
    std::string olddir;
    int fd;
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
        if ((fd = open(".", O_RDONLY)) == -1)
            throw SystemError("Unable to open current directory");
        if (chdir(path.c_str()) == -1) {
            close(fd);
            throw SystemError("Unable to chdir() to: " + path);
        }
    }

    /**
     * Go back to the original directory, after this call
     * ~ChDir() becomes a no-op.
     *
     * @throws SystemError If unable to chdir() back to the original
     *                     directory.
     */
    void popd() {
        if (done) return;
        done = true;

        for (int i = 0; i < 1000; i++) {
            if (fchdir(fd) != -1) {
                close(fd);
                return;
            }

            switch (errno) {
                case EIO:
                case EINTR:
                    break;

                default:
                    goto loop_end;
            }
        } loop_end:

        throw SystemError("Unable to chdir() back to '" + olddir + "': ", errno);
    }

    /**
     * Go back to the original directory, errors result in abort() here.
     */
    ~ChDir() {
        try {
            popd();
        } catch (const SystemError &e) {
            e.printBacktrace();
            syslog(LOG_CRIT, "Aborting due to: %s", e.what());
            abort();
        }
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
    inline Flocka(const std::string& path) noexcept(false) {
        fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
        if (fd == -1) {
            throw SystemError("Unable to open file: ", errno);
        }

        if (flock(fd, LOCK_SH) == -1) {
            close(fd);
            throw SystemError("Unable to acquire shared lock: ", errno);
        }

        if (flock(fd, LOCK_EX) == -1) {
            close(fd);
            throw SystemError("Unable to acquire exclusive lock: ", errno);
        }
    }

    inline ~Flocka() {
        // Did not find any failure conditions for LOCK_UN in the documentation.
        flock(fd, LOCK_UN);
        close(fd);
    }
};

inline std::string realpath_safe(const std::string& path) {
    char *rpath_raw = realpath(path.c_str(), nullptr);
    if (rpath_raw == nullptr)
        return "";
    auto rpath_chars = std::unique_ptr<char, decltype(&free)>(rpath_raw, &free);
    return std::string(rpath_chars.get());
}

inline std::string readlink(const std::string& path) {
    char link_path[PATH_MAX];
    if (readlink(path.c_str(), link_path, sizeof(link_path)) == -1) {
        throw SystemError("Unable to read link: ", errno);
    }
    return std::string(link_path);
}

class StringJoiner {
private:
    std::string sep;

public:
    inline StringJoiner(std::string sep) : sep(sep) {}

    template <class... Ts>
    inline std::string join(Ts... parts) {
        std::stringstream sstream;
        return _join(&sstream, parts...);
    }

private:
    template <class T>
    inline std::string _join(std::stringstream *stream, T arg) {
        (*stream) << arg;
        return stream->str();
    }

    template <class T, class... Ts>
    inline std::string _join(std::stringstream *stream, T arg, Ts... rest) {
        (*stream) << arg << sep;
        return _join(stream, rest...);
    }
};

template <class... Ts>
static inline std::string pathJoin(Ts... parts) {
    StringJoiner joiner("/");
    return joiner.join(parts...);
}

/**
 * Check if string `a` ends with string `b`.
 * NOTE: This is a strict ends with, so it will return false if a == b
 * TODO: Remove this when C++20 lands.
 */
static inline bool stringEndsWith(const std::string& a, const std::string& b) {
    return a.size() > b.size() && a.substr(a.size() - b.size()) == b;
}

/**
 * Check if string `a` starts with string `b`.
 * NOTE: This is a strict starts with, so it will return false if a == b.
 * TODO: Remove this when C++20 lands.
 */
static inline bool stringStartsWith(const std::string& a, const std::string& b) {
    return a.size() > b.size() && a.substr(0, b.size()) == b;
}


template <int num> constexpr int countT() {
    return num;
}

template <int num, class, class... T> constexpr int countT() {
    return countT<num+1, T...>();
}

template <class... T> constexpr int countT() {
    return countT<0, T...>();
}
