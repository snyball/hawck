extern "C" {
    #include <sys/types.h>
    #include <grp.h>
    #include <sys/stat.h>
    #include <syslog.h>
    #include <pwd.h>
}

#include <tuple>
#include <string>
#include <memory>
#include <vector>
#include <sstream>

#include "SystemError.hpp"

namespace Permissions {

struct Group {
    struct group grpbuf;
    char buf[200];
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

    return std::make_tuple(pwd_ptr, usr);
}

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

}
