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

public:
    MacroDaemon();
    ~MacroDaemon();

    void run();
};
