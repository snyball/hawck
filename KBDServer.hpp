#include <string>
#include <exception>

#include "KBDAction.hpp"

class SocketError : public std::exception {
private:
    std::string expl;
public:
    SocketError(std::string expl) : expl(expl) {}

    virtual const char *what() const noexcept {
        return expl.c_str();
    }
};

/**
 * UNIX-domain socket server for receiving keyboard events,
 * these are received from a privileged process able to read
 * from input devices.
 *
 * The process running the server should be running under the
 * unpriviliged regular user process.
 */
class KBDServer {
private:
    int fd;

public:
    KBDServer(std::string addr);
    ~KBDServer();
    void recv(KBDAction *action);
};
