#include <string>
#include "KBDAction.hpp"

/**
 * Persistent connection to a keyboard server, a daemon
 * that performs actions depending on keypresses.
 */
template <class Packet>
class UNIXConnection {
private:
    int fd;

public:
    UNIXConnection(std::string addr) {}
    ~UNIXConnection() {}
    void close();
    void send(const Packet *action);
};
