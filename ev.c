#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <sys/time.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* FIXME: Sticky key
 *
 * If a key (like enter) is pressed as the program is started, the desktop environment
 * does not seem to receive the key-up properly. The result is that the key becomes unusable
 * while Hawck is running.
 *
 * The bug can also be reproduced with the following code:
 *   match[ up + key "s" ] = function()end
 *
 * When the 's' key is pressed the key-up will be hidden from the desktop environment
 * and the key becomes unusable no matter how many times it is pressed. This one should
 * be fixable by just faking another key-up event. However,
 *
 * Faking key-ups does not seem to work with keys that were originally received from a
 * physical keyboard. The problem seems to be that DEs like GNOME expect the key-up to
 * come from the same device as the key-down. This is illustrated by the following:
 *   Press and hold key on keyboard 0
 *   Press and hold the same key on keyboard 1
 *   Release key on either keyboard and the key will still be repeated.
 * This is what indicates that GNOME expects the key-up to come from the same keyboard.
 *
 * Workaround:
 *   When starting up, wait for a keypress using select()
 *   When a key is pressed, wait for the keyup.
 *   When the SYN for the keyup is received, lock the keyboard.
 * This should resolve the bug.
 */ 

/* TODO: Special Hawck syntax for Lua scripts.
 *
 * The following syntactical sugar should be implemented:
 *   key "a" => action
 *   Translates to:
 *   __match[ key "a" ] = action
 *
 *   [ key "a" ] => action
 *   Translates to:
 *   __match[ key "a" ] = action
 *
 * These replacements are performed when a script is loaded.
 */

/* TODO: Killswitch keys
 *
 * There should be a sequence of keys that- no matter what- will either kill
 * the current Hawck action, or bring down the whole daemon, releasing the keyboard.
 *
 * Proposals:
 *   Ctrl-Alt-Backspace
 *   Ctrl-Alt-End
 *
 * How to implement it:
 *   
 */ 

/* TODO: Multiple Lua scripts
 *
 * ev should run as a daemon, and should be able to receive commands
 * to load Lua scripts or just add a new key-matching pattern.
 *
 * How commands should be received:
 *   D-Bus:
 *     Should be easy to work with on the client side.
 *   FIFO:
 *     Requires thinking more about protocol, but will
 *     be supported everywhere.
 *   TCP/IP:
 *     Bad idea.
 *
 * What commands to implement:
 *   loadScript(path :: string, name :: string)
 *   exec(script :: string, code :: string)
 *   stopScript(script :: string)
 *   addKeyboard(path :: string)
 *   removeKeyboard(path :: string)
 *   // Stop listening for events, free the keyboard
 *   stop()
 *   // Start again
 *   start()
 *   // Stop daemon entirely
 *   kill()
 *     
 * A Python script will be written for controlling the daemon.
 * It should be called "haw" and should work like this
 *   haw --load=path/to/<script-name>.hwk
 *   haw --stop
 *   haw --stop-script=script-name
 *   haw --start
 *   haw --kill
 *   haw --exec="print('hello')" --script=script-name
 *   haw --add-kbd=/path/to/kbd/device
 *   haw --rm-kbd=/path/to/kbd/device
 */

/* TODO: Report errors
 *
 * Lua errors should be reported with a dialog box, the associated
 * event should be disabled (to avoid spam.)
 * 
 */

/* TODO: More Lua utility functions
 *
 * Working with desktop files: 
 *   app("firefox"):run("new-window")
 * Launching scripts:
 *   run("special-script.sh", arg0, arg1)
 * Forking shell:
 *   cmd("echo dis && echo dat")
 */

/* TODO: Build/install scripts and packaging
 *
 * Package the program for:
 *   Debian/Ubuntu (ppa)
 *   Arch Linux (AUR)
 */

/* TODO: Security
 *
 * The user running ev needs to be a part of the input group in order
 * to read from the keyboard. Some might consider this a security risk,
 * particularly Wayland devs/proponents.
 *
 * Here's how everything should be split up:
 *   Scripts are owned by root and require sudo to replace, they are
 *   globally readable however. Stored in /usr/share/hawck/scripts/,
 *   with libraries stored in /usr/share/hawck/libs/
 *   installed using:
 *     sudo haw --install-script /path/to/script.hwk
 *     sudo haw --install-library /path/to/library.lua
 *
 *   ev:
 *     Runs under ev-listen user, which is part of the input group
 *     Loads scripts and checks for which keys should be passed on.
 *     Exposes keyboards in FIFOs like this:
 *       /tmp/ev-listen/kbd0
 *       /tmp/ev-listen/kbd1
 *     Has UDev for echoing back keys that are not on the whitelist.
 *     With the group ev-connect, writes binary data directly from
 *     the keyboard (but with whitelisting of keys.)
 *   Hawck:
 *     Started as root, will add the ev-connect group and drop priviliges
 *     to user mode.
 *     If started as a regular user, it will assume that it can read
 *     from the keyboard by itself.
 *     Runs under the desktop user.
 *     Has UDev for echoing back keys that did not match any patterns.
 *     Loads scripts just like ev.
 *     Listens for keypresses that come from ev, gives them to Lua,
 *     and runs __event() to see if it should echo the key back.
 *
 * Effectively this means that super-user privilges are required
 * to decide which keys are allowed to get intercepted. But the
 * scripts actually run under the user like he/she would expect.
 *
 * There should be a way to disable all of this to make the system
 * easier to work with.
 *
 * Scripts can opt to receive ALL keys by declaring the following:
 *   receive "*"
 *
 * One problem with all of this is increased latency when pressing
 * keys, which further emphasizes the need for this additional
 * security to be optional.
 *
 * Another alternative is to create a shared memory segment using
 * mmap and then fork/exec, that might make it very viable. This
 * solution also avoids having two separate executables.
 *
 * It should be started like:
 *   sudo hawck
 * Then the process will create a shared memory segment for IPC
 * and fork twice into the event listener and the event evaluator.
 * The listener will run under the ev-listen user and the event
 * evaluator will run under the desktop user, no events are filtered,
 * everything is passed through as we are relying on the scripts
 * not doing anything phishy. In this model events don't really need
 * to be filtered as we are trusting that scripts are stored with
 * mode 644 and are owned by the root user.
 *
 * Filtering could still be implemented purely for the sake of
 * performance.
 *
 * Now the user can also configure hawck to load scripts from
 * ~/.local/share/hawck/scripts/ to avoid having to use sudo for installing
 * scripts, though this will leave the user open to keylogging
 * hawck scripts being installed it is still very useful for quick
 * development of hawck scripts.
 *
 * This should be enabled/disabled with:
 *   sudo haw --enable-user-scripts
 *   sudo haw --disable-user-scripts
 * Which writes to a 644 FIFO owned by root that hawck listens on.
 * 
 * --disable-user-scripts will unload all user scripts.
 * --enable-user-scripts will allow user scripts to be loaded.
 */

/* TODO: Automatically reload scripts on changes.
 *
 * Use inotify to automatically reload scripts from /usr/share/hawck/scripts
 * and (if enabled) ~/.local/share/hawck/scripts/
 */

/* TODO: The working directory of scripts
 *
 * Should be ~/.local/share/hawck/
 *
 * Now the user can place symlinks to any utility shell-scripts inside
 * there and run them easily with run("script.sh")
 */

#define PRINT_EVENTS 0

static const char *const evval[] = {
    "UP",
    "DOWN",
    "REPEAT"
};

static const char *event_str[EV_CNT];

static const size_t evbuf_start_len = 128;

typedef struct _UDevice {
    int fd;
    struct uinput_setup usetup;
    size_t evbuf_len;
    size_t evbuf_top;
    struct input_event *evbuf;
} UDevice;

void initEventStrs()
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

UDevice *newUDevice()
{
    UDevice *dev = malloc(sizeof(UDevice));
    if (dev == NULL) {
        fprintf(stderr, "Unable to allocate memory");
        abort();
    }

    dev->fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    if (dev->fd < 0) {
        fprintf(stderr, "Unable to open /dev/uinput: %s\n", strerror(errno));
        free(dev);
        return NULL;
    }

    /*
     * The ioctls below will enable the device that is about to be
     * created, to pass key events, in this case the space key.
     */
    ioctl(dev->fd, UI_SET_EVBIT, EV_KEY);
    for (int key = KEY_ESC; key <= KEY_MICMUTE; key++) {
        ioctl(dev->fd, UI_SET_KEYBIT, key);
    }

    memset(&dev->usetup, 0, sizeof(dev->usetup));
    dev->usetup.id.bustype = BUS_USB;
    dev->usetup.id.vendor = 0x1234; /* sample vendor */
    dev->usetup.id.product = 0x5678; /* sample product */
    strcpy(dev->usetup.name, "HawcK");

    ioctl(dev->fd, UI_DEV_SETUP, &dev->usetup);
    ioctl(dev->fd, UI_DEV_CREATE);

    if (!(dev->evbuf = calloc(dev->evbuf_len = evbuf_start_len, sizeof(dev->evbuf[0])))) {
        fprintf(stderr, "Unable to allocate memory");
        abort();
    }
    dev->evbuf_top = 0;

    return dev;
}

void destroyUDevice(UDevice *udev)
{
    ioctl(udev->fd, UI_DEV_DESTROY);
    close(udev->fd);
    free(udev->evbuf);
    free(udev);
}

void deviceEmit(UDevice *dev, int type, int code, int val)
{
    if (dev->evbuf_top >= dev->evbuf_len &&
        !(dev->evbuf = realloc(dev->evbuf, (dev->evbuf_len *= 2) * sizeof(dev->evbuf[0]))))
    {
        fprintf(stderr, "Unable to allocate memory");
        abort();
    }

    struct input_event *ie = &dev->evbuf[dev->evbuf_top++];

    ie->type = type;
    ie->code = code;
    ie->value = val;

    /* fprintf(stderr, "%s: %d %d\n", event_str[ie->type], ie->value, (int)ie->code); */
    /* fflush(stderr); */

    /* timestamp values below are ignored */
    gettimeofday(&ie->time, NULL);
}

void deviceFlush(UDevice *udev)
{
    if (udev->evbuf_top == 0)
        return;
    write(udev->fd, udev->evbuf, sizeof(udev->evbuf[0]) * udev->evbuf_top);
    udev->evbuf_top = 0;
}

#undef lua_istable
static int lua_istable(lua_State *L, int idx) {
    return lua_type(L, idx) == LUA_TTABLE;
}

#undef lua_isboolean
static int lua_isboolean(lua_State *L, int idx) {
    return lua_type(L, idx) == LUA_TBOOLEAN;
}

#undef lua_isnil
static int lua_isnil(lua_State *L, int idx) {
    return lua_type(L, idx) == LUA_TNIL;
}

int lua_emit(lua_State *L) {
    if (lua_gettop(L) != 3 || !lua_isnumber(L, -1) || !lua_isnumber(L, -2) || !lua_isnumber(L, -3)) {
        luaL_error(L, "Invalid arguments, expected (int type, int code, int val)");
    }
    UDevice *dev = lua_touserdata(L, lua_upvalueindex(1));
    deviceEmit(dev, lua_tonumber(L, -3), lua_tonumber(L, -2), lua_tonumber(L, -1));
    deviceFlush(dev);
    return 0;
}

/** Figure out if a Lua value is callable.
 *
 * It is not sufficient to just use lua_isfunction in certain cases,
 * as that won't handle the cases where the value is actually a
 * callable table. I.e a table that has a __call meta-method, will also
 * handle nested __call metamethods, i.e cases where a __call metamethod
 * is pointing to another table with a __call metamethod and so on.
 *
 * Warning: In case of deeply nested __call metamethods the function
 *          will throw an error. A soft maximum depth of 100 is used,
 *          but in case of low memory it can also fail due to the Lua
 *          stack no longer being resizeable (on most systems this will
 *          cause a segfault.)
 */
static bool _luaIsCallableHelper(lua_State *L, int idx, int depth)
{
    static const int soft_max_depth = 10;

    if (!lua_checkstack(L, 2)) {
        luaL_error(L, "Maximum recursion depth exceeded (%s)", __func__);
    } else if (depth > soft_max_depth) {
        luaL_error(L, "Unable to determine if function is callable, __call metamethods nested beyond %d layers (%s)", soft_max_depth, __func__);
    }

    if (lua_isfunction(L, idx))
        return true;
    lua_getmetatable(L, idx);
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    lua_getfield(L, -1, "__call");

    // Nested __call metamethods are possible
    bool is_callable = _luaIsCallableHelper(L, -1, depth+1);
    lua_pop(L, 2);

    return is_callable;
}

static bool luaIsCallable(lua_State *L, int idx)
{
    return _luaIsCallableHelper(L, idx, 0);
}

int main(int argc, char *argv[])
{
    initEventStrs();

    if (argc < 2) {
        fprintf(stderr, "Usage: ev <input device>\n");
        return EXIT_FAILURE;
    }

    const char *dev = argv[1];
    struct input_event ev;
    ssize_t n;
    int fd;

    UDevice *udev = newUDevice();
    if (udev == NULL) {
        fprintf(stderr, "Unable to create new udevice\n");
        return EXIT_FAILURE;
    }

    fd = open(dev, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
        return EXIT_FAILURE;
    }

    int grab = 1;
    ioctl(fd, EVIOCGRAB, &grab);

    fprintf(stderr, "Started.\n");

    bool repeat = true;

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    lua_pushlightuserdata(L, (void *)udev);
    lua_pushcclosure(L, lua_emit, 1);
    lua_setglobal(L, "__emit");
    if (luaL_loadfile(L, "./default.hwk") != LUA_OK) {
        fprintf(stderr, "Unable to load Lua script");
        return EXIT_FAILURE;
    }
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        const char *err = lua_tostring(L, -1);
        fprintf(stderr, "Error encountered while loading script: %s\n", err);
        lua_pop(L, 1);
        return EXIT_FAILURE;
    }

    for (;;) {
        if ((n = read(fd, &ev, sizeof(ev))) != sizeof(ev)) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            continue;
        }

        const char *ev_val = (ev.value <= 2) ? evval[ev.value] : "?";
        const char *ev_type = event_str[ev.type];

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
        if (!luaIsCallable(L, -1)) {
            fprintf(stderr, "ERROR: Unable to retrieve __match function from Lua state");
        }

        repeat = true;
        if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
            fprintf(stderr, "Lua error: %s\n", lua_tostring(L, -1));
        } else {
            repeat = !lua_toboolean(L, -1);
        }
        lua_pop(L, 1);

#if PRINT_EVENTS
        if (repeat) {
            fprintf(stderr, "%s: %d %d\n", event_str[ev.type], ev.value, (int)ev.code);
            fflush(stderr);
        }
#endif
        
        if (repeat)
            write(udev->fd, &ev, sizeof(ev));

        deviceFlush(udev);
    }

    ioctl(fd, EVIOCGRAB, NULL);
    close(fd);

    return EXIT_SUCCESS;
}

