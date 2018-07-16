# Hawck
## A key-rebinding daemon

Hawck runs in user space and uses kernel-apis to intercept key presses. It can
run key-presses through Lua scripts in order to modify the behaviour of keys.

This can mean simple rebindings, launching of programs, or really anything else.

The name is a portmanteau of "Hack" and "AWK", as the program started out as a
bit of a hack, and the scripts take inspiration from the pattern-matching style
of the AWK programming language (plus, hawks are pretty cool I guess.)

### Supported platforms

- Linux
  - Currently the only supported platform, as input-grabbing in this
    way is very platform specific.
    
If `UDevice.cpp` and `Keyboard.cpp` were to be ported everything
else should run just fine under Mac OS X or BSD.

### GUI

A simple gui is planned so that simple cases can be covered for normal users,
while more advanced users may write scripts instead.

### Security

When Hawck starts up it splits up into two daemons that communicate with
each other:

- Keyboard daemon
  - Runs under the 'hawck-grab' user and is part of the input group,
    letting it read from /dev/input/ devices.
  - Grabs keyboard input exclusively.
  - Knows which keys to pass over to the macro daemon
  - Controls a virtual keyboard that is used to emulate
    keypresses, this includes re-pressing keys that did
    not need to be handled by the macro daemon.
- Macro daemon
  - Runs under the desktop user.
  - Listens for keypresses sent from the keyboard daemon.
  - Passes received keypresses into Lua scripts in order to
    perform actions or conditionally modify the keys.
  - Potential output keys produced by the script are sent
    back to the keyboard daemon.
    - Inconsequential implementation details: Having two virtual
      keyboards operated by both daemon opens up an entirely new
      can of worms especially for modifier keys.
  
The keyboard daemon contains a whitelist of keys that the macro daemon
is allowed to see, this whitelist is derived from the Lua scripts used.
This means that the process run under the desktop user never sees all
keyboard input, this is important as the `/proc/` filesystem would
allow any process launched by the user to see all keyboard input if
it were not filtered.

If you are using X11 your key-presses can already be intercepted without any
special permission, so all this extra security is unnecessary, but this program
is primarily aimed towards Wayland users.

### Known Bugs:

- Outputting keys too quickly:
  - In macros that result in a lot of keys being outputted in
    a short amount of time some keys may be skipped, leading
    to inconsistencies in output.
  - Workaround: Wait for a few milliseconds between emulated keypresses,
    this seems to only be noticeable with macros that produce
    a **lot of output**.

### NOTICE: This software is in alpha stage

I personally use the software daily without any issues, but I do not recommend
anyone to actually auto-start this software on boot, there may be a few bugs,
and in the case of auto-starting the daemon with SystemD on boot these problems
may render the computer unusable until a system recovery is performed or another
keyboard is plugged in (hawck only listens to a single keyboard at a time.)
