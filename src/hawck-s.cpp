/* =====================================================================================
 * Hawck
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
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */

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
    #include <stdio.h>
}
#include <string.h>
#include <stdio.h>
#include <stdexcept>
#include <exception>
#include <iostream>
#include <memory>
#include "UDevice.hpp"
#include "Keyboard.hpp"
#include "MacroDaemon.hpp"
#include "KBDDaemon.hpp"

using namespace std;

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
 * 
 * FIXME: UPDATE
 * 
 * The workaround has now been implemented inside `Keyboard::lock` and is working
 * as intended for single keypresses.
 */ 

/** TODO: Compile for all of luajit, lua5.1, lua5.2, and lua5.3
 *
 * In the case of luajit, optimize all calls using the LuaJIT FFI.
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
 *
 * OK, new approach:
 *
 *   Because of the /proc/ filesystems the approaches above just
 *   wont work. Disabling /proc is unrealistic as too many things
 *   depend on it. Here is the new approach:
 *
 *   When a script is loaded it is loaded for both the priviliged
 *   reader and the user-level end. The priviliged reader only
 *   runs match() to get an index of which action to take, it then
 *   passes this action to the user mode daemon. This should work
 *   pretty seamlessly. When a match is found the key event information
 *   is also passed, but all of this means that the key event is
 *   only passed in cases where there was an actual match. This makes
 *   installing keyloggers impossible without root access as the scripts
 *   are still owned by root. In this model both daemons have to maintain
 *   their own UDevice.
 *
 *   The two processes still communicate via a shared memory mapping.
 *
 *   The optional feature of disabling all this will still be present.
 *   As will the `haw --enable-user-scripts` option.
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

/**
 * Change stdout and stderr to point to different files, specified
 * by the given paths.
 *
 * Stdin will always be redirected from /dev/null, if /dev/null cannot
 * be open this will result in abort(). Note that after this call any
 * blocking reads on stdin will halt the program.
 */
void dup_streams(string stdout_path, string stderr_path) {
    using CFile = unique_ptr<FILE, decltype(&fclose)>;
    auto fopen = [](string path, string mode) -> CFile {
                     return CFile(::fopen(path.c_str(), mode.c_str()), fclose);
                 };

    // Attempt to open /dev/null
    auto dev_null = fopen("/dev/null", "r");
    if (dev_null == nullptr) {
        abort();
    }

    // Open new output files
    auto stdout_new = fopen(stdout_path, "w");
    auto stderr_new = fopen(stderr_path, "w");
    if (stdout_new == nullptr || stderr_new == nullptr) {
        throw invalid_argument("Unable to open new stdout and stderr");
    }

    // Dup files
    ::dup2(dev_null->_fileno, STDIN_FILENO);
    ::dup2(stdout_new->_fileno, STDOUT_FILENO);
    ::dup2(stderr_new->_fileno, STDERR_FILENO);
}
    
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: hawck <input device>\n");
        return EXIT_FAILURE;
    }

    const char *dev = argv[1];

    // In priviliged (root) process, fork and switch users around.

    fprintf(stderr, "Forking ...");

    pid_t cpid;
    switch (cpid = fork()) {
        case -1:
            fprintf(stderr, "FATAL: Unable to fork()");
            abort();
        break;

        // Child process becomes Lua code executor, runs as the normal user.
        case 0:
            // In child process.
        {
            // dup_streams("lua_stdout.log", "lua_stderr.log");
            try {
                MacroDaemon daemon;
                daemon.run();
            } catch (exception &e) {
                cout << e.what() << endl;
                exit(1);
            }
        }
        break;
        
        // Parent switches uid to keyboard-grabbing user. Has access to all input.
        default:
        {
            // dup_streams("kbd_stdout.log", "kbd_stderr.log");
            try {
                KBDDaemon daemon(dev);
                daemon.run();
            } catch (exception &e) {
                cout << e.what() << endl;
                exit(1);
            }
        }
        break;
    }

    return EXIT_SUCCESS;
}
