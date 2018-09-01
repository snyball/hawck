# Changelog for 0.6.0

- Features:
  - Can now have multiple keyboards.
  - Can now hotplug keyboards, i.e plug them out and
    have everything just work when you plug it back in.
  - Fixed/improved log viewer, no longer displays duplicates
    and reloads automatically every 0.5s.
  - Added backtrace to Lua error logs.
  - Added getopt cmdline parsers to hawck-inputd and
    hawck-macrod
  - Now automatically running `require "init"` in Lua scripts.
  - Made most of the error logging use `syslog`
  - Improved error logging related to invalid permissions.
  - Added man pages for hawck-inputd, hawck-macrod, and lsinput

- Misc. changes
  - No longer writing default script content to scripts.
  - Now always installing an example.hwk script.
  - Changed switches on status page to color-coded labels.
  - Added timeout on socket connections in InputD.
  - Added privesc library, and made all actions requiring
    elevated priviliges use `@su.do`
  - Added runtime configuration system using Lua to MacroD.
  - Added `--udev-event-delay` as a cmdline flag to InputD.
  - Added Lua library for json serialization.

- Bugfixes:
  - Fixed the key-held-on-start bug that has existed
    since 0.1
  - Fixed launching of desktop files without a %u in
    the Exec option
  - Fixed /dev/uinput permissions, now adding a udev
    rules file during the install.
  - Fixed desktop file for hawck UI not being attached
    to the running program on Wayland.

- Code cleanup
  - Cleanup and steps towards modularization in the
    UI code.
  - Cleanup in lsinput, fixed memory leak.
