#include "Permissions.hpp"

using namespace std;

std::string Permissions::fmtPermissions(unsigned num) noexcept {
    stringstream ret;
    string bitstr[] = {"r", "w", "x"};
    for (unsigned mask = S_IRUSR, i = 0; mask >= S_IXOTH; mask >>= 1, i++)
        ret << ((num & mask) ? bitstr[i % 3] : "-");
    return ret.str();
}

std::string Permissions::fmtPermissions(struct stat& stbuf) noexcept {
    stringstream ret;
    string rwx = fmtPermissions(reinterpret_cast<unsigned>(stbuf.st_mode));
    auto [grp, grpbuf] = getgroup(stbuf.st_gid);
    auto [usr, usrbuf] = getuser(stbuf.st_uid);
    ret << rwx << " " << string(usr->pw_name) << ":" << grp->gr_name;
    return ret.str();
}
