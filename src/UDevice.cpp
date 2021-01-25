#include <functional>
#include <type_traits>
#include <iostream>

extern "C" {
    #include <sys/utsname.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <time.h>
    #include <dirent.h>
    #include <poll.h>
}

#include "SystemError.hpp"
#include "UDevice.hpp"
#include "utils.hpp"
#include <filesystem>
#include <regex>
#include <Version.hpp>

const char hawck_udev_name[] = "Hawck Virtual Keyboard";
static_assert(sizeof(hawck_udev_name) < UINPUT_MAX_NAME_SIZE,
              "Length of uinput device name is too long.");

using namespace std;
namespace fs = std::filesystem;

static int getDevice(const string &by_name) {
    char buf[NAME_MAX];
    string devdir = "/dev/input";

    for (auto &entry : fs::directory_iterator(devdir)) {
        int fd = open(entry.path().c_str(), O_RDWR | O_CLOEXEC | O_NONBLOCK);
        if (fd < 0)
            continue;

        int ret = ioctl(fd, EVIOCGNAME(sizeof(buf)), buf);
        string name(ret > 0 ? buf : "");
        if (name == by_name)
            return fd;
        close(fd);
    }

    return -1;
}

UDevice::UDevice()
    : LuaIface(this, UDevice_lua_methods)
{
    // UDevice initialization taken from this guide:
    //   https://www.kernel.org/doc/html/v4.12/input/uinput.html

    if ((fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0)
        throw SystemError("Unable to open /dev/uinput: ", errno);

    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
        throw SystemError("Unable to set event bit", errno);

    // Hawck might have been compiled with keys that are not supported on the
    // kernel on which it runs.
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
    auto cur_ver = getLinuxVersionCode();
#endif
    int newer_keys = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
    if (cur_ver < KERNEL_VERSION(5, 10, 0))
        newer_keys += 4;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
    if (cur_ver < KERNEL_VERSION(5, 6, 0))
        newer_keys += 1;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
    if (cur_ver < KERNEL_VERSION(5, 5, 0))
        newer_keys += 37;
#endif

    syslog(LOG_INFO, "Older kernel version, popping %d key(s).",
           newer_keys);

    for (int i = 0; i < ((int) ALL_KEYS.size()) - newer_keys; i++)
        if (ioctl(fd, UI_SET_KEYBIT, ALL_KEYS[i]) < 0)
            throw SystemError("Unable to set key bit", errno);

    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x420b;
    usetup.id.product = 0x1a2e;
    strncpy(usetup.name, hawck_udev_name, UINPUT_MAX_NAME_SIZE);

    if (ioctl(fd, UI_DEV_SETUP, &usetup) < 0)
        throw SystemError("Unable to create udevice", errno);
    if (ioctl(fd, UI_DEV_CREATE) < 0)
        throw SystemError("Unable to create udevice", errno);

    // FIXME: HACK: For some reason the `fd` from open() cannot be used in
    // `upAll()` or anything else that requires retrieving the key states. But
    // opening up a second file-handle to the device works.
    int errors = 0;
    while ((dfd = getDevice(usetup.name)) < 0) {
        if (errors++ > 100)
            throw SystemError("Unable to get file descriptor of udevice.");
        usleep(1000);
    }
}    

UDevice::~UDevice() {
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    close(dfd);
}

void UDevice::emit(const input_event *send_event) {
    events.push_back(*send_event);
}

void UDevice::emit(int type, int code, int val) {
    struct input_event ev;
    memset(&ev, '\0', sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = val;
    events.push_back(ev);
}

void UDevice::flush() {
    for (struct input_event& ev : this->events) {
        if (write(fd, &ev, sizeof(ev)) != sizeof(ev))
            throw SystemError("Error in write(): ", errno);
        usleep(ev_delay);
    }
    this->events.clear();
}

void UDevice::done() {
    flush();
}

void UDevice::setEventDelay(int delay) {
    ev_delay = delay;
}

void UDevice::upAll() {
    unsigned char key_states[KEY_MAX/8 + 1];
    memset(key_states, 0, sizeof(key_states));
    if (ioctl(dfd, EVIOCGKEY(sizeof(key_states)), key_states) == -1)
        throw SystemError("Unable to get key states: ", errno);
    int key = 0;
    for (size_t i = 0; i < sizeof(key_states); i++)
        // Shift bits to count downed keys.
        for (unsigned int bs = 1; bs <= 128; bs <<= 1) {
            if (bs & key_states[i]) {
                emit(EV_KEY, key, 0);
                emit(EV_SYN, 0, 0);
            }
            key++;
        }
    flush();
}

LUA_CREATE_BINDINGS(UDevice_lua_methods)
