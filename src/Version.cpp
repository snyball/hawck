extern "C" {
    #include <sys/utsname.h>
    #include <unistd.h>
}

#include "Version.hpp"
#include <regex>

using namespace std;

tuple<size_t, size_t, size_t> getLinuxVersion() {
    const static regex rx("^(\\d+)\\.(\\d+)\\.(\\d+)-.*");
    struct utsname name;
    if (uname(&name))
        throw SystemError("Uname failed: ", errno);
    std::string vstr(name.release);
    smatch cm;
    if (regex_search(vstr, cm, rx)) {
        if (cm.size() < 4)
            throw SystemError("Invalid kernel version format");

        int major = atoi(cm[1].str().c_str()),
            minor = atoi(cm[2].str().c_str()),
            patch = atoi(cm[3].str().c_str());

        return std::tuple(major, minor, patch);
    }
    throw SystemError("Invalid kernel version format");
}
