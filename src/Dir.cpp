#include "Dir.hpp"

extern "C" {
    #include <sys/stat.h>
}

using namespace std;

Dir::Dir(const std::string& path) {
    auto dirp = mkuniq(opendir(path.c_str()), &closedir);
    if (dirp == nullptr)
        throw SystemError("Unable to opendir(" + path + "): ", errno);
    char *rpath = realpath(path.c_str(), nullptr);
    if (rpath == nullptr)
        throw SystemError("Unable to realpath(): ", errno);
    this->path = string(rpath);
    free(rpath);
    this->dirp = dirp.release();
}

Dir::~Dir() noexcept(false) {
    if (closedir(dirp) == -1) {
        throw SystemError("Unable to closedir(): ", errno);
    }
}

std::vector<FileEntry> Dir::readAll() {
    errno = 0;
    std::vector<FileEntry> vec;
    struct dirent *entry;
    while ((entry = readdir(this->dirp)) != nullptr) {
        string path = this->path + string(entry->d_name);
        struct stat stbuf;
        if (stat(path.c_str(), &stbuf) == -1)
            throw SystemError("Error in stat(): ", errno);
        vec.push_back(make_tuple(path, stbuf));
        errno = 0;
    }
    if (errno != 0)
        throw SystemError("Error in readdir(): ", errno);
    return vec;
}
