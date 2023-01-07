/* =====================================================================================
 * UNIX socket helper library.
 *
 * Copyright (C) 2018 Jonas Møller (no) <jonas.moeller2@protonmail.com>
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

/** @file UNIXSocket.hpp
 *
 * @brief UNIX socket helper library.
 */

#pragma once

extern "C" {
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <sys/un.h>
    #include <poll.h>
}

#include <string>
#include <iostream>
#include <sstream>
#include <exception>
#include <system_error>
#include <cstring>
#include <vector>
#include <chrono>

#include "SystemError.hpp"

class SocketError : public std::exception {
private:
    std::string expl;
public:
    explicit SocketError(const std::string& expl) noexcept : expl(expl) {}

    virtual const char *what() const noexcept {
        return expl.c_str();
    }
};

class SocketTimeout : public SocketError {
public:
    explicit SocketTimeout(const std::string& expl) noexcept : SocketError(expl) {}
};

/**
 * Receive an exact amount of bytes on a file descriptor,
 * all or nothing.
 *
 * @param fd The file descriptor to recieve on.
 * @param dst The buffer to insert received data into.
 * @param sz The amount of bytes to be read.
 */
inline void recvAll(int fd, char *dst, ssize_t sz) {
    for (size_t n; (sz -= (n = ::read(fd, dst, sz))) > 0; dst += n)
        if (n <= 0)
            throw SocketError("Unable to receive packet: " + std::string(strerror(errno)));
}

/**
 * Receive an exact amount of bytes on a file descriptor,
 * all or nothing, with a timeout.
 *
 * @param fd The file descriptor to recieve on.
 * @param dst The buffer to insert received data into.
 * @param sz The amount of bytes to be read.
 * @param timeout The timeout in milliseconds.
 */
inline void recvAll(int fd, char *dst, ssize_t sz, std::chrono::milliseconds timeout) {
    using namespace std::chrono;
    auto now = []() {
                   return duration_cast<milliseconds>(system_clock::now().time_since_epoch());
               };

    struct pollfd pfd;
    pfd.events = POLLIN;
    pfd.fd = fd;
    for (size_t n = 0; sz -= n; dst += n) {
        auto start_t = now();

        switch (poll(&pfd, 1, timeout.count())) {
            case -1:
                throw SystemError("Error in poll(): ", errno);

            case 0:
                goto end_loop;

            default:
                if (pfd.revents & POLLIN && (n = ::read(fd, dst, sz)) <= 0)
                    throw SocketError("Unable to receive packet: " + std::string(SystemError("", errno).what()));
        }

        // Check if we ran out of time
        if ((timeout -= (now() - start_t)) <= milliseconds(0))
            break;
    } end_loop:

    if (sz)
        throw SocketTimeout("Unable to receive packet: timeout");
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
 * Receive an entire object, all or nothing, with a timeout.
 *
 * @param fd The file descriptor to recieve on.
 * @param obj The buffer to insert received data into.
 *
 * @see recvAll(int fd, char *dst, ssize_t sz)
 */
template <class T>
inline void recvAll(int fd, T *obj, std::chrono::milliseconds timeout) {
    recvAll(fd, (char *) obj, sizeof(*obj), timeout);
}

/**
 * UNIX socket connection for sending discrete packets.
 */
template <class Packet>
class UNIXSocket {
private:
    int fd;
    std::string addr = "";

    int connectTo(const std::string& addr) {
        int fd;
        struct sockaddr_un saun;
        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            throw SocketError("Unable to create socket");
        }
        memset(&saun, 0, sizeof(saun));
        saun.sun_family = AF_UNIX;
        strncpy(saun.sun_path, addr.c_str(), sizeof(saun.sun_path) - 1);
        const size_t len = sizeof(saun.sun_family) + strlen(saun.sun_path);
        errno = 0;
        int last_errno = 0;
        while (::connect(fd, (sockaddr*)&saun, len) != 0) {
            // Only print the error if it changed.
            if (errno != last_errno) {
                auto exc = SystemError("", errno);
                last_errno = errno;
            }
            usleep(250000);
            errno = 0;
        }
        fprintf(stderr, "Connection established!\n");
        return fd;
    }

public:
    /**
     * Create a socket from a file descriptor.
     *
     * @param fd File descriptor.
     */
    explicit UNIXSocket(int fd) noexcept : fd(fd) {}

    /**
     * Establish a socket connection.
     *
     * Will attempt a connection ad infinitum until it succeeds.
     *
     * @param addr The address to connect to.
     */
    explicit UNIXSocket(const std::string& addr) {
        fd = connectTo(addr);
        this->addr = addr;
    }

    /** Reconnect to the server, this only works for UNIXSockets
     *  that have addr set. */
    void recon() {
        close();
        fd = connectTo(addr);
    }

    /**
     * Closes the connection.
     */
    ~UNIXSocket() noexcept {
        close();
    }

    /**
     * Closes the connection.
     */
    void close() noexcept {
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
     * Receive a packet.
     *
     * All or nothing, if the entire packet cannot be received
     * a SocketError exception will be thrown.
     *
     * @param p The buffer to insert the packet into.
     */
    void recv(Packet *p, std::chrono::milliseconds timeout) {
        recvAll(fd, p, timeout);
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
        if (packets.size() == 0)
            return;
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
    explicit UNIXServer(std::string addr) {
        struct sockaddr_un saun;

        if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
            throw SocketError("Unable to create UNIX socket");
        }

        memset(&saun, 0, sizeof(saun));
        saun.sun_family = AF_UNIX;
        strncpy(saun.sun_path, addr.c_str(), sizeof(saun.sun_path) - 1);

        unlink(addr.c_str());
        size_t len = sizeof(saun.sun_family) + strlen(saun.sun_path);

        if (bind(fd, (sockaddr*)&saun, len) == -1) {
            throw SocketError("Unable to bind to address: " + addr);
        }

        if (listen(fd, 5) == -1) {
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
        int ns;
        sockaddr_un saun;
        socklen_t fromlen = sizeof(saun);
        if ((ns = ::accept(fd, (sockaddr *)&saun, &fromlen)) == -1) {
            throw SocketError("Unable to accept connections.");
        }
        return ns;
    }
};
