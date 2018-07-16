#include <string>
#include "KBDAction.hpp"

/**
 * Persistent connection to a keyboard server, a daemon
 * that performs actions depending on keypresses.
 */
class KBDConnection {
private:
    int fd;

public:
    KBDConnection(std::string addr);
    ~KBDConnection();
    void close();
    void send(const KBDAction *action);
};
