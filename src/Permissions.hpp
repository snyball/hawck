#pragma once

#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <sstream>

#include "SystemError.hpp"

extern "C" {
    #include <sys/types.h>
    #include <grp.h>
    #include <sys/stat.h>
    // #include <syslog.h>
    #include <pwd.h>
    #include <unistd.h>
}

namespace Permissions {

/** Format UNIX file permissions to the following format:
 *  rwxrwxrwx <user>:<group>
 *
 * @param stbuf Stat buffer.
 */
std::string fmtPermissions(struct stat& stbuf) noexcept;

/** Format UNIX file permissions to the following format:
 *  rwxrwxrwx
 *
 * @param num Permission bits.
 */
std::string fmtPermissions(unsigned num) noexcept;

struct Group {
    struct group grpbuf;
    char buf[2048];
};

struct Collection {
    mode_t mode;
    mode_t mode_mask;
    std::string user_name;
    uid_t uid;
    std::string group_name;
    gid_t gid;
    std::string file_type_char;

    Collection(const struct stat *stbuf);

    /**
     * The description string has the following format (expressed as a regex):
     *
     *                  <---------------------[PERMISSIONS]------------------------>
     *       <-[TYPE]>   <-----[USER]----->  <----[GROUP]----->  <-----[ALL]------>     <--[USER]--->   <--[GRP]-->
     *      ([dpbclsf])(([r\.-][w\.-][x\.-])([r\.-][w\.-][x\.-])([r\.-][w\.-][x\.-])) +(~|\*|[a\.-z]+):(\*|[a\.-z]+)
     *
     * Type:
     *
     *     (d)irectory
     *     (p)ipe, FIFO
     *     (b)lock
     *     (c)har
     *     (l)ink
     *     (s)ocket
     *     (f)ile
     *
     * Permissions:
     *
     *      (r)ead
     *      (w)rite
     *     e(x)ecute
     *      (.): Anything
     *      (-): Unset
     *
     * User:
     *
     *     ~: User running the program.
     *     *: Any user
     *
     * Group:
     *
     *     *: Any group
     *  
     * Examples:
     *
     *     drwx...--- root:root (A directory owned by root, only rwx by root.)
     *     frwxr-xr-x user:user (An executable/readable file by all, owned and
     *                           writeable by user.)
     *     frw------- ~:*       (A file readable and writeable, but only by the
     *                           user running the program.)
     */
    Collection(const std::string& description);

    inline bool operator==(const Collection& other) noexcept {
        mode_t mmask = mode_mask & other.mode_mask;
        return ((other.mode & mmask) == (mode & mmask) &&
                other.file_type_char == file_type_char &&
                (other.uid == uid || (user_name == "*"  || other.user_name == "*")) &&
                (other.gid == gid || (group_name == "*" || other.group_name == "*")));
    }

    inline std::string fmt() noexcept {
        return file_type_char + fmtPermissions(mode) + " " + user_name + ":" + group_name;
    }
};

inline std::tuple<struct group*, std::shared_ptr<Group>> getgroup(const std::string& name) {
    struct group *grp_ptr;
    auto grp = std::shared_ptr<Group>(new Group());

    if (getgrnam_r(name.c_str(),
                   (struct group*)&grp->grpbuf,
                   (char *)&grp->buf,
                   sizeof(grp->buf),
                   &grp_ptr) != 0)
    {
        throw SystemError("Failure in getgrnam_r(): ", errno);
    }

    if (grp_ptr == nullptr)
        throw SystemError("No such group: " + name);

    return std::make_tuple(grp_ptr, grp);
}

inline std::tuple<struct group*, std::shared_ptr<Group>> getgroup(gid_t gid) {
    struct group *grp_ptr;
    auto grp = std::shared_ptr<Group>(new Group());

    if (getgrgid_r(gid,
                   (struct group*)&grp->grpbuf,
                   (char *)&grp->buf,
                   sizeof(grp->buf),
                   &grp_ptr) != 0)
    {
        throw SystemError("Failure in getgrnam_r(): ", errno);
    }

    if (grp_ptr == nullptr)
        throw SystemError("No such group: ", errno);

    return std::make_tuple(grp_ptr, grp);
}

struct User {
    struct passwd pwdbuf;
    char buf[200];
};

inline std::tuple<struct passwd*, std::shared_ptr<User>> getuser(const std::string& name) {
    struct passwd *pwd_ptr;
    auto usr = std::shared_ptr<User>(new User());
    
    if (getpwnam_r(name.c_str(),
                   (struct passwd*)&usr->pwdbuf,
                   (char *)&usr->buf,
                   sizeof(usr->buf),
                   &pwd_ptr) != 0)
    {
        throw SystemError("Failure in getpwnam_r(): ", errno);
    }

    if (pwd_ptr == nullptr)
        throw SystemError("No such user: " + name);

    return std::make_tuple(pwd_ptr, usr);
}

inline std::tuple<struct passwd*, std::shared_ptr<User>> getuser(uid_t uid) {
    struct passwd *pwd_ptr;
    auto usr = std::shared_ptr<User>(new User());
    
    if (getpwuid_r(uid,
                   (struct passwd*)&usr->pwdbuf,
                   (char *)&usr->buf,
                   sizeof(usr->buf),
                   &pwd_ptr) != 0)
    {
        throw SystemError("Failure in getpwuid_r(): ", errno);
    }

    if (pwd_ptr == nullptr)
        throw SystemError("No such user: ", errno);

    return std::make_tuple(pwd_ptr, usr);
}

inline std::tuple<struct passwd*, std::shared_ptr<User>> getuser() {
    return getuser(getuid());
}

/**
 * Compute the permission mode bitfield from a string during compile time, so
 * that you don't make stupid mistakes.
 *
 * @param perm A string representation of file permissions (example: rwxr-x---)
 */
Permissions::Collection parsePermissions(const std::string& perm);

bool checkFile(const std::string& path, const std::string& mode);

Collection describeFile(const std::string& path);

std::string fileTypeChar(const struct stat *stbuf);

}
