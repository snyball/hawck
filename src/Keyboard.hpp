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

/** @file Keyboard.hpp
 *
 * @brief Read directly from the keyboard.
 */

#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include <functional>

extern "C" {
    #include <unistd.h>
    #include <fcntl.h>
    #include <linux/uinput.h>
    #include <linux/input.h>
    #include <errno.h>
    #include <stdlib.h>
}

#include "FSWatcher.hpp"

class KeyboardError : public std::runtime_error {
public:
    explicit inline KeyboardError(std::string&& msg) : std::runtime_error(msg) {}
};

/** Keyboard state, used in the locking process */
enum KBDState {
    LOCKED,
    LOCKING,
    OPEN,
};

/**
 * Read directly from a keyboard input device.
 */
class Keyboard {
private:
    /** Whether or not we have an exclusive lock on the device. */
    bool locked;
    /** Human-readable name of the device. */
    std::string name = "";
    /** Name of the device in /dev/input/by-id/ */
    std::string by_id = ""; // TODO: Initialize this
    /** Name of the device in /dev/input/by-path/ */
    std::string by_path = ""; // TODO: Initialize this
    /** This path is volatile, if the device is unplugged it will
     *  cease to exist. */
    std::string ev_path = "";
    /** Physical location of device. */
    std::string phys = "";
    /** Numeric id of the device. */
    struct input_id dev_id;
    /** Unique id of the device. */
    std::string uniq_id = "";
    /** Filed descriptor for keyboard device. */
    int fd = -1;
    /** State of the keyboard, used in locking. */
    KBDState state = KBDState::OPEN;

public:
    /** Keyboard constructor.
     *
     * Tip: If you don't know which path you want, try using or looking
     *      into the lsinput program (part of Hawck.)
     *
     * @param path Path to the character device file of the device,
     *             this should be in /dev/input/.
     */
    explicit Keyboard(const char *path);

    /** Keyboard destructor.
     * 
     * Will also unlock the keyboard if it is locked.
     */
    ~Keyboard();

    /** Acquire an exclusive lock to the keyboard. */
    void lock();

    /** Acquire a lock immediately, this function will block until
     *  a lock has been acquired. */
    void lockSync();

    /** Release the exclusive lock to the keyboard. */
    void unlock();

    /** Disable the keyboard. */
    void disable() noexcept;

    /** Check whether or not the keyboard has been disabled,
     * this happens if disable() is called, usually as a result
     * of the get() function throwing a KeyboardException.
     *
     * @return Disabled status.
     */
    inline bool isDisabled() const noexcept {
        return fd < 0;
    }

    /** Reset the device, this is done after the device has
     *  been removed and then added again (physically.)
     *
     * You should only reset on a new device after confirming
     * it's identity with Keyboard::isMe
     */
    void reset(const char *path);

    /** Check if the given device is the same device as the
     *  instance was initialized to originally. */
    bool isMe(const char *path) const;

    /** Get an event from the keyboard.
     *
     * This call will block until a key is pressed.
     */
    void get(struct input_event *ev);

    /** Get human-readable name of the keyboard device.
     *
     * @return Human-readable name of device.
     */
    inline const std::string& getName() const noexcept {
        return name;
    }

    /** Get file descriptor of the keyboard device.
     * 
     * @return System file descriptor.
     */
    inline int getfd() const noexcept {
        return fd;
    }

    /**
     * Get the current state of the keyboard locking process.
     *
     * @return Current state, @see KBDState
     */
    inline KBDState getState() const noexcept {
        return state;
    }

    /**
     * Get the number of keys current held down on the keyboard.
     *
     * @return Number of keys held down.
     */
    int numDown() const;
};

/**
 * IO multiplexing on keyboards.
 *
 * @param kbds Keyboards to check.
 * @param timeout Time to wait in milliseconds.
 * 
 * @throws SystemError if the underlying polling function fails.
 *
 * @return Index of keyboard with available input in kbds, returns
 *         -1 if the function timed out.
 */
int kbdMultiplex(const std::vector<Keyboard*>& kbds, int timeout);

/**
 * Same as kbdMultiplex, but will not time out.
 *
 * @see kbdMultiplex
 */
inline int kbdMultiplex(const std::vector<Keyboard*>& kbds) {
    return kbdMultiplex(kbds, -1);
}
