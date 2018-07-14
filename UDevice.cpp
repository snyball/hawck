//
// Mostly taken from examples given here:
//   https://www.kernel.org/doc/html/v4.12/input/uinput.html
//

#include "UDevice.hpp"
#include <functional>
#include <type_traits>

extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
}

UDevice::UDevice() : LuaIface(this, UDevice_lua_methods) {
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    if (fd < 0) {
        fprintf(stderr, "Unable to open /dev/uinput: %s\n", strerror(errno));
        throw std::exception();
    }

    /*
     * The ioctls below will enable the device that is about to be
     * created, to pass key events, in this case the space key.
     */
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (int key = KEY_ESC; key <= KEY_MICMUTE; key++)
        ioctl(fd, UI_SET_KEYBIT, key);

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234; /* sample vendor */
    usetup.id.product = 0x5678; /* sample product */
    strcpy(usetup.name, "Hawck");

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);

    evbuf = (input_event*)calloc(evbuf_len = evbuf_start_len,
                                 sizeof(evbuf[0]));
    if (!evbuf) {
        fprintf(stderr, "Unable to allocate memory");
        abort();
    }
    evbuf_top = 0;
}    

UDevice::~UDevice() {
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    free(evbuf);
}

void UDevice::emit(const input_event *send_event) {
    if (evbuf_top >= evbuf_len &&
        !(evbuf = (input_event*)realloc(evbuf,
                                        (evbuf_len *= 2) * sizeof(evbuf[0]))))
    {
        fprintf(stderr, "Unable to allocate memory: evbuf_len=%ld, evbuf_top=%ld\n", evbuf_len, evbuf_top);
        abort();
    }

    input_event *ie = &evbuf[evbuf_top++];

    ie->time.tv_sec = 0;
    ie->time.tv_usec = 0;
    gettimeofday(&ie->time, NULL);

    memcpy(ie, send_event, sizeof(*ie));
}

void UDevice::emit(int type, int code, int val) {
    input_event ie;
    ie.type = type;
    ie.code = code;
    ie.value = val;
    emit(&ie);
}

void UDevice::flush() {
    // if (evbuf_top == 0)
    //     ;
    // // 8 is a heuristic
    // else if (evbuf_top <= 8)
    //     write(fd, evbuf, sizeof(evbuf[0]) * evbuf_top);
    // else {
    // }

    input_event *bufp = evbuf;
    for (size_t i = 0; i < evbuf_top; i++) {
        write(fd, bufp++, sizeof(*bufp));
        // Quick-fix until I manage to receive events when keys are pressed
        // too fast
        usleep(3500);
    }
    evbuf_top = 0;
}

LUA_CREATE_BINDINGS(UDevice_lua_methods)
