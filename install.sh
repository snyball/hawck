#!/bin/bash

function show_error() {
    if which zenity; then
        zenity --error --ellipsize --text="$1"
    else
        echo "$1"
    fi
}

export SPID=$$
function die() {
    echo "$1" >&2
    kill $SPID
}

## Generate documentation
echo "Generating documentation ..."
if ! doxygen &>/dev/null; then
    echo "Failed, skipping."
fi

mkdir build
cp build-scripts/run-hawck.sh build/

## Configure, build, install
pushd "build"
meson -Ddesktop_user=$(whoami) || die
meson configure -Ddesktop_user=$(whoami) || die
ninja -j4 || die
ninja hawck-ui || die
ninja install || die
popd

## Set up user directories

LOCAL_SHARE="$HOME/.local/share"
if ! [ -d "$LOCAL_SHARE" ]; then
    mkdir -p "$LOCAL_SHARE"
fi

mkdir -p "$LOCAL_SHARE/hawck/scripts"
mkdir -p "$LOCAL_SHARE/hawck/scripts-enabled"
ln -s /usr/share/hawck/LLib "$LOCAL_SHARE/hawck/scripts/LLib"
ln -s /usr/share/hawck/keymaps "$LOCAL_SHARE/hawck/scripts/keymaps"
ln -s /usr/share/hawck/LLib/init.lua "$LOCAL_SHARE/hawck/scripts/init.lua"

echo "Keyboards:"
lsinput -s | grep -v 'if0' | grep -iE 'keyboard|kbd'
