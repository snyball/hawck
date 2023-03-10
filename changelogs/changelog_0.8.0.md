# Changelog for 0.8.0

- Breaking changes
  - `replace` and `sub` now work with modifiers instead of actively silencing them

- Features:
  - Now finds keymaps with '.' in their names

- Misc. changes
  - only use NOTIFY_URGENCY_CRITICAL notifs when adequate
  - Added useful logs to 'No such key' and 'No such keymap' errors
  - Added feedback in case included keymaps fail to parse

- Bugfixes:
  - Fix parsing of key names containing '\_' like KP_Enter etc. Fixes #10, #72
  - Fix for Ubuntu 22.04 `mktemp` hardening
  - Fix for Asahi Linux where `/dev/input/by-id` is not created
