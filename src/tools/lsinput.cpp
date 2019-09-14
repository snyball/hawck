/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * lsinput, list input devices from /dev/input/event                                 *
 *                                                                                   *
 * Copyright (C) 2018 Jonas Møller (no) <jonas.moeller2@protonmail.com>              *
 * All rights reserved.                                                              *
 *                                                                                   *
 * Redistribution and use in source and binary forms, with or without                *
 * modification, are permitted provided that the following conditions are met:       *
 *                                                                                   *
 * 1. Redistributions of source code must retain the above copyright notice, this    *
 *    list of conditions and the following disclaimer.                               *
 * 2. Redistributions in binary form must reproduce the above copyright notice,      *
 *    this list of conditions and the following disclaimer in the documentation      *
 *    and/or other materials provided with the distribution.                         *
 *                                                                                   *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND   *
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED     *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE            *
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE      *
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL        *
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR        *
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER        *
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,     *
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE     *
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/** @file */

#include <string>
#include <sstream>
#include <iostream>
#include <memory>
#include <vector>
#include <functional>

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

/**
 * Find symbolic links to a target inode from within a directory.
 *
 * @param target_rel Path to the target.
 * @param dirpath Path to the directory to search for links in.
 * @return Pointer to vector of paths to symbolic links that
 *         reference `target_rel`. Must be deleted by the caller.
 */
vector<string> *getLinksTo(const string& target_rel, const string& dirpath) {
    using VecT = unique_ptr<vector<string>>;
    char *target_real_c = realpath(target_rel.c_str(), nullptr);
    if (target_real_c == nullptr)
        throw SystemError("Failure in realpath(): ", errno);
    string target(target_real_c);
    free(target_real_c);
    // DIR *dir = opendir(dirpath.c_str());
    auto dir = shared_ptr<DIR>(opendir(dirpath.c_str()), [](DIR *d) {
                                                             closedir(d);
                                                         });
    if (dir == nullptr)
        throw SystemError("Unable to open directory: ", errno);

    VecT vec = VecT(new vector<string>);

    struct dirent *entry;
    while ((entry = readdir(dir.get()))) {
        string path = dirpath + "/" + string(entry->d_name);
        struct stat stbuf;
        // Use lstat, as it won't silently look up symbolic links.
        if (lstat(path.c_str(), &stbuf) == -1)
            throw SystemError("Failure in stat(): ", errno);

        if (S_ISLNK(stbuf.st_mode)) {
            char lnk_rel_c[PATH_MAX];
            // Get link contents
            ssize_t len = readlink(path.c_str(), lnk_rel_c, sizeof(lnk_rel_c));
            if (len == -1)
                throw SystemError("Failure in readlink(): ", errno);
            string lnk_rel(lnk_rel_c, len);
            // lnk_rel path may only be valid from within the directory.
            ChDir cd(dirpath);
            auto lnk_dst_real = mkuniq(realpath(lnk_rel.c_str(), nullptr), &free);
            cd.popd(); // May throw SystemError
            if (lnk_dst_real == nullptr)
                throw SystemError("Failure in realpath(): ", errno);
            char *lnk_dst_real_p = lnk_dst_real.release();
            if (!strcmp(lnk_dst_real_p, target.c_str()))
                vec->push_back(string(path));
            free(lnk_dst_real_p);
        }
    }

    return vec.release();
}

using PrintLinkFn = function<void(const string& dir_base,
                                 const string& lnk)>;

static inline void printLinks(const string& path,
                              const string& dir,
                              const PrintLinkFn& fn)
{
    string dir_base = pathBasename(dir);
    try {
        auto links = mkuniq(getLinksTo(path, dir));
        for (const auto& lnk : *links)
            fn(dir_base, lnk);
            // cout << indent << dir_base << ": " << pathBasename(lnk) << sep;
    } catch (const SystemError &e) {
        // cout << "    " << dir_base << ": " << "Unable to acquire links: " << e.what();
    }
}

int main(int argc, char *argv[]) {
    char buf[256];
    int c;
    bool small = false;

    while ((c = getopt(argc, argv, "hvs")) != -1)
        switch (c) {
            case 'h':
                cout <<
                    "Usage: lsinput [-hvs]\n"
                    "\n"
                    "Options:\n"
                    "  -h    Display this help info.\n"
                    "  -v    Display version.\n"
                    "  -s    Print each input device on a single line, easier for\n"
                    "        stream editors like awk and sed to deal with.\n";
                return EXIT_SUCCESS;
            case 'v':
                printf("lsinput v0.1\n");
                return EXIT_SUCCESS;
            case 's':
                small = true;
                break;
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

        PrintLinkFn fn;
        if (small) {
            cout << path << "\t" << "\"" << name << "\"";
            fn = [](const string&, const string& lnk) {
                     cout << "\t" << pathBasename(lnk);
                 };
        } else {
            cout << pathBasename(path.c_str()) << ": " << name << endl;
            fn = [](const string& dir, const string& lnk) {
                     cout << "    " << dir << ": " << pathBasename(lnk) << endl;
                 };
        }

        printLinks(path, "/dev/input/by-path", fn);
        printLinks(path, "/dev/input/by-id", fn);

        if (small)
            cout << endl;

        close(fd);
    }

    closedir(dir);

    return 0;
}
