/* =====================================================================================
 * Remote user input device
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

#include "RemoteUDevice.hpp"

RemoteUDevice::RemoteUDevice(UNIXSocket<KBDAction> *conn)
    : LuaIface(this, RemoteUDevice_lua_methods) {
    this->conn = conn;
}

RemoteUDevice::RemoteUDevice()
    : LuaIface(this, RemoteUDevice_lua_methods) {}

RemoteUDevice::~RemoteUDevice() {}

void RemoteUDevice::emit(int type, int code, int val) {
    KBDAction ac;
    ac.ev.type = type;
    ac.ev.code = code;
    ac.ev.value = val;
    ac.done = 0;
    evbuf.push_back(ac);
}

void RemoteUDevice::emit(const input_event *send_event) {
    KBDAction ac;
    memset(&ac, 0, sizeof(ac));
    memcpy(&ac.ev, send_event, sizeof(*send_event));
    ac.done = 0;
    evbuf.push_back(ac);
}

void RemoteUDevice::flush() {
    if (!conn)
        return;
    if (evbuf.size()) {
        conn->send(evbuf);
        evbuf.clear();
    }
}

void RemoteUDevice::done() {
    if (!conn)
        return;
    flush();
    KBDAction ac;
    memset(&ac, 0, sizeof(ac));
    ac.done = 1;
    conn->send(&ac);
}

LUA_CREATE_BINDINGS(RemoteUDevice_lua_methods)
