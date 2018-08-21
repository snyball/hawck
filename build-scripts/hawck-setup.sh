#!/bin/bash

##
## Script to be run as root.
##

export SPID=$$
function die() {
    echo "$1" >&2
    kill $SPID
}

if [ "$#" -lt 1 ]; then
    die "Not enough arguments, do not use this script directly, use install.sh"
fi
USER="$1"

pushd "${MESON_SOURCE_ROOT}"

HAWCK_BIN=/usr/share/hawck/bin
mkdir -p "$HAWCK_BIN"
pushd "src/scripts"
cp ./*.sh "$HAWCK_BIN/"
for script in $HAWCK_BIN/*.sh; do
    echo "Script: $script"
    chmod 755 "$script"
done
chown -R root:root "$HAWCK_BIN"
popd

HAWCK_LLIB=/usr/share/hawck/LLib
mkdir -p "$HAWCK_LLIB"
cp src/Lua/*.lua "$HAWCK_LLIB/"

cp -r keymaps /usr/share/hawck/keymaps
cp -r icons /usr/share/hawck/icons

#find /usr/share/hawck -type d -exec chmod 755 '{}' \;
#find /usr/share/hawck \( -type f -and -not -name "*.sh" \) -exec chmod 755 '{}' \;

echo "Currently in: $(pwd -P)"
USER_HOME=$(realpath /home/$USER/)

BIN=/usr/local/bin/

cp src/hwk2lua/hwk2lua.py "$BIN/hwk2lua"
chmod 755 "$BIN/hwk2lua"

popd ## Done installing files

## Set up hawck-input home directory
mkdir -p /var/lib/hawck-input/keys

## Set up users and groups.
useradd hawck-input
usermod -aG input hawck-input
usermod --home /var/lib/hawck-input hawck-input
groupadd hawck-input-share
usermod -aG hawck-input-share hawck-input

## Allow for sharing of key inputs via UNIX sockets.
usermod -aG hawck-input-share "$USER"
chown hawck-input:hawck-input-share /var/lib/hawck-input
chmod 770 /var/lib/hawck-input

## Make sure the keys are locked down with correct permissions.
chown hawck-input:hawck-input /var/lib/hawck-input/keys
chmod 750 /var/lib/hawck-input/keys

