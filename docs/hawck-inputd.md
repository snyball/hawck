% HAWCK-INPUTD(1) Version 0.7 | Keyboard Input Grabber

NAME
====

**hawck-inputd** — Keyboard Input Grabber

SYNOPSIS
========

| **hawck-macrod** [**OPTIONS**]... [**\--kbd-device** _kbd_]...

DESCRIPTION
===========

Listen to keyboard devices and pass whitelisted input over to hawck-macrod.

Hawck InputD works in two modes:

 1. Listening on all keyboards
   - The default, in this mode hawck-inputd will grab all keyboard devices when
     it starts up. It will also automatically grab the keyboard input of any
     keyboards that are plugged in while running.
 2. Listening on only a few specific keyboards
   - Hawck InputD works in this mode when it is given at least one keyboard with
     the **\--kbd-device**, as well as **\--no-hotplug**.
   - Keyboards that were initially provided using **\--kbd-device** are not
     affected by **\--no-hotplug**, they can still be removed and re-attached.

Options
-------

**-h**, **\--help**

:   Prints brief usage information.

**\--no-fork**

:   Causes hawck-inputd to not fork/daemonize to the background.

**-k**, **\--kbd-device** _device_

:   Select a keyboard device that should be listened for.

    This keyboard device should be a device file, these device files are
    available in the */dev/input/by-path* and */dev/input/by-id* directories.
    
**\--udev-event-delay** _µs_

:   The delay between keyboard events in µs.

    Changing this can be useful if you're dealing with a program or desktop
    environment that is dropping keys.
    
**\--socket-timeout** _ms_

:   Time until socket timeout in milliseconds.

    The socket connection in question is the one between **hawck-macrod** and **hawck-inputd**.

**-v**, **\--version**

:   Prints the current version number.

FILES
=====

*/var/lib/hawck-input/keys/*

:   contains whitelisted keys in csv format.

*/var/lib/hawck-input/kbd.sock*

:   Is the socket that InputD will connect to and send key events.

*/var/lib/hawck-input/pid*

:   Contains the pid of the currently running hawck-inputd daemon.

BUGS
====

See GitHub Issues: <https://github.com/snyball/Hawck/issues>

COPYRIGHT
======

Copyright (C) Jonas Møller 2018

Provided under the BSD 2-clause license.

SEE ALSO
========

**hawck-macrod(1)**, **hwk2lua(1)**

