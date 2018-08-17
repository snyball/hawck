#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <vector>

extern "C" {
    #include <unistd.h>
    #include <fcntl.h>
    #include <dirent.h>
    #include <linux/input.h>
    #include <linux/uinput.h>
    #include <sys/stat.h>
    #include <stdlib.h>
}

#include "SystemError.hpp"
#include "utils.hpp"

using namespace std;

vector<string> *getLinksTo(const string& target_rel, const string& dirpath) {
    using VecT = unique_ptr<vector<string>>;
    DIR *dir = opendir(dirpath.c_str());
    if (dir == nullptr)
        throw SystemError("Unable to open directory: ", errno);
    char *target_real_c = realpath(target_rel.c_str(), nullptr);
    if (target_real_c == nullptr)
        throw SystemError("Failure in realpath(): ", errno);
    string target(target_real_c);
    free(target_real_c);

    VecT vec = VecT(new vector<string>);
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        string path = dirpath + "/" + string(entry->d_name);
        struct stat stbuf;
        // Use lstat, as it won't silently look up symbolic links.
        if (lstat(path.c_str(), &stbuf) == -1)
            throw SystemError("Failure in stat(): ", errno);

        if (S_ISLNK(stbuf.st_mode)) {
            char lnk_rel_c[PATH_MAX];
            ssize_t len = readlink(path.c_str(), lnk_rel_c, sizeof(lnk_rel_c));
            if (len == -1)
                throw SystemError("Failure in readlink(): ", errno);
            string lnk_rel(lnk_rel_c, len);
            ChDir cd(dirpath);
            char *lnk_dst_real = realpath(lnk_rel.c_str(), nullptr);
            if (lnk_dst_real == nullptr)
                throw SystemError("Failure in realpath(): ", errno);
            string lnk_dst(lnk_dst_real);
            free(lnk_dst_real);
            if (target == lnk_dst)
                vec->push_back(string(path));
        }
    }

    return vec.release();
}

static void printLinks(const string& path, const string& dir) {
    string dir_base = pathBasename(dir);
    try {
        auto links = mkuniq(getLinksTo(path, dir));
        for (const auto& lnk : *links)
            cout << "    " << dir_base << ": " << pathBasename(lnk) << endl;
    } catch (const SystemError &e) {
        cout << "    " << dir_base << ": " << "Unable to acquire links: " << e.what();
    }
}

int main(int argc, char *argv[]) {
    char buf[256];
    int c;

    while ((c = getopt(argc, argv, "hv")) != -1)
        switch (c) {
            case 'h':
                cout <<
                    "lsinput:" << endl <<
                    "    List all input devices from /dev/input/event*" << endl <<
                    "    Display their names, ids, and paths." << endl <<
                    "Usage:" << endl <<
                    "    lsinput [-hv]" << endl;
                return EXIT_SUCCESS;
            case 'v':
                printf("lsinput v0.1\n");
                return EXIT_SUCCESS;
        }

    string devdir = "/dev/input";
    DIR *dir = opendir(devdir.c_str());
    if (dir == nullptr) {
        cout << "Unable to open /dev/input directory" << endl;
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        int fd, ret;
        string filename(entry->d_name);
        try {
            if (filename.substr(0, 5) != "event")
                continue;
        } catch (out_of_range &e) {
            continue;
        }
        string path = devdir + "/" + filename;
        fd = open(path.c_str(), O_RDWR | O_CLOEXEC | O_NONBLOCK);
        if (fd < 0)
            continue;

        ret = ioctl(fd, EVIOCGNAME(sizeof(buf)), buf);
        string name(ret > 0 ? buf : "unknown");
        cout << basename(path.c_str()) << ": " << name << endl;

        printLinks(path, "/dev/input/by-path");
        printLinks(path, "/dev/input/by-id");

        close(fd);
    }

    return 0;
}
