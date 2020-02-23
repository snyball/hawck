/** @file UDevice.hpp
 *
 * @brief User input device.
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
#include <vector>
#include "IUDevice.hpp"

// Methods to export to Lua
// (ClassName, methodName, type0(), type1()...)
#define UDevice_lua_methods(M, _)               \
    M(UDevice, emit, int(), int(), int()) _     \
    M(UDevice, flush)

// Declare extern "C" Lua bindings
LUA_DECLARE(UDevice_lua_methods)

class UDevice : public IUDevice,
                public Lua::LuaIface<UDevice> {
private:
    static const size_t evbuf_start_len = 128;
    int fd;
    int dfd;
    int ev_delay = 3800;
    uinput_setup usetup;
    std::vector<struct input_event> events;

    LUA_METHOD_COLLECT(UDevice_lua_methods);

public:
    UDevice();

    ~UDevice();

    virtual void emit(const struct input_event *send_event) override;

    virtual void emit(int type, int code, int val) override;

    virtual void flush() override;

    virtual void done() override;

    /** Set delay between outputted events in µs
     *
     * This is a workaround for a bug in GNOME Wayland where keys
     * are being dropped if they are sent too fast.
     *
     * @param delay Delay in µs.
     */
    void setEventDelay(int delay);

    /** Generate key up events for all held keys.
     */
    void upAll();

    LUA_EXTRACT(UDevice_lua_methods)
};
