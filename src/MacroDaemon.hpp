/* =====================================================================================
 * Macro daemon.
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

#include <vector>
#include <memory>

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

#include "UNIXSocket.hpp"
#include "KBDAction.hpp"
#include "LuaUtils.hpp"
#include "RemoteUDevice.hpp"

class MacroDaemon {
private:
    UNIXServer kbd_srv;
    UNIXSocket<KBDAction> *kbd_com;
    std::vector<Lua::Script *> scripts;
    RemoteUDevice *remote_udev;

    void notify(std::string title,
                std::string msg,
                const Lua::Script *script,
                const lua_Debug *ar);

public:
    MacroDaemon();
    ~MacroDaemon();

    void run();
};
