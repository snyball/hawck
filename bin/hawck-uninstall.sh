#!/bin/bash

##
## Uninstall everything installed by the custom install
## script.
##

RED="\033[0;31m"
REDB="\033[1;31m"
WHITEB="\033[1;37m"
GREENB="\033[1;32m"
NC="\033[0m"

function ok() {
    echo -e "$WHITEB""OK: $GREENB$1$NC"
}

systemctl disable --now hawck-input
kill $(pidof hawck-macrod)
ok "Stopped daemon"

## Directories/files
rm -r '/var/lib/hawck-input'
rm -r '/usr/share/hawck'
rm '/etc/udev/rules.d/99-hawck-input.rules'
ok "Removed scripts and daemon files"

## Icons
# find '/usr/share/icons/hicolor' -name "hawck.png" -exec rm '{}' \;
# find '/usr/share/icons/hicolor' -name "hawck.svg" -exec rm '{}' \;
for res in 32 64 128 256 512; do
    res2="$res"'x'"$res"
    fpath="/usr/share/icons/hicolor/$res2/apps/"
    [ -f "$fpath/hawck.png" ] && rm "$fpath/hawck.png"
    [ -f "$fpath/hawck.svg" ] && rm "$fpath/hawck.svg"
done
ok "Removed icons"

rm '/usr/local/bin/hawck-ui'
rm '/usr/local/bin/hwk2lua'
yes | pip uninstall hawck_ui
rm '/usr/share/applications/hawck.desktop'
ok "Removed hawck-ui"

## Users and groups
userdel hawck-input
groupdel hawck-input-share
# groupdel uinput
# chown root:uinput /dev/uinput
chmod 600 /dev/uinput
ok "Removed users/groups"

## SystemD service file
rm '/etc/systemd/system/hawck-input.service'
ok "Removed SystemD service"
