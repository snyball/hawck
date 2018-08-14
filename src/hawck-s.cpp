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

/** TODO: Compile for all of luajit, lua5.1, lua5.2, and lua5.3
 *
 * In the case of luajit, optimize all calls using the LuaJIT FFI.
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

int main(int argc, char *argv[])
{
    #if DANGER_DANGER_LOG_KEYS
    fprintf(stderr, "WARNING: This build has been compiled with DANGER_DANGER_LOG_KEYS, keys will be logged.");
    fprintf(stderr, "WARNING: If you did not compile this yourself to perform debugging, please refrain from using this build of hawck");
    #endif

    int pfd_kbd[2];
    int pfd_udev[2];

    if (argc < 2) {
        fprintf(stderr, "Usage: hawckd <input device>\n");
        return EXIT_FAILURE;
    }

    const char *dev = argv[1];

    // In priviliged (root) process, fork and switch users around.

    fprintf(stderr, "Forking ...");

    if (pipe(pfd_kbd) == -1 || pipe(pfd_udev) == -1)
        throw SystemError("Error in pipe(): ", errno);

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
            sleep(1);
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
