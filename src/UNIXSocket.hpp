/* =====================================================================================
 * UNIX socket helper library.
 *
 * Copyright (C) 2018 Jonas Møller (no) <jonasmo441@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */

#pragma once

extern "C" {
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/un.h>
}

#include <string>
#include <sstream>
#include <exception>
#include <system_error>

#include "json.hpp"

using json = nlohmann::json;

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
 * Receive an exact amount of bytes on a socket, all or nothing.
 *
 * @param fd The file descriptor to recieve on.
 * @param dst The buffer to insert received data into.
 * @param sz The amount of bytes to be read.
 */
inline void recvAll(int fd, char *dst, ssize_t sz) {
    ssize_t n;
    while ((sz -= (n = ::recv(fd, dst, sz, 0))) > 0) {
        if (n < 0) {
            std::string err(strerror(errno));
            throw SocketError("Unable to receive packet: " + err);
        }
        dst += n;
    }
}

/**
 * Receive an entire object, all or nothing.
 *
 * @param fd The file descriptor to recieve on.
 * @param obj The buffer to insert received data into.
 *
 * @see recvAll(int fd, char *dst, ssize_t sz)
 */
template <class T>
inline void recvAll(int fd, T *obj) {
    recvAll(fd, (char *) obj, sizeof(*obj));
}

/**
 * UNIX socket connection for sending discrete packets.
 */
template <class Packet>
class UNIXSocket {
private:
    int fd;

public:
    /**
     * Create a socket from a file descriptor.
     *
     * @param fd File descriptor.
     */
    UNIXSocket(int fd) : fd(fd) {}

    /**
     * Establish a socket connection.
     *
     * Will attempt a connection ad infinitum until it succeeds.
     *
     * @param addr The address to connect to.
     */
    UNIXSocket(std::string addr) {
        struct sockaddr_un saun;
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            throw SocketError("Unable to create socket");
        }
        saun.sun_family = AF_UNIX;
        strcpy(saun.sun_path, addr.c_str());
        const size_t len = sizeof(saun.sun_family) + strlen(saun.sun_path);
        while (::connect(fd, (sockaddr*)&saun, len) != 0) {
            fprintf(stderr, "Connection failed, trying again ...\n");
            usleep(500000);
        }
        fprintf(stderr, "Connection established!\n");
    }

    /**
     * Closes the connection.
     */
    ~UNIXSocket() {
        close();
    }

    /**
     * Closes the connection.
     */
    void close() {
        ::close(fd);
    }

    /**
     * Receive a packet.
     *
     * All or nothing, if the entire packet cannot be received
     * a SocketError exception will be thrown.
     *
     * @param p The buffer to insert the packet into.
     */
    void recv(Packet *p) {
        recvAll(fd, p);
    }

    /**
     * Send a packet.
     *
     * @param action The buffer to send.
     */
    void send(const Packet *action) {
        if (::send(fd, action, sizeof(*action), 0) != sizeof(*action)) {
            throw SocketError("Unable to send the packet.");
        }
    }

    /**
     * Send several packets.
     *
     * @param packets Vector holding all the packages.
     */
    void send(const std::vector<Packet> &packets) {
        ssize_t len = sizeof(packets[0])*packets.size();
        if (::send(fd, &packets[0], len, 0) != len) {
            throw SocketError("Unable to send packet");
        }
    }
};

/**
 * UNIX socket server.
 */
class UNIXServer {
private:
    int fd;

public:
    /**
     * Create a listening socket.
     *
     * @param addr The address to listen on.
     */
    UNIXServer(std::string addr) {
        struct sockaddr_un saun;

        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            throw SocketError("Unable to create UNIX socket");
        }

        saun.sun_family = AF_UNIX;
        strcpy(saun.sun_path, addr.c_str());

        unlink(addr.c_str());
        size_t len = sizeof(saun.sun_family) + strlen(saun.sun_path);

        if (bind(fd, (sockaddr*)&saun, len) < 0) {
            std::stringstream err("Unable to bind to address: ");
            err << addr;
            throw SocketError(err.str());
        }

        if (listen(fd, 5) < 0) {
            throw SocketError("Unable to listen on socket.");
        }
    }

    ~UNIXServer() {
        close(fd);
    }

    /**
     * Listen for a connection.
     *
     * @return File descriptor for socket.
     */
    int accept() {
        socklen_t fromlen;
        int ns;
        sockaddr_un saun;
        if ((ns = ::accept(fd, (sockaddr *)&saun, &fromlen)) <= 0) {
            throw SocketError("Unable to accept connections.");
        }
        return ns;
    }
};

/**
 * Json communication channel.
 *
 * TODO: Finish this class up and test it.
 */
class JSONChannel {
private:
    int fd;

public:
    static const ssize_t MAX_JSON_MSG_LEN = 8192;

    JSONChannel(int fd) : fd(fd) {}

    json recv() {
        ssize_t len;
        recvAll(fd, &len);
        if (len > JSONChannel::MAX_JSON_MSG_LEN) {
            throw SocketError("Message was too large");
        }

        char msg[len];
        recvAll(fd, msg, len);

        return json::parse(msg);
    }

    void send(const json &obj) {
        std::string data = obj.dump();
        ssize_t len = data.size();
        if (::send(fd, &len, sizeof(len), 0) != sizeof(len)) {
            throw SocketError("Unable to send json message length");
        }
        if (::send(fd, data.c_str(), len, 0) != len) {
            throw SocketError("Unable to send json message");
        }
    }
};
