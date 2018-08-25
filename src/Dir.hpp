#include <string>
#include <memory>

extern "C" {
    #include <dirent.h>
}

#include "SystemError.hpp"
#include "utils.hpp"

using FileEntry = std::tuple<std::string, struct stat>;

class Dir
    // : public std::iterator<std::forward_iterator_tag, FileEntry>
{
private:
    DIR *dirp;
    std::string path;

public:
    explicit inline Dir(const std::string& path);

    ~Dir() noexcept(false);

    std::vector<FileEntry> readAll();
};
