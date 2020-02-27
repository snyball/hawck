#include "Popen.hpp"

using namespace std;

Popen::~Popen() {
    int status;
    if (pid != -1)
        waitpid(pid, &status, 0);
}

string Popen::readOnce() {
    std::stringstream sstream("");
    char buf[8192];
    int nb, status;

    do {
        nb = ::read(stdout.io[0], buf, sizeof(buf) - 1);
        if (nb == -1)
            throw SystemError("Unable to read: ", errno);
        buf[nb] = '\0';
        sstream << buf;
    } while (waitpid(pid, &status, WNOHANG) != -1);

    if (status != 0)
        throw SubprocessError(exe, stderr.read(0), status);
    return sstream.str();
}

string Pipe::read(int idx) {
    std::stringstream sstream("");
    char buf[1024];
    int nb;

    do {
        nb = ::read(io[idx], buf, sizeof(buf) - 1);
        if (nb == -1)
            throw SystemError("Unable to read(): ", errno);
        buf[nb] = '\0';
        sstream << buf;
    } while (nb > 0);

    return sstream.str();
}
