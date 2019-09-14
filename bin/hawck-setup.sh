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

ok "Setting up hawck."

function setup-users() {
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

    chown hawck-input:hawck-input-share /var/lib/hawck-input
    chmod 770 /var/lib/hawck-input

    ## Make sure the keys are locked down with correct permissions.
    chown hawck-input:hawck-input-share /var/lib/hawck-input/keys
    chmod 750 /var/lib/hawck-input/keys

    ## Make sure that hawck-input can create virtual input devices.
    ## After a reboot this will be done by the 99-hawck-input.rules file.
    chown root:hawck-uinput /dev/uinput
    chmod 660 /dev/uinput
}

pushd "${MESON_SOURCE_ROOT}" &>/dev/null

HAWCK_SHARE=/usr/share/hawck
mkdir -p "$HAWCK_SHARE"

HAWCK_BIN="$HAWCK_SHARE/bin"
mkdir -p "$HAWCK_BIN"
for src in src/scripts/*.sh src/scripts/*.awk; do
    name="$(basename "$src")"
    echo "\$ install -m 755 '$src' '$HAWCK_BIN/$name'"
    install -m 755 "$src" "$HAWCK_BIN/$name"
done
chown -R root:root "$HAWCK_BIN"

## TODO: Move __UNSAFE_MODE.csv into here:
mkdir -p "$HAWCK_SHARE/keys"
cp bin/__UNSAFE_MODE.csv  "$HAWCK_SHARE/keys/"
chmod 644 bin/__UNSAFE_MODE.csv

cp bin/hawck-macrod.desktop $HAWCK_SHARE/bin/

ok "Installed scripts to hawck/bin"

cp bin/start-hawck-inputd.sh /usr/local/bin/start-hawck-inputd

HAWCK_LLIB=$HAWCK_SHARE/LLib
mkdir -p "$HAWCK_LLIB"
cp src/Lua/*.lua "$HAWCK_LLIB/"

KEYMAPS_DIR=$HAWCK_SHARE/keymaps
[ -d $KEYMAPS_DIR ] && rm -r $KEYMAPS_DIR
cp -r keymaps $KEYMAPS_DIR

ICONS_DIR=$HAWCK_SHARE/icons
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

cp bin/hawck-inputd.service /etc/systemd/system/

## Default keyboards to listen on.
cp bin/keyboards.txt /var/lib/hawck-input/

popd &>/dev/null ## Done installing files

setup-users

ok "Configured groups"

chmod -R a+r "$HAWCK_SHARE"
find "$HAWCK_SHARE" -type d -exec chmod a+x '{}' \;

echo -e "$WHITEB""Installation was succesful$NC"
