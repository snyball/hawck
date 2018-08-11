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

#include "Keyboard.hpp"
#include <sstream>
#include <errno.h>
extern "C" {
    #include <string.h>
}

using namespace std;

Keyboard::Keyboard(const char *path) {
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        std::stringstream err("Cannot open: ");
        err << path << ": ";
        err << strerror(errno);
        throw KeyboardError(err.str());
    }
}

Keyboard::~Keyboard() {
    if (locked)
        unlock();
    close(fd);
}

void Keyboard::lock() {
    locked = true;
    int grab = 1;
    struct input_event ev;
    // bool held_keys[KEY_ONSCREEN_KEYBOARD];
    int down = 0, up = 0;

    // Wait for keyup
    do {
        get(&ev);
        if (ev.code == 0)
            continue;
        if (ev.value == 0)
            up++;
        if (ev.value == 1)
            down++;
        fprintf(stderr, "\rdown = %d, up = %d; GOT EVENT %d WITH KEY %d\n", down, up, ev.value, (int)ev.code);
    } while (ev.type != EV_KEY || ev.value != 0 || down > up);

    ioctl(fd, EVIOCGRAB, &grab);

    printf("KBD LOCKED\n");
}

void Keyboard::unlock() {
    locked = false;
    ioctl(fd, EVIOCGRAB, NULL);
}

void Keyboard::get(struct input_event *ev) {
    ssize_t n;
    if ((n = read(fd, ev, sizeof(*ev))) != sizeof(*ev)) {
        stringstream err("read() failed, returned: ");
        err << n << ": " << strerror(errno);
        throw KeyboardError(err.str());
    }
}
