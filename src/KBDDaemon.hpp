#include <unordered_map>
#include <set>

#include "KBDConnection.hpp"
#include "UNIXSocket.hpp" 
#pragma once

#include "UDevice.hpp"
#include "LuaUtils.hpp"
#include "Keyboard.hpp"
#include "SystemError.hpp"

extern "C" {
    #include <fcntl.h>
    #include <sys/stat.h>
}

#define KBDDaemon_lua_methods(M, _)             \
    M(KBDDaemon, requestKey, int())

LUA_DECLARE(KBDDaemon_lua_methods)

class KBDDaemon : public Lua::LuaIface<KBDDaemon> {
private:
    std::set<int> passthrough_keys;
    UNIXSocket<KBDAction> kbd_com;
    UDevice udev;
    Keyboard kbd;
    LUA_METHOD_COLLECT(KBDDaemon_lua_methods);

public:
    KBDDaemon(const char *device);
    ~KBDDaemon();

    /**
     * Request a key from the daemon.
     */
    void requestKey(int code);

    void initLua(std::string path);

    /**
     * Start running the daemon.
     */
    void run();

    LUA_EXTRACT(KBDDaemon_lua_methods)
};
