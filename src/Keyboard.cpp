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
static inline string _ioctlGetString(int fd, unsigned long what) {
    char buf[ioctl_get_bufsz];
    ssize_t sz;
    if ((sz = ioctl(fd, what, buf)) == -1)
        return "";
    return string(buf);
}

Keyboard::Keyboard(const char *path) {
    syslog(LOG_INFO, "Opening device: '%s' ...", path);
    fd = open(path, O_RDONLY);

    if (fd == -1) {
        std::stringstream err("Cannot open: ");
        err << path << ": ";
        err << strerror(errno);
        throw KeyboardError(err.str());
    }

    syslog(LOG_INFO, "ioctl get on device: '%s' ...", path);
    name = ioctlGetString(fd, EVIOCGNAME);
    uniq_id = ioctlGetString(fd, EVIOCGUNIQ);
    phys = ioctlGetString(fd, EVIOCGPHYS);
    if (ioctl(fd, EVIOCGID, &dev_id) == -1) {
        memset(&dev_id, 0, sizeof(dev_id));
    }
    // Not a 32-bit integer actually is input_id, a 64 bit struct.
    // num_id = ioctlGetInt(fd, EVIOCGID);
}

Keyboard::~Keyboard() {
    if (locked)
        unlock();
    close(fd);
}

bool Keyboard::isMe(const char *path) const {
    errno = 0;
    int new_fd = open(path, O_RDONLY);
    if (new_fd == -1) {
        throw SystemError("Error in open(" + string(path) + "): ", errno);
    }
    struct input_id dev_id;
    if (ioctl(new_fd, EVIOCGID, &dev_id) == -1)
        memset(&dev_id, 0, sizeof(dev_id));

    return (name == ioctlGetString(new_fd, EVIOCGNAME) &&
            uniq_id == ioctlGetString(new_fd, EVIOCGUNIQ) &&
            // phys == ioctlGetString(new_fd, EVIOCGPHYS) &&
            !memcmp(&this->dev_id, &dev_id, sizeof(dev_id)));
}

int Keyboard::numDown() const {
    unsigned char key_states[KEY_MAX/8 + 1];
    memset(key_states, 0, sizeof(key_states));
    if (ioctl(fd, EVIOCGKEY(sizeof(key_states)), key_states) == -1)
        throw SystemError("Unable to get key states: ", errno);
    int was_down = 0;
    for (size_t i = 0; i < sizeof(key_states); i++)
        // Shift bits to count downed keys.
        for (unsigned int bs = 1; bs <= 128; bs <<= 1)
            was_down += !!(bs & key_states[i]);
    return was_down;
}

void Keyboard::lockSync() {
    int grab = 1;
    struct input_event ev;
    int down = numDown(), up = 0;

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

    if (ioctl(fd, EVIOCGRAB, &grab) == -1)
        throw SystemError("Unable to lock keyboard: ", errno);

    state = KBDState::LOCKED;

    cout << "\033[31;1;4mKBD LOCKED\033[0m" << endl;
}

void Keyboard::lock() {
    syslog(LOG_INFO, "Locking keyboard: %s ...", name.c_str());
    if (numDown() == 0) {
        int grab = 1;
        if (ioctl(fd, EVIOCGRAB, &grab) == -1)
            throw SystemError("Unable to lock keyboard: ", errno);
        syslog(LOG_INFO, "Immediate lock on: %s @ %s", name.c_str(), phys.c_str());
        state = KBDState::LOCKED;
    } else {
        syslog(LOG_INFO, "Preparing async lock on: %s @ %s", name.c_str(), phys.c_str());
        state = KBDState::LOCKING;
    }
}

void Keyboard::unlock() {
    if (state == KBDState::LOCKED)
        if (ioctl(fd, EVIOCGRAB, NULL) == -1)
            throw KeyboardError("Failure in ioctl(EVIOCGRAB)");

    state = KBDState::OPEN;
}

void Keyboard::get(struct input_event *ev) {
    ssize_t n;
    if ((n = read(fd, ev, sizeof(*ev))) != sizeof(*ev)) {
        stringstream err("read() failed, returned: ");
        err << n << ": " << strerror(errno);
        throw KeyboardError(err.str());
    }

    // Wait until key down events have been eliminated.
    if (state == KBDState::LOCKING && !numDown()) {
        int grab = 1;
        if (ioctl(fd, EVIOCGRAB, &grab) == -1)
            throw SystemError("Unable to lock keyboard: ", errno);
        state = KBDState::LOCKED;
        syslog(LOG_INFO, "Acquired lock on keyboard: %s", name.c_str());
    }
}

void Keyboard::disable() noexcept {
    try {
        unlock();
    } catch (const KeyboardError& e) {
        // Swallow the error.
        syslog(LOG_ERR, "Error in unlock: %s", e.what());
    }

    if (close(fd) == -1) {
        auto exc = SystemError("Unable to close(): ", errno);
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
