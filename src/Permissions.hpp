extern "C" {
    #include <sys/types.h>
    #include <grp.h>
}

#include <tuple>
#include <string>
#include <memory>

#include "SystemError.hpp"

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
