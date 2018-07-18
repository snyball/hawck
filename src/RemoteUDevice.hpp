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

class RemoteUDevice : public IUDevice,
                      public Lua::LuaIface<RemoteUDevice> {
private:
    UNIXSocket<KBDAction> *conn;
    std::vector<KBDAction> evbuf;

    // Collect methods into an array
    LUA_METHOD_COLLECT(RemoteUDevice_lua_methods);

public:
    RemoteUDevice(UNIXSocket<KBDAction> *conn);

    ~RemoteUDevice();

    virtual void emit(const input_event *send_event) override;

    virtual void emit(int type, int code, int val) override;

    virtual void done() override;

    virtual void flush() override;

    // Extract methods as static members taking `this` as
    // the first argument for binding with Lua
    LUA_EXTRACT(RemoteUDevice_lua_methods)
};
