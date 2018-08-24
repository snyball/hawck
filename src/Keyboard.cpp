/* =====================================================================================
 * Keyboard listening class.
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

#include <sstream>
#include <errno.h>
#include <iostream>
extern "C" {
    #include <string.h>
    #include <poll.h>
    #include <syslog.h>
}

#include "Keyboard.hpp"
#include "SystemError.hpp"
#include "utils.hpp"

using namespace std;

/** Size of buffers given to ioctl. */
constexpr size_t ioctl_get_bufsz = 512;

/**
 * Retrieve a string from a device using ioctl,
 *
 * @param _fd File descriptor of ioctl
 * @param _what A macro taking a length argument, should be one of
 *              the EVIOCG* function-like macros from <linux/uinput.h>.
 */
#define ioctlGetString(_fd, _what) \
    _ioctlGetString((_fd), _what(ioctl_get_bufsz))

/**
 * @see ioctlGetString(_fd, _what)
 */
static inline string _ioctlGetString(int fd, int what) {
    char buf[ioctl_get_bufsz];
    ssize_t sz;
    if ((sz = ioctl(fd, what, buf)) == -1)
        return "";
    return string(buf);
}

/**
 * Retrieve an integer value from a device using ioctl.
 *
 * @param fd File descriptor for the device.
 * @param what What information to retrieve, should be one of the
 *             EVIOCG* non-function macros from <linux/input.h>.
 * @return The integer received.
 */
static inline int ioctlGetInt(int fd, int what) {
    int ret;
    if (ioctl(fd, what, &ret) == -1)
        return -1;
    return ret;
}

Keyboard::Keyboard(const char *path) {
    fd = open(path, O_RDONLY);

    if (fd == -1) {
        std::stringstream err("Cannot open: ");
        err << path << ": ";
        err << strerror(errno);
        throw KeyboardError(err.str());
    }

    name = ioctlGetString(fd, EVIOCGNAME);
    uniq_id = ioctlGetString(fd, EVIOCGUNIQ);
    phys = ioctlGetString(fd, EVIOCGPHYS);
    // Not a 32-bit integer actually is input_id, a 64 bit struct.
    // num_id = ioctlGetInt(fd, EVIOCGID);
}

Keyboard::~Keyboard() {
    if (locked)
        unlock();
    close(fd);
}

bool Keyboard::isMe(const char *path) const {
    auto ext = mkuniq(fopen(path, "r"), &fclose);
    if (ext == nullptr)
        throw SystemError("Error in fopen(): ", errno);
    int fd = ext->_fileno;

    return (name == ioctlGetString(fd, EVIOCGNAME) &&
            uniq_id == ioctlGetString(fd, EVIOCGUNIQ) &&
            phys == ioctlGetString(fd, EVIOCGPHYS) &&
            num_id == ioctlGetInt(fd, EVIOCGID));
}

void Keyboard::lock() {
    locked = true;
    int grab = 1;
    struct input_event ev;
    int down = 0, up = 0;

    // TODO: Figure out initial key downs from this:
    #if 0
    unsigned char key_states[KEY_MAX/8 + 1];
    memset(key_states, 0, sizeof(key_states));
    ioctl(fd, EVIOCGKEY(sizeof(key_states)), key_states);
    int was_down = 0;
    for (size_t i = 0; i < sizeof(key_states); i++)
        // Shift bits to count downed keys.
        for (unsigned int bs = 1; bs <= 128; bs <<= 1)
            was_down += bs & key_states[i];
    syslog(LOG_INFO, "Keys held: %d", was_down);
    cout << "Keys held: " << was_down << endl;
    #endif

    cout << "\033[32;1;4mKBD READY\033[0m" << endl;

    // Wait for keyup
    do {
        get(&ev);
        if (ev.code == 0)
            continue;
        if (ev.value == 0)
            up++;
        if (ev.value == 1)
            down++;
        if (ev.value != 2)
            fprintf(stderr, "\rdown = %d, up = %d; GOT EVENT %d\n", down, up, ev.value);
        else
            fprintf(stderr, "\r");
    } while (ev.type != EV_KEY || ev.value != 0 || down > up);

    ioctl(fd, EVIOCGRAB, &grab);

    cout << "\033[31;1;4mKBD LOCKED\033[0m" << endl;
}

void Keyboard::unlock() {
    locked = false;
    int grab = 0;
    ioctl(fd, EVIOCGRAB, &grab);
}

void Keyboard::get(struct input_event *ev) {
    ssize_t n;
    if ((n = read(fd, ev, sizeof(*ev))) != sizeof(*ev)) {
        stringstream err("read() failed, returned: ");
        err << n << ": " << strerror(errno);
        throw KeyboardError(err.str());
    }
}

void Keyboard::disable() noexcept {
    if (close(fd) == -1) {
        auto exc = SystemError("Unable to close(): ", fd);
        syslog(LOG_ERR, "%s", exc.what());
    }
    fd = -1;
}

void Keyboard::reset(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        throw SystemError("Error in open(): ", errno);
    this->fd = fd;
}

int kbdMultiplex(const std::vector<Keyboard*>& kbds, int timeout) {
    size_t idx = 0,
           len = kbds.size();
    struct pollfd pfds[len];
    for (const auto& kbd : kbds) {
        pfds[idx].events = POLLIN;
        pfds[idx++].fd = kbd->getfd();
    }

    int num_fds;
    errno = 0;
    switch (num_fds = poll(pfds, len, timeout)) {
        case -1:
            throw SystemError("Error in poll(): ", errno);

        case 0:
            if (timeout == -1)
                throw SystemError("Poll should not have timed out");
            return -1;

        default:
            int fd = -1;
            for (auto pfd : pfds)
                if (pfd.revents & (POLLNVAL | POLLERR | POLLHUP | POLLNVAL | POLLIN)) {
                    fd = pfd.fd;
                    break;
                }
            for (idx = 0; idx < kbds.size(); idx++)
                if (pfds[idx].fd == fd)
                    return idx;

            throw SystemError("Unable to find file descriptor returned by poll()");
    }
}
