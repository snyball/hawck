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
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS     OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */
#include <thread>
#include <iostream>
#include "MacroDaemon.hpp"
#include "LuaUtils.hpp"

extern "C" {
    #include <glib.h>
    #include <libnotify/notify.h>
    #include <unistd.h>
}

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
    notify_init("Hawck");
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

void MacroDaemon::notify(string title, string msg) {
    NotifyNotification *n;
    string ntitle = "Hawck: " + title;
    n = notify_notification_new(ntitle.c_str(), msg.c_str(), nullptr);
    notify_notification_set_timeout(n, 8000);
    notify_notification_set_urgency(n, NOTIFY_URGENCY_CRITICAL);
    if (!notify_notification_show(n, nullptr)) {
        fprintf(stderr, "Failed to show notification: %s", msg.c_str());
    }
    g_object_unref(n);
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
            notify("Lua error", "Unable to retrieve __match function from Lua state\n");
        } else if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
            string err(lua_tostring(L, -1));
            notify("Lua error", err);
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

