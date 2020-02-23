% HAWCK-MACROD(1) Version 0.7 | Lua Keyboard Macro Executor

NAME
====

**hawck-macrod** — Hawck Macro Daemon

SYNOPSIS
========

| **hawck-inputd** [**OPTIONS**]... 

DESCRIPTION
===========

Listen for keys coming from InputD and run Lua scripts to modify the behaviour
of these keys.

Options
-------

**-h**, **\--help**

:   Prints brief usage information.

**\--no-fork**

:   Causes hawck-inputd to not fork/daemonize to the background.

**-v**, **\--version**

:   Prints the current version number.

FILES
=====

*\$XDG_DATA_DIR*
:    If you haven't changed this, then Hawck will use: *\$HOME/.local/share*

*\$XDG_RUNTIME_DIR*
:    Hawck expects this environment variable to be set.

*\$XDG_DATA_DIR/hawck/scripts*

:    Contains user hwk scripts, as well as the Lua files generated from
     them.

*\$XDG_DATA_DIR/hawck/scripts-enabled*

:    Contains symlinks to Lua scripts, putting Lua files in here with
     execute permissions will cause them to be loaded instantly by
     MacroD.
     
     MacroD will also automatically reload scripts when they change.

*\$XDG_RUNTIME_DIR/hawck/lua-comm.fifo*

:    FIFO that MacroD listens on, writes to this fifo should be a length (32 bit
     unsigned int) followed by Lua code. This Lua code can either query or set
     values in the `config' object.

*\$XDG_RUNTIME_DIR/hawck/json-comm.fifo*

:    FIFO that MacroD writes to, reads to this should be performed
     after a write to `lua-comm.fifo' and will result in a length
     (32 bit unsigned int) followed by a JSON object.

*\$XDG_DATA_DIR/hawck/cfg.lua*

:    Contains configuration options that can be set and queried
     from \$XDG_DATA_DIR/hawck/*-comm.fifo.

*\$XDG_DATA_DIR/hawck/macrod.log*

:    Misc. logs from MacroD, not meant for users. Use journalctl(1)
     or an alternative syslog viewer to view the MacroD logs.

*/var/lib/hawck-input/kbd.sock*

:    Is the socket that MacroD will listen on for connections from
     InputD.

BUGS
====

See GitHub Issues: <https://github.com/snyball/Hawck/issues>

COPYRIGHT
======

Copyright (C) Jonas Møller 2018

Provided under the BSD 2-clause license.

SEE ALSO
========

**hawck-inputd(1)**, **hwk2lua(1)**

