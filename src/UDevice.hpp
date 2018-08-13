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
#include "IUDevice.hpp"

class UDevice : public IUDevice {
private:
    static const size_t evbuf_start_len = 128;
    int fd;
    uinput_setup usetup;
    size_t evbuf_len;
    size_t evbuf_top;
    input_event *evbuf;

public:
    UDevice();

    ~UDevice();

    virtual void emit(const input_event *send_event) override;

    virtual void emit(int type, int code, int val) override;

    virtual void flush() override;

    virtual void done() override;
};
