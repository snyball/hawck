//
// Mostly taken from examples given here:
//   https://www.kernel.org/doc/html/v4.12/input/uinput.html
//

#include <functional>
#include <type_traits>
#include <iostream>

extern "C" {
    #include <unistd.h>
    #include <fcntl.h>
    #include <time.h>
    #include <dirent.h>
    #include <poll.h>
}

#include "SystemError.hpp"
#include "UDevice.hpp"
#include "utils.hpp"

const char hawck_udev_name[] = "Hawck virtual keyboard device";

using namespace std;

static int getDevice(const string &by_name) {
    char buf[256];
    string devdir = "/dev/input";
    DIR *dir = opendir(devdir.c_str());
    if (dir == nullptr)
        throw SystemError("Unable to open directory: ", errno);

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        int fd, ret;
        sstream ss;
        ss << devdir << "/" << entry->d_name;
        fd = open(ss.str().c_str(), O_RDWR | O_CLOEXEC | O_NONBLOCK);
        // fd = open(ss.str().c_str(), O_RDONLY);
        if (fd < 0)
            continue;

        ret = ioctl(fd, EVIOCGNAME(sizeof(buf)), buf);
        string name(ret > 0 ? buf : "");
        if (name == by_name)
            return fd;
        close(fd);
    }

    return -1;
}

UDevice::UDevice() {
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    if (fd < 0) {
        fprintf(stderr, "Unable to open /dev/uinput: %s\n", strerror(errno));
        throw std::exception();
    }

    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    for (int key = KEY_ESC; key <= KEY_MICMUTE; key++)
        ioctl(fd, UI_SET_KEYBIT, key);

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234; /* sample vendor */
    usetup.id.product = 0x5678; /* sample product */
    strcpy(usetup.name, hawck_udev_name);

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);

    evbuf = (input_event*)calloc(evbuf_len = evbuf_start_len,
                                 sizeof(evbuf[0]));
    if (!evbuf) {
        fprintf(stderr, "Unable to allocate memory");
        abort();
    }
    evbuf_top = 0;

    int errors = 0;
    while ((dfd = getDevice(usetup.name)) < 0) {
        if (errors++ > 10)
            throw SystemError("Unable to get file descriptor of udevice.");
        cout << "Unable to get file descriptor of udevice, retrying ..." << endl;
        usleep(10000);
    }
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
    if (evbuf_top == 0)
        return;

    input_event *bufp = evbuf;
    for (size_t i = 0; i < evbuf_top; i++) {
        if (write(fd, bufp++, sizeof(*bufp)) != sizeof(*bufp))
            throw SystemError("Error in write(): ", errno);

        // FIXME FIXME FIXME: This shit is ridiculous but I can't find any documentation
        //                    on how to handle this.
        // Quick-fix until I manage to receive events when keys are pressed
        // too fast
        // UPDATE: The bug is specific to GNOME Wayland, no SYN_DROPPED messages
        // are sent back on the virtual device. Wayland X11 applications work
        // just fine.
        // TTYs also seem to not be dropping any keys.
        // GNOME Wayland applications will drop modifier keys resulting in borked
        // macros.
        // UPDATE: XWayland also drops keys, but just not as badly as with
        //         Wayland clients. I'll have to test this on X11.
        //         X clients are fine with 500µs of sleep between events,
        //         while Wayland clients need 3800µs between events.
        //         It's also worth noting that this is a problem that
        //         appeared quite suddenly, probably as a result of
        //         an update.
        //         There appears to be nothing I can do on my end,
        //         the clients need to handle SYN_DROPPED events and
        //         they don't seem to be doing that properly.

        // usleep(500);
        usleep(3800);
        // usleep(13600);
    }


    #if 0
    struct pollfd pfd;
    cout << "BEGIN RECV" << endl;
    for (;;) {
        pfd.fd = dfd;
        pfd.events = POLLIN;
        // Wait for something to read for 16ms
        switch (poll(&pfd, 1, 16)) {
            case -1:
                // throw SystemError("Error in poll(): ", errno);
                cout << "Error in poll()" << endl;
                break;

            case 0:
                // Timeout, skip events for now.
                cout << "poll() on loopback kbd timed out." << endl;
                goto loop_end;

            default: {
                input_event ev;
                if (read(dfd, &ev, sizeof(ev)) != sizeof(ev))
                    throw SystemError("Unable to receive event");
                cout << "Received event" << endl;
                if (ev.code == SYN_DROPPED && ev.type == EV_SYN)
                    cout << "DROPPED" << endl;
                cout << "ev.code: " << ev.code << endl;
                cout << "ev.type: " << ev.type << endl;
                cout << "ev.value: " << ev.value << endl;
            }
        }
    } loop_end:
    cout << "END RECV" << endl << endl ;
    #endif

    evbuf_top = 0;
}

void UDevice::done() {
    flush();
}
