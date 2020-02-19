#!/bin/bash

if [ "$(whoami)" = "root" ]; then
    echo "Will not run as root."
    exit 1
fi

LOCAL_SHARE="${XDG_DATA_HOME:-$HOME/.local/share}"

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

LLIB="$LOCAL_SHARE/hawck/scripts/LLib"
[ -e "$LLIB" ] || ln -s /usr/share/hawck/LLib "$LLIB"
KEYMAPS_LNK="$LOCAL_SHARE/hawck/scripts/keymaps"
[ -e "$KEYMAPS_LNK" ] || ln -s /usr/share/hawck/keymaps "$KEYMAPS_LNK"
INIT_LNK="$LOCAL_SHARE/hawck/scripts/init.lua"
[ -e "$INIT_LNK" ] || ln -s /usr/share/hawck/LLib/init.lua "$INIT_LNK"

if ! groups | tr " " "\n" | grep "^hawck-input-share\$"; then
    notify-send --icon=warn \
                --app-name="Hawck" \
                "You are not a member of the hawck-input-share group."

    if ! pkexec usermod -aG hawck-input-share "$(whoami)"; then
        echo "Do you have a PolicyKit Authentication agent running?"
        echo "Try running:"
        echo "  nohup /usr/lib/polkit-gnome/polkit-gnome-authentication-agent-1 &"
        echo "  disown"
    fi
fi
