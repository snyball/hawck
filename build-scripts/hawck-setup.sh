#!/usr/bin/pkexec /bin/bash

##
## Script to be run as root.
##

export SPID=$$
function fail() {
    echo "$1"
    kill $SPID
}

if [ "$#" -lt 2 ]; then
    fail "Not enough arguments, do not use this script directly, use install.sh"
fi
USER="$1"
HAWCK_SRC_ROOT="$2"

pushd "$HAWCK_SRC_ROOT"

pushd "hawck-ui"
chmod +x setup.py
if ! ./setup.py; then
    fail "Unable to install hawck-ui"
fi
popd

HAWCK_BIN=/usr/share/hawck/bin
mkdir -p "$HAWCK_BIN"
pushd "src/scripts"
cp ./*.sh "$HAWCK_BIN/"
chmod 755 "$HAWCK_BIN/*.sh"
chown -R root:root "$HAWCK_BIN"
popd

HAWCK_LLIB=/usr/share/hawck/LLib
mkdir -p "$HAWCK_LLIB"
cp src/Lua/*.lua "$HAWCK_LLIB/"

cp -r keymaps /usr/share/hawck/keymaps
cp -r icons /usr/share/hawck/icons

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
chown hwack-input:hawck-input /var/lib/hawck-input/keys
chmod 750 /var/lib/hawck-input/keys

