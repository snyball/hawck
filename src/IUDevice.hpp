/* =====================================================================================
 * User device abstract class.
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

/** @file IUDevice.hpp
 * 
 * Virtual device interface, and no,
 * the virtual methods are not some sort of pun.
 */

#pragma once

extern "C" {
    #include <unistd.h>
    #include <fcntl.h>
    #include <linux/uinput.h>
    #include <linux/input.h>
    #include <errno.h>
    #include <stdlib.h>
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}
#include "LuaUtils.hpp"

class IUDevice {
public:
    /** Destroy virtual device. */
    virtual ~IUDevice() {}

    /**
     * Send an input event struct, this is useful for when
     * you want to re-emit a cought event.
     *
     * @param send_event Event to emit.
     */
    virtual void emit(const input_event *send_event) = 0;

    /**
     * Emit an input event on the virtual device.
     *
     * @param type What kind of event it is, i.e EV_KEY or EV_SYN
     * @param code Key code, i.e which key got pressed.
     * @param val Supplementary information for the event, for
     *            keys, 0 means key up and 1 means key down, and
     *            2 means key repeat.
     */
    virtual void emit(int type, int code, int val) = 0;

    virtual void done() = 0;

    /**
     * Flush buffered events to the virtual device.
     */
    virtual void flush() = 0;
};
