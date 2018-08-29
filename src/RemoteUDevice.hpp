/* =====================================================================================
 * Remote user input device
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

/** @file RemoteUDevice.hpp
 *
 * @brief Remote user input device.
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
#include <string.h>
#include <stdio.h>
#include <stdexcept>
#include "LuaUtils.hpp"
#include "UNIXSocket.hpp"
#include "IUDevice.hpp"
#include "KBDAction.hpp"

// Methods to export to Lua
// (ClassName, methodName, type0(), type1()...)
#define RemoteUDevice_lua_methods(M, _)                 \
    M(RemoteUDevice, emit, int(), int(), int()) _       \
    M(RemoteUDevice, flush)

// Declare extern "C" Lua bindings
LUA_DECLARE(RemoteUDevice_lua_methods)

/** Remote UDevice
 *
 * See UDevice
 */
class RemoteUDevice : public IUDevice,
                      public Lua::LuaIface<RemoteUDevice> {
private:
    UNIXSocket<KBDAction> *conn = nullptr;
    std::vector<KBDAction> evbuf;

    // Collect methods into an array
    LUA_METHOD_COLLECT(RemoteUDevice_lua_methods);

public:
    explicit RemoteUDevice(UNIXSocket<KBDAction> *conn);

    RemoteUDevice();

    virtual ~RemoteUDevice();

    virtual void emit(const input_event *send_event) override;

    virtual void emit(int type, int code, int val) override;

    virtual void done() override;

    virtual void flush() override;

    inline void setConnection(UNIXSocket<KBDAction> *conn) {
        this->conn = conn;
    }

    // Extract methods as static members taking `this` as
    // the first argument for binding with Lua
    LUA_EXTRACT(RemoteUDevice_lua_methods)
};
