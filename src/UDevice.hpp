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
#include "IUDevice.hpp"

// Methods to export to Lua
// (ClassName, methodName, type0(), type1()...)
#define UDevice_lua_methods(M, _)                 \
    M(UDevice, emit, int(), int(), int()) _       \
    M(UDevice, flush)

// Declare extern "C" Lua bindings
LUA_DECLARE(UDevice_lua_methods)

class UDevice : public IUDevice,
                public Lua::LuaIface<UDevice> {
private:
    static const size_t evbuf_start_len = 128;
    int fd;
    uinput_setup usetup;
    size_t evbuf_len;
    size_t evbuf_top;
    input_event *evbuf;

    // Collect methods into an array
    LUA_METHOD_COLLECT(UDevice_lua_methods);

public:
    UDevice();

    ~UDevice();

    virtual void emit(const input_event *send_event) override;

    virtual void emit(int type, int code, int val) override;

    virtual void flush() override;

    virtual void done() override;

    // Extract methods as static members taking `this` as
    // the first argument for binding with Lua
    LUA_EXTRACT(UDevice_lua_methods)
};
