# Hawck
### A key-rebinding daemon

Hawck runs in user space and uses kernel-apis to intercept
key presses. It can run key-presses through Lua scripts in
order to modify the behaviour of keys.

This can mean simple rebindings, launching of programs, or
really anything else.

### GUI

A simple gui is planned so that simple cases can be covered
for normal users, while more advanced users may write scripts
instead.

### WARNING: Security

The program currently contains only the basic functionality and
should be seen as a prototype. Security features to protect the
user from malicious software is planned for the 1.0 release.

Under no circumstances run the daemon as root

To try it out you currently have to add your user to the input group, which also
happens to be a bad idea.

If you are using X11 your key-presses can already be intercepted without any
special permission, so adding yourself to the input group should not pose any
additional risk; this is mostly a warning for Wayland-users (the group this
software is targetting.)

### NOTICE: This software is in alpha stage

I personally use the software daily without any issues, but
I do not recommend anyone to actually auto-start this software
on boot, there may be a few bugs, and in the case of auto-starting
the daemon with SystemD on boot these problems may render the computer
unusable until a system recovery is performed or another keyboard
is plugged in (hawck only listens to a single keyboard at a time.)
