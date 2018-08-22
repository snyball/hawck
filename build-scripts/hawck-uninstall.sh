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

sytemctl stop hawck-input
kill $(pidof hawck-macrod)
ok "Stopped daemon"

## Directories/files
rm -r '/var/lib/hawck-input'
rm -r '/usr/share/hawck'
rm '/etc/udev/rules.d/99-hawck-input.rules'
ok "Removed scripts and daemon files"

## Icons
find '/usr/share/icons/hicolor' -name "hawck.png" -exec rm '{}' \;
find '/usr/share/icons/hicolor' -name "hawck.svg" -exec rm '{}' \;
ok "Removed icons"

rm '/usr/local/bin/hawck-ui'
rm '/usr/local/bin/hwk2lua'
yes | pip uninstall hawck_ui
rm '/usr/share/applications/hawck.desktop'
ok "Removed hawck-ui"

## Users and groups
userdel hawck-input
groupdel hawck-input-share
groupdel hawck-uinput
chown root:hawck-uinput /dev/uinput
chmod 600 /dev/uinput
ok "Removed users/groups"

## SystemD service file
systemctl disable hawck-input
rm '/etc/systemd/system/hawck-input.service'
ok "Removed SystemD service"
