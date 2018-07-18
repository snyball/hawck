#include "RemoteUDevice.hpp"

RemoteUDevice::RemoteUDevice(UNIXSocket<KBDAction> *conn)
    : LuaIface(this, RemoteUDevice_lua_methods) {
    this->conn = conn;
}

RemoteUDevice::~RemoteUDevice() {}

void RemoteUDevice::emit(int type, int code, int val) {
    KBDAction ac;
    ac.ev.type = type;
    ac.ev.code = code;
    ac.ev.value = val;
    ac.done = 0;
    evbuf.push_back(ac);
}

void RemoteUDevice::emit(const input_event *send_event) {
    KBDAction ac;
    memcpy(&ac.ev, send_event, sizeof(*send_event));
    ac.done = 0;
    evbuf.push_back(ac);
}

void RemoteUDevice::flush() {
    conn->send(evbuf);
    evbuf.clear();
}

void RemoteUDevice::done() {
    flush();
    KBDAction ac;
    ac.done = 1;
    conn->send(&ac);
}

LUA_CREATE_BINDINGS(RemoteUDevice_lua_methods)
