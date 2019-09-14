#!/bin/bash
##
## Set up hawck for the user running the script, or
## alternatively set it up for any user (if running
## as root)
##

USER="$(whoami)"
USER_HOME="$HOME"
if [ "$USER" = "root" ]; then
    if [ $# -lt 1 ]; then
        echo "Will not install hawck for root, specify a user when running as root."
        echo "Usage:"
        echo "  hawck-user-setup.sh <user>"
        exit 1
    fi
    USER="$1"
    USER_HOME="/home/$USER"
fi

## FIXME: This should respect $XDG variables
LOCAL_SHARE="$USER_HOME/.local/share"
## Very unlikely, but you never know
if ! [ -d "$LOCAL_SHARE" ]; then
    mkdir -p "$LOCAL_SHARE"
fi

mkdir -p "$LOCAL_SHARE/hawck/scripts"
mkdir -p "$LOCAL_SHARE/hawck/scripts-enabled"
## Install example script if it does not exist.
## The user may rewrite this script without renaming it, so the check
## is critical.
if ! [ -f "$LOCAL_SHARE/hawck/scripts/example.hwk" ]; then
    cp src/macro-scripts/example.hwk "$LOCAL_SHARE/hawck/scripts/example.hwk"
fi
if ! [ -f "$LOCAL_SHARE/hawck/cfg.lua" ]; then
    cp bin/cfg.lua "$LOCAL_SHARE/hawck/"
fi
ln -s /usr/share/hawck/LLib "$LOCAL_SHARE/hawck/scripts/LLib" &>/dev/null
ln -s /usr/share/hawck/keymaps "$LOCAL_SHARE/hawck/scripts/keymaps" &>/dev/null
ln -s /usr/share/hawck/LLib/init.lua "$LOCAL_SHARE/hawck/scripts/init.lua" &>/dev/null
mkfifo -m 600 "$LOCAL_SHARE/hawck/json-comm.fifo"
mkfifo -m 600 "$LOCAL_SHARE/hawck/lua-comm.fifo"

if [ "$USER" = "root" ]; then
   usermod -aG hawck-input-share "$USER"
   chown -R "$USER":"$USER" "$LOCAL_SHARE/hawck"
else
   pkexec usermod -aG hawck-input-share "$USER"
fi
