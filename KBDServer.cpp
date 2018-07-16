#include <system_error>
#include <sstream>
extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
}

#include "KBDServer.hpp"

using namespace std;

KBDServer::KBDServer(string addr) {
    struct sockaddr_un saun;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        throw SocketError("Unable to create UNIX socket");
    }

    saun.sun_family = AF_UNIX;
    strcpy(saun.sun_path, addr.c_str());

    unlink(addr.c_str());
    size_t len = sizeof(saun.sun_family) + strlen(saun.sun_path);

    if (bind(fd, (sockaddr*)&saun, len) < 0) {
        stringstream err("Unable to bind to address: ");
        err << addr;
        throw SocketError(err.str());
    }

    if (listen(fd, 5) < 0) {
        throw SocketError("Unable to listen on socket.");
    }
}

KBDServer::~KBDServer() {
    close(fd);
}

void KBDServer::recv(KBDAction *action) {
    ::recv(fd, action, sizeof(*action), 0);
}
