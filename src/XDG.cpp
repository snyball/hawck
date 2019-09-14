#include <sstream>
#include <algorithm>
#include <iostream>
#include <string>

#include "XDG.hpp"
#include "Permissions.hpp"

using namespace std;

XDG::XDG() noexcept(false) {
    auto [pw, pwbuf] = Permissions::getuser();
    (void) pwbuf;
    string user_name(pw->pw_name);

    auto user_home_fallback   = "/home/" + user_name;
    auto user_home_var        = envString("HOME", user_home_fallback);
    auto config_home_fallback = user_home_var + "/.config";
    auto config_home_var      = envString("XDG_CONFIG_HOME", config_home_fallback);
    auto data_home_fallback   = user_home_var + "/.local/share";
    auto data_home_var        = envString("XDG_DATA_HOME", data_home_fallback);
    auto cache_home_fallback  = user_home_var + "/.cache";
    auto cache_home_var       = envString("XDG_CACHE_HOME", cache_home_fallback);
    auto runtime_dir_var      = envString("XDG_RUNTIME_DIR");

    // These file-permissions are based on the defaults in Ubuntu 18.04
    mkPathIfNotExists(user_home_var, 0755);
    mkPathIfNotExists(data_home_var, 0700);
    mkPathIfNotExists(cache_home_var, 0775);

    if (runtime_dir_var == "") {
        struct stat stbuf;
        if (stat(data_home_var.c_str(), &stbuf) == -1)
            throw SystemError("Unable to stat() $XDG_DATA_HOME: ", errno);

        // Fall back to DATA_HOME for RUNTIME_DIR if the directory has suitable
        // permissions.
        if ((stbuf.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO)) == 0700) {
            runtime_dir_var = data_home_var;
        } else {
            // If all else fails, try to create a fallback directory in $HOME
            runtime_dir_var = user_home_var + "/.xdg-runtime-dir";
            mkPathIfNotExists(runtime_dir_var, 0700);
        }
    }

    dirs["CONFIG_HOME"] = config_home_var;
    dirs["DATA_HOME"]   = data_home_var;
    dirs["CACHE_HOME"]  = cache_home_var;
    dirs["RUNTIME_DIR"] = runtime_dir_var;
}

XDG::~XDG() {}

int XDG::mkPathIfNotExists(const std::string& path, mode_t um) {
    if (path.size() == 0)
        return 0;

    int created = 0;
    bool rel = path[0] != '/';
    stringstream ss(path);
    stringstream out;
    string tok;

    // Why in the name of Christ doesn't the C++ standard library include
    // something as basic as std::string::split?
    // You have to use getline on a stringstream instead SMH.
    while (getline(ss, tok, '/')) {
        if (rel && out.str().size() == 0)
            out << tok;
        else if (tok != "")
            out << "/" << tok;
        if (out.str().size() > 0)
            created += mkdirIfNotExists(out.str(), um);
    }
    return created;
}

bool XDG::mkdirIfNotExists(const std::string& dir, mode_t um) {
    struct stat stbuf;
    if (stat(dir.c_str(), &stbuf) == -1) {
        switch (errno) {
            case ENOENT:
                // Recovery possible, by creating the directory.
                syslog(LOG_WARNING, "Directory did not exist, creating it with mask %s: %s",
                       Permissions::fmtPermissions(um).c_str(), dir.c_str());
                if (mkdir(dir.c_str(), um) == -1)
                    // Either we don't have write-access to the directory, or
                    // the filesystem is corrupted, either way there's no
                    // recovery here.
                    throw SystemError("Unable to create directory: ", errno);
                return true;

            default:
                // No way to recover, either the directory was malformed, or
                // there is some other system misconfiguration.
                throw SystemError("Unable to stat, but no ENOENT: ", errno);
        }
    }

    // No way to recover from this, misconfigured system. We can't go around
    // deleting files so we throw a SystemError.
    if (!S_ISDIR(stbuf.st_mode))
        throw SystemError("Required to be a directory: " + dir);

    return false;
}

bool XDG::isDir(const std::string &path) {
    struct stat stbuf;
    if (stat(path.c_str(), &stbuf) == -1)
        return false;
    return S_ISDIR(stbuf.st_mode);
}
