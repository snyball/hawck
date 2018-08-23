#include <memory>
#include <sstream>
#include <regex>

extern "C" {
    #include <unistd.h>
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
 *
 * Warning: This function will abort() if the original directory
 * ceases to exist before the destructor is called.
 * If you want to handle this yourself, use ChDir::popd() instead
 * of relying on the destructor.
 */
class ChDir {
private:
    std::string olddir;
    bool done = false;

public:
    explicit ChDir(const std::string& path) {
        char *cur_path = realpath(".", nullptr);
        if (cur_path == nullptr)
            throw SystemError("Unable to get current directory");
        olddir = std::string(cur_path);
        free(cur_path);
        if (chdir(path.c_str()) == -1)
            throw SystemError("Unable to chdir() to: " + path);
    }

    /**
     * Go back to the original directory, after this call
     * ~ChDir() becomes a no-op.
     */
    void popd() {
        done = true;

        if (chdir(olddir.c_str()) == -1)
            throw SystemError("Unable to chdir() back to: " + olddir);
    }

    ~ChDir() {
        if (done)
            return;

        if (chdir(olddir.c_str()) == 0)
            return;

        std::cout << "chdir() back to original directory failed, aborting." << std::endl;
        abort();
    }
};

inline std::string pathBasename(const std::string& path) {
    const char *bn = basename(path.c_str());
    if (bn == nullptr)
        return std::string("");
    return std::string(bn);
}
