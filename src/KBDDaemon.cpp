#include "KBDDaemon.hpp"
#include <iostream>

using namespace std;

KBDDaemon::KBDDaemon(const char *device) :
    LuaIface(this, KBDDaemon_lua_methods),
    kbd_com("kbd.sock"),
    kbd(device)
{
    initLua("default.hwk");
}

KBDDaemon::~KBDDaemon() {}

void KBDDaemon::run() {
    KBDAction action;

    kbd.lock();

    // 10 consecutive socket errors will lead to the keyboard daemon
    // aborting.
    static const int MAX_ERRORS = 10;
    int errors = 0;

    for (;;) {
        action.done = 0;
        try {
            kbd.get(&action.ev);
        } catch (KeyboardError &e) {
            fprintf(stderr, "Error: %s", e.what());
            // Close connection to let the macro daemon know it should
            // terminate.
            kbd_com.close();
            abort();
        }

        // Check if the key is listed in the passthrough set.
        if (passthrough_keys.count(action.ev.code)) {
            // Pass key to Lua executor
            try {
                kbd_com.send(&action);

                // Receive keys to emit from the macro daemon.
                for (;;) {
                    kbd_com.recv(&action);
                    if (action.done) {
                        break;
                    }
                    udev.emit(&action.ev);
                }
                // Flush received keys and continue on.
                udev.flush();
                errors = 0;
                // Skip emmision of the key if everything went OK
                continue;
            } catch (SocketError &e) {
                cout << e.what() << endl;
                // If we're getting constant errors then the daemon needs
                // to be stopped, as the macro daemon might have crashed
                // or run into an error.
                if (errors++ > MAX_ERRORS) {
                    kbd_com.close();
                    abort();
                }
                // On error control flow continues on to udev.emit()
            }
        }

        udev.emit(&action.ev);
        udev.flush();
    }
}

void KBDDaemon::requestKey(int code) {
    passthrough_keys.insert(code);
}

void KBDDaemon::initLua(string path) {
    uid_t cur_uid = getuid();
    uid_t cur_gid = getgid();
    struct stat st;
    if (stat(path.c_str(), &st) < 0) {
        string err(strerror(errno));
        throw SystemError("Unable to stat(): " + err);
    }

    if (st.st_uid != cur_uid || st.st_gid != cur_gid) {
        throw SystemError("Will not load scripts with unsafe ownership.");
    }

    // FIXME: Can't seem to get this working properly
    // if (st.st_mode != 0744) {
    //     throw SystemError("Will not load scripts with unsafe permissions, require 0744.");
    // }

    // Running __setup will cause the Lua script to request keys using
    // KBDDaemon::requestKey(), letting the daemon know which keys it should
    // pass on to the macro daemon.
    Lua::Script sc(path);
    // Put the KBDDaemon functions into the global scope of the script.
    /// @see Lua::LuaIface
    sc.open(this, "KBDDaemon");
    sc.call<void>("__setup");
}

// Insert extern "C" function implementations for the Lua bindings.
LUA_CREATE_BINDINGS(KBDDaemon_lua_methods)
