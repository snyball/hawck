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
    virtual ~IUDevice() {}

    virtual void emit(const input_event *send_event) = 0;

    virtual void emit(int type, int code, int val) = 0;

    virtual void done() = 0;

    virtual void flush() = 0;
};
