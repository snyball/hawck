<img src="./icons/hawck_logo_v3_with_text.svg"/>

![License: BSD-2](https://img.shields.io/badge/license-BSD--2-brightgreen.svg)
![Version: 0.7](https://img.shields.io/badge/version-0.7-blue.svg)

## A key-rebinding daemon

Linux with all it's combinations of window managers, display servers and
desktop environments needs a key-rebinding system that works everywhere.

Hawck intercepts key presses and lets you write Lua scripts to perform
actions or modify keys depending on your needs.

Your Lua scripts will work on Wayland, X11, and every WM/DE you throw at
them, as well as console ttys.

The ultimate goal of the project is to serve as a user-friendly Linux
alternative to AutoHotkey, but this time with a sane scripting language.

Common concrete use cases:

- Rebind caps lock to ctrl or escape:

```lua
key "Caps" => replace "Escape"
-- , or
key "Caps" => replace "Control"
```

- Conditionally replace caps lock (or any other replacement)

```lua
-- Pressing F7 will activate the replacement, and pressing F7
-- again will disable it.
mode("Caps => Ctrl mode", down + key "F7") => {
    key "Caps" => replace "Control"
}
```

- Replace a key, but only on a specific keyboard (not in `v0.6`, use master branch
  for this)

```lua
-- Run `src/tools/lskbd.rb -k` and look for "ID" in the output to find keyboard IDs.
fromkbd "1:1:AT_Translated_Set_2_keyboard" + key "F7" => say "Hello"
```

- Paste into a tty, or another program which does not support pasting

```lua
function getClipboard()
    -- get-clipboard should be replaced with whatever works with your setup.
    local p = io.popen("get-clipboard")
    local clip = p:read("*a")
    p:close()
    return clip
end

shift + alt + key "v" => function ()
  local clip_contents = getClipboard()
  write(clip_contents)() -- Note the extra parens, write() returns a closure
end
```

- Make non-us keyboards more convenient for programming:

```lua
-- Replace with your respective european characters
shift + key "ø" => insert "["
key "ø" => insert "{"
-- , or
shift + key "æ" => insert "]"
key "æ" => insert "}"
```

- Store a common phrase and activate it with a key-binding

```lua
local seals_pasta = "What the **** did you just say about me you little *****? I'll have you know I graduated top of my class from…"
shift + alt + key "p" => write(seals_pasta)
```

- Run .desktop application actions, and generally launch programs

```lua
shift + alt + key "f" => app("firefox"):new_window("http://man7.org/linux/man-pages/man1/yes.1.html")
shift + alt + key "w" => app("firefox"):new_private_window("https://www.youtube.com/watch?v=V4MF2s6MLxY")
```

<small>
The name is a portmanteau of "Hack" and "AWK", as the program started out as a
bit of a hack, and the scripts take inspiration from the pattern-matching style
of the AWK programming language (plus, hawks are pretty cool I guess.)
</small>

## Installation

For `v0.6`, click Branch\>Tags\>Master\>`v0.6.1` and follow the instructions in
the `README` files for that version. The rest of this section will apply to the
master branch.

### Arch/Manjaro

Install the [hawck-git](https://aur.archlinux.org/packages/hawck-git/) AUR package.

Remember to run `/usr/share/hawck/bin/hawck-user-setup` after install (you
should run this script with your personal user account, not as root.)

### Debian/Ubuntu

```bash
$ git clone --recurse-submodules -j8 https://github.com/snyball/Hawck.git
$ cd Hawck
$ pkexec xargs apt -y install < bin/dependencies/debian-deps.txt
$ ./install.sh
```

The user setup script will prompt you for your password, it has to add your user
to the `hawck-input-share` group. You'll have to log out and back in again after
your user has been added to the group (on some distributions, e.g Ubuntu, a
reboot is required.)

When you've started the computer back up again, run the following commands:

### Other distributions

Install all the dependencies listed on the
[AUR page](https://aur.archlinux.org/packages/hawck-git/).
Once you've found the appropriate packages for your distribution, run the
`install.sh` script. Note that the install script has only been tested on Ubuntu
19.10.

## Testing it out

As Hawck is not able to determine your Wayland/X11 keymap dynamically, you must
first set your keymap in `~/.local/share/hawck/cfg.lua`. This also applies to
us-keymap users, as the default is stubbornly set to the more uncommon "no"
(norwegian) because the author of Hawck is from there.
[FAQ: Keyboard Layout](#faq-layout) below.

```lua
  keymap = "no", -- set to "us", "de", "no-latin1", etc. depending on your keyboard layout
```

Now you can test out a simple script, your `example.hwk` could look like this:

```lua
-- Pressing C-a should insert the character "b"
down + ctrl + key "a" => insert "b"
```

> Tip: If you make a mistake, hold Escape and press the Spacebar (ESC-SPC)
> to deactivate all scripts. Repeat ESC-SPC to reactivate script processing.

```bash
# Work around issue with ACL (a better solution is WIP)
$ sudo setfacl -m 'g:uinput:rw' /dev/uinput
# Start services
$ sudo systemctl start hawck-inputd
$ hawck-macrod
# Create a new script
$ vim ~/.config/hawck/scripts/example.hwk
# Whitelist keys from the script
$ hawck-add ~/.config/hawck/scripts/example.hwk
# Enable the script (note: macrod specifically requires mode 0755)
$ chmod +x ~/.config/hawck/scripts/example.hwk
# Disable the script
$ chmod -x ~/.config/hawck/scripts/example.hwk
```

> Warning: When editing example.hwk, the macro daemon will auto-reload, but
> because of the [security model](#security), new keys will be ignored until
> you rerun `hawck-add`. You can check what keycodes are intercepted by the
> input daemon for our example script with:

```sh
$ cat /var/lib/hawck-input/keys/example.csv
```

## Security

As you can imagine, with all this power comes great responsibility. Here's
an explanation of the strategy used to make things a bit more secure.

When Hawck starts up it splits up into two daemons that communicate with
each other:

- InputD
  - Runs under the 'hawck-input' user and is part of the input group,
    letting it read from /dev/input/ devices.
  - Member of the hawck-uinput group, allowing it to use /dev/uinput
  - Grabs keyboard input exclusively.
  - Knows which keys to pass over to the macro daemon
  - Controls a virtual keyboard that is used to emulate
    keypresses, this includes re-pressing keys that did
    not need to be handled by the macro daemon.
- MacroD
  - Runs under the desktop user.
  - Listens for keypresses sent from the keyboard daemon.
  - Passes received keypresses into Lua scripts in order to
    perform actions or conditionally modify the keys.
  - Potential output keys produced by the script are sent
    back to the keyboard daemon.
    - Inconsequential implementation details: Having two virtual
      keyboards operated by both daemons opens up an entirely new
      can of worms especially for modifier keys.

<img src="./images/arch.svg"/>

The keyboard daemon contains a whitelist of keys that the macro daemon
is allowed to see, this whitelist is derived from the Lua scripts used.
This means that the process run under the desktop user never sees all
keyboard input, this is important as the `/proc/` filesystem would
allow any process launched by the user to see all keyboard input if
it were not filtered.

If you are using X11 your key-presses can already be intercepted without any
special permission, so all this extra security is unnecessary.

Read the alternative strategies FAQ section below for more information and a
more thorough explanation of the drawbacks with the current model.

### Scripting

Hawck is scripted with, you guessed it, hawk scripts. These scripts are
essentially just Lua with some extra operators. The hwk2lua program is
used to transpile `.hwk` files into `.lua` files (this happens automagically
if you use Hawck-UI.)

As an example, here is a hawk script:

```lua
-- Programming mode is activated by pressing down the f7 key.
-- It is only run when a key is not being released (-up)
mode("Programming mode", down + key "f7") + -up => {
    -- When caps-lock is pressed, substitute with escape
    key "caps" => insert "escape"
    shift => {
        key "ø" => insert "["
        key "æ" => insert "]"
    }
    key "ø" => insert "{"
    key "æ" => insert "}"
    -- Write "Hello " 10 times using the virtual keyboard,
    -- then show a "World!" notification
    key "F12" => (write "Hello ") * 10 .. say "World!"
}
```

With this script the `ø` character produces `{`, similarly
for the other bindings. But only if `"Programming mode"` is enabled.

This transpiles to the following (without comments):

```lua
__match[mode("Programming mode", down + key "f7") + -up ] = MatchScope.new(function (__match)
    __match[key "caps"] = insert "escape"
    __match[shift] = MatchScope.new(function (__match)
        __match[key "oslash"] = insert "bracketleft"
        __match[key "ae"    ] = insert "bracketright"
    end)
    __match[key "oslash"] = insert "braceleft"
    __match[key "ae"    ] = insert "braceright"
    __match[key "F12"   ] = (write "Hello ") * 10 .. say "World!"
end)
```

Which is what the `hawck-macrod` daemon actually runs. Note
that the only differences between plain Lua syntax and the
added Hawck-specific syntax is the `=>` operator and how it
modifies the behaviour of `{}` braces. Currently the "transpiler"
only consists of ~100 lines of Python code, the remaining
syntactic sugar is achieved using Lua operator overloading
(see `match.lua` for details.)

<small>
Writing DSLs can be fun, but can also be too much fun, resulting
in an overengineered language requiring a lot of maintenance.

This is why Lua, with a single extra operator that was
easy to implement is what I went with in Hawck.
</small>

The lua files are deleted on the fly, but you can open a lua shell with your
script loaded for investigation:

```sh
$ cd ~/.local/share/hawck/scripts/
$ hwk2lua example.hwk >! tmp.lua
$ lua -l init -l tmp -i
$ rm tmp.lua
```

## GUI

Hawck used to have a more actively maintained GUI, but as of now it is in a
frozen state, I currently don't recommend using it.

Use `hawck-add` and your preferred editor from the terminal instead.

In the future I may start working on the GUI again, to implement key-remapping
for non-advanced users who don't wish to use either the terminal or a scripting
language. But currently that use case just doesn't exist for Hawck.

## FAQ

### Persisting the configuration

The **input daemon** should autostart, which you can enable/disable with

```sh
$ sudo systemctl enable hawck-inputd
```

The **macro daemon** will not autostart (and your journal will contain
`hawck-inputd: SystemError: Connection refused`). When you've got everything set
up correctly, and you understand how it works and how to recover issues, you may
want autostart it. Depending on your distribution the provided _desktop_ file
might work:

```sh
cp bin/hawck-macrod.desktop "${XDG_CONFIG_HOME:-~/.config}"/autostart
```

#### <a name="faq-layout"></a> Is my keyboard layout supported?

If you're using a Norwegian or US keyboard, yes.
If not, maybe.

If not you might have to do some tweaking, feel free to report
an issue on GitHub for your keymap.

The layout support works like this:

- Because the input daemon runs as root, what it intercepts are the raw kernel
  keycodes (defined in [/usr/include/linux/input-event-codes.h][1]
- The numeric keycodes that are listed in
  _/usr/share/hawck/keymaps/default_linux.lua_ can be used in your scripts eg
  `key(57)` for the space key (except numbers 1 to 9 which are aliased to the
  numeric top row keys). Find the keycode for any key by switching to a
  [Virtual console](https://wiki.archlinux.org/title/Linux_console) and running
  `showkey`.
- Using numeric codes is impractical, and using the name the kernel has for the
  key is confusing as they are not aligned with the key label especially on
  non-us layouts. This is why Hawck tries to provide localized key names.
- There are 2 main mechanisms to transform the keycodes the kernel detects into
  higher-level events like an actual character on screen:
  1. [Kbd](https://kbd-project.org/), used by the virtual console
  2. [Xkb](https://www.x.org/wiki/XKB/), used by both X11 and Wayland
     (through libinput).
- Both provide a way to define localized keymaps, and Hawck decided to hook into
  the first, simpler console keymaps to provide support. Note, that their may be
  discrepancies between the Kbd and Xkb definitions of a given keyboard layout.
- So Hawck needs to parse a console keymap definition of your layout. It is not
  necessarily built-into your system, for example on Debian/Ubuntu based distro
  it is in the `console-data` package.
- Based on the `keymap = "foo"` line _~/.local/share/hawck/cfg.lua_, Hawck will
  search for a file named `foo.(k)map.gz` in a number of common directories like
  _/usr/share/keymaps/_ (see [kbmap.getall()][2]).
- **The symbol names present in those keymap files are the ones that can be
  used**, with the addition of some [built-in aliases](keymaps/aliases.lua)
  between unicode characters and key names, so that `insert "/"` is able to
  insert a slash.

For example, the top-left key (under Esc) on a US keyboard has code 41 according
to `showkey` and _us.kmap.gz_ has this line:

```
  keycode  41 = grave            asciitilde
```

So `key("grave")` can be used to refer to this key.

In a lua shell `cd ~/.local/share/hawck/scripts/ ; lua -l init -i`, you can see
how the keymap has been parsed with `u.puts(kbd.map)`

[1]: https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
[2]: https://github.com/snyball/Hawck/blob/master/src/Lua/Keymap.lua#L247

#### Hotplugging keyboards

When `hawck-inputd` starts up it finds the connected keyboards
then proceeds to lock them and listen for key events.

You can then plug the keyboards out and in as many times as
you please.

But, emphasis on out and in, not in and out. `Inputd` will
only use the keyboards that were available when it started.
Changing this to support adding more keyboards while inputd
is running is planned, it is mostly a relic of having to
debug `inputd` by plugging in another keyboard (which I
didn't want it locking on to.)

Currently, if you plug in another keyboard you can just
run `systemctl restart hawck-inputd` and it should find
the new keyboard, and automatically lock it the next time
it is plugged in (before you restart hawck-inputd again.)

#### Alternative security strategies and motivations

The current model is a compromise between ease of use, and security.

The issue with the current Hawck security model is that binding all keys
on your keyboard (or even 20-30% of them, depending on which keys they
are and how common they are in your language) effectively renders the
security model useless.

Here are some alternative models that have been considered:

- Run all scripts inside inputd
  - In this model there is only a single daemon, hawck-inputd
  - This is planned as an optional alternative, and a lot of
    the code has already been written, but it takes a lot of
    work and consideration to make this work well, and securely.
    - Work on this might just be scrapped for script-less
      rebinding directly inside inputd.
  - Scripts are sandboxed and are not allowed to communicate
    with the outside world, this includes launching programs.
    - Problem #1: Not being able to bind keys to launchers was
      a deal breaker for me.
    - Problem #2: Perfect sandboxing isn't easy to pull of, it
      requires very careful programming and as such takes longer to
      implement.
- Don't use scripts
  - This is obviously not as flexible.
  - A common use case for something like this is very simple
    rebindings, like caps => control. This system would work
    very well for those cases.
  - Although this is planned as an alternative, it can never
    be the only supported mode, as Hawck aims to be as useful
    when it comes to keyboard automation as something like
    AutoHotkey.
- "Abandon all hope, ye who enter unsafe mode"

  - For a lot of users, especially the ones still on X11, these
    issues are not a concern. They might want a model that just
    let's them do whatever they want whenever they want without
    any bothersome password prompts. This is not recommended,
    but a perfectly valid stance.
  - This is already supported, opt-in, by toggling unsafe mode
    in the settings panel of hawck-ui.

- What I'm currently aiming for is being able to support these
  three modes:
  - Sandboxed inside inputd
    - Used for most simple key-replacement scripts.
  - Whitelisted keys passed from inputd to macrod (current method)
    - Used for anything that can't be done from the sandbox.
  - Unsafe mode (optional current method)
    - Used by X11 users, and users who just don't care.

Ideally the hawck-ui editor should be able to start off
assuming sandboxed mode, then inform the user about the
need to switch to whitelist mode if it sees use of any feature
unsupported in the sandbox, like `io.popen` or `app`.

#### Synchronous and asynchronous keyboard locking explained

If you read the logs you might see something like this:

```
hawck-inputd[7672]: Running Hawck InputD ...
hawck-inputd[7672]: Attempting to get lock on device: Logitech Logitech G710 Keyboard @ usb-0000:00:14.0-4/input0
hawck-inputd[7672]: Locking keyboard: Logitech Logitech G710 Keyboard ...
hawck-inputd[7672]: Preparing async lock on: Logitech Logitech G710 Keyboard @ usb-0000:00:14.0-4/input0
hawck-inputd[7672]: Attempting to get lock on device: AT Translated Set 2 keyboard @ isa0060/serio0/input0
hawck-inputd[7672]: Locking keyboard: AT Translated Set 2 keyboard ...
hawck-inputd[7672]: Immediate lock on: AT Translated Set 2 keyboard @ isa0060/serio0/input0
hawck-inputd[7672]: Acquired lock on keyboard: Logitech Logitech G710 Keyboard
```

`hawck-inputd` employs two different methods for locking keyboards.

One is the simple immediate lock, where an `ioctl` call is used to
gain exclusive access to the keyboard immediately.

The other one is an asynchronous lock, which solves the following
problem, illustrated by using an immediate lock where it should
not be used:

- hawck-inputd is not running
- user holds down shift on G710 keyboard
- GNOME desktop receives a [down + shift] event, from G710 keyboard
- hawck-inputd starts up
- hawck-inputd acquires an immediate lock on G710
  - GNOME will now not see any more of the G710 keypresses
- hawck-inputd registers a virtual keyboard device
  - This device will now echo key events that come from G710,
    and any other keyboard on the system.
- user stops pressing the shift key on G710 keyboard
- hawck-inputd receives the [up + shift] event
- hawck-inputd will now echo the [up + shift] event through
  its virtual keyboard.
- GNOME desktop sees an [up + shift] event, from Hawck keyboard
  - GNOME doesn't (and shouldn't) do anything with this event
- GNOME desktop still considers the shift key to be held down,
  because as far as it knows it is still held down on one of the
  keyboards, Hawck stole the memo.
- user presses w, but what comes out is "W"
- No amount of pressing and releasing shift solves the problem.
- user is having a bad time

Asynchronous lock solves this problem by simply waiting until
every key has been released before the keyboard is locked.

## Ports

### BSD

Porting to FreeBSD is possible, but requires the installation of shims, or
porting of the following classes/functions.

- `Keyboard`
  - Access to raw keyboard input.
  - shim: https://www.freshports.org/devel/evdev-proto/
  - Requires: `ioctl(fd, EVIOCGRAB, ...)` for exclusive keyboard lock.
- `UDevice`
  - Virtual keyboard devices for outputting keys.
  - shim: https://www.freshports.org/devel/evdev-proto/
- `KBDInfo`/`KBDB`
  - Gathers info about a keyboard from `sysfs`
  - Can this be retrieved using `sysctl` on FreeBSD?
- `pidexe(pid_t)`
  - Used to find the executable behind a process id
  - Single line of code change.
- `FSWatcher.cpp`
  - File system notifications
  - shim: https://github.com/libinotify-kqueue/libinotify-kqueue

## Known Bugs:

- Outputting keys too quickly:
  - GNOME Wayland has a bug where it will drop a lot of keys
  - Workaround: run hawck-inputd with the --udev-event-delay flag set to 3800 (µs)
