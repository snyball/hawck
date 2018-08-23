#!/bin/bash

##
## Script to be run as root.
##

RED="\033[0;31m"
REDB="\033[1;31m"
WHITEB="\033[1;37m"
GREENB="\033[1;32m"
NC="\033[0m"

export SPID=$$
function die() {
    echo -e "$REDB""Setup error: $1$NC"
    kill $SPID
}

function ok() {
    echo -e "$WHITEB""OK: $GREENB$1$NC"
}

if [ "$#" -lt 1 ]; then
    die "Not enough arguments, do not use this script directly, use install.sh"
fi
USER="$1"

if [ "$USER" = "" ]; then
    die "Desktop user not defined, run: 'meson configure -Ddesktop_user=\$(whoami)'"
fi

if [ "$USER" = "root" ]; then
    die "Desktop user was set to 'root', this is not allowed, run: 'meson configure -Ddesktop_user=\$(whoami)'"
fi

ok "Setting up hawck for $USER"

pushd "${MESON_SOURCE_ROOT}" &>/dev/null

HAWCK_BIN=/usr/share/hawck/bin
mkdir -p "$HAWCK_BIN"
for src in src/scripts/*.sh; do
    name=$(basename $src)
    echo "\$ install -m 755 '$src' '$HAWCK_BIN/$name'"
    install -m 755 "$src" "$HAWCK_BIN/$name"
done
chown -R root:root "$HAWCK_BIN"

ok "Installed scripts to hawck/bin"

HAWCK_LLIB=/usr/share/hawck/LLib
mkdir -p "$HAWCK_LLIB"
cp src/Lua/*.lua "$HAWCK_LLIB/"

KEYMAPS_DIR=/usr/share/hawck/keymaps
[ -d $KEYMAPS_DIR ] && rm -r $KEYMAPS_DIR
cp -r keymaps $KEYMAPS_DIR

ICONS_DIR=/usr/share/hawck/icons
[ -d $ICONS_DIR ] && rm -r $ICONS_DIR
cp -r icons $ICONS_DIR

BIN=/usr/local/bin/

install -m 755 src/hwk2lua/hwk2lua.py "$BIN/hwk2lua"

## Copy icons
pushd "icons" &>/dev/null
for res in 32 64 128 256 512; do
    res2="$res"'x'"$res"
    icon_src="alt_hawck_logo_v2_red_$res2.png"
    if [ -f "$icon_src" ]; then
        cp "$icon_src" /usr/share/icons/hicolor/$res2/apps/hawck.png
    fi
done
popd &>/dev/null
gtk-update-icon-cache /usr/share/icons/hicolor/ && ok "Updated icon cache"

## Install rules to make /dev/uinput available to hawck-uinput users
cp bin/99-hawck-input.rules /etc/udev/rules.d/
chown root:root /etc/udev/rules.d/99-hawck-input.rules
chmod 644 /etc/udev/rules.d/99-hawck-input.rules

## Make sure the uinput module is loaded, this isn't necessary on
## some systems, but is required on Arch Linux. If this isn't done
## the 99-hawck-input.rules file will have no effect.
echo uinput > /etc/modules-load.d/hawck-uinput.conf

## Hawck desktop integration.
cp bin/hawck-ui.desktop /usr/share/applications/
chmod 644 /usr/share/applications/hawck-ui.desktop

## Default keyboards to listen on.
cp bin/keyboards.txt /var/lib/hawck-input/

popd &>/dev/null ## Done installing files

## Set up hawck-input home directory
mkdir -p /var/lib/hawck-input/keys

## Set up users and groups.
useradd hawck-input
usermod -aG input hawck-input
usermod --home /var/lib/hawck-input hawck-input
groupadd hawck-input-share
usermod -aG hawck-input-share hawck-input
groupadd hawck-uinput
usermod -aG hawck-uinput hawck-input

ok "Configured groups"

## Allow for sharing of key inputs via UNIX sockets.
usermod -aG hawck-input-share "$USER"
chown hawck-input:hawck-input-share /var/lib/hawck-input
chmod 770 /var/lib/hawck-input

## Make sure the keys are locked down with correct permissions.
chown hawck-input:hawck-input-share /var/lib/hawck-input/keys
chmod 750 /var/lib/hawck-input/keys

## Make sure that hawck-input can create virtual input devices.
## After a reboot this will be done by the 99-hawck-input.rules file.
chown root:hawck-uinput /dev/uinput
chmod 660 /dev/uinput

echo -e "$WHITEB""Installation was succesful$NC"
