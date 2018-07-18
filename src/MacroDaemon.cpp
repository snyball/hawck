#include <thread>
#include <iostream>
#include "MacroDaemon.hpp"
#include "LuaUtils.hpp"

using namespace Lua;
using namespace std;

static const char *const evval[] = {
    "UP",
    "DOWN",
    "REPEAT"
};
static const char *event_str[EV_CNT];

static inline void initEventStrs()
{
    event_str[EV_SYN      ] = "SYN"       ;
    event_str[EV_KEY      ] = "KEY"       ;
    event_str[EV_REL      ] = "REL"       ;
    event_str[EV_ABS      ] = "ABS"       ;
    event_str[EV_MSC      ] = "MSC"       ;
    event_str[EV_SW       ] = "SW"        ;
    event_str[EV_LED      ] = "LED"       ;
    event_str[EV_SND      ] = "SND"       ;
    event_str[EV_REP      ] = "REP"       ;
    event_str[EV_FF       ] = "FF"        ;
    event_str[EV_PWR      ] = "PWR"       ;
    event_str[EV_FF_STATUS] = "FF_STATUS" ;
    event_str[EV_MAX      ] = "MAX"       ;
}

MacroDaemon::MacroDaemon() : kbd_srv("kbd.sock") {
    initEventStrs();
    // Keep looping around until we get a connection.
    for (;;) {
        try {
            int fd = kbd_srv.accept();
            kbd_com = new UNIXSocket<KBDAction>(fd);
            break;
        } catch (SocketError &e) {
            ;
        }
        usleep(100);
    }
    remote_udev = new RemoteUDevice(kbd_com);
    auto s = new Script("default.hwk");
    s->open(remote_udev, "udev");
    scripts.push_back(s);
}

MacroDaemon::~MacroDaemon() {
    for (Script *s : scripts) {
        delete s;
    }
    delete remote_udev;
}

void MacroDaemon::run() {
    KBDAction action;
    struct input_event &ev = action.ev;
    // auto *sys_com = new JSONChannel(kbd_srv.accept());
    lua_State *L = scripts[0]->getL();

    for (;;) {
        bool repeat = true;

        cout << "Receiving ..." << endl;
        kbd_com->recv(&action);
        cout << "Received keyboard action ." << endl;
        fflush(stdout);

        const char *ev_val = (ev.value <= 2) ? evval[ev.value] : "?";
        const char *ev_type = event_str[ev.type];

        fprintf(stderr, "REPEAT=%d ON %s: %d %d\n", repeat, event_str[ev.type], ev.value, (int)ev.code);
        fflush(stderr);

        lua_pushlstring(L, ev_type, strlen(ev_type));
        lua_setglobal(L, "__event_type");
        lua_pushinteger(L, ev.type);
        lua_setglobal(L, "__event_type_num");
        lua_pushinteger(L, ev.code);
        lua_setglobal(L, "__event_code");
        lua_pushlstring(L, ev_val, strlen(ev_val));
        lua_setglobal(L, "__event_value");
        lua_pushinteger(L, ev.value);
        lua_setglobal(L, "__event_value_num");

        lua_getglobal(L, "__match");
        if (!Lua::isCallable(L, -1)) {
            fprintf(stderr, "ERROR: Unable to retrieve __match function from Lua state\n");
        } else if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
            fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
        } else {
            repeat = !lua_toboolean(L, -1);
        }

        if (ev.type == EV_KEY && ev.value != 2) {
            fprintf(stderr, "REPEAT=%d ON %s: %d %d\n", repeat, event_str[ev.type], ev.value, (int)ev.code);
            fflush(stderr);
        }

        lua_pop(L, 1);
        
        if (repeat)
            remote_udev->emit(&ev);

        remote_udev->done();
    }
}

