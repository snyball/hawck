#!/bin/bash

export SPID=$$
function die() {
    echo "$1." >&2
    zenity --error --ellipsize --text="$1"
    kill $SPID
}

if [ $(whoami) = 'root' ]; then
    echo "Error: Do not run $0 as root, run it under your normal user account."
    exit 1
fi

## Generate documentation
echo "Generating documentation ..."
if ! doxygen &>/dev/null; then
    echo "Failed, skipping."
fi

#git pull || die "Failed to pull from git repo"

## Install dependencies
if which apt; then
    pkexec xargs apt -y install < bin/dependencies/debian-deps.txt || die "Failed to install dependencies"
fi

mkdir build
cp bin/run-hawck.sh build/
cp bin/run-tests.sh build/
## Configure, build, install
pushd "build" &>/dev/null
meson -Ddesktop_user=$(whoami) || die "Failed to create build"
meson configure -Ddesktop_user=$(whoami) || die "Failed to configure meson"
ninja -j4 || die "Failed to build hawck-macrod and hawck-inputd"
ninja hawck-ui || die "Failed to build hawck-ui"
pkexec bash -c "cd $(pwd) && ninja install" || die "Installation failed"
popd &>/dev/null

## Set up user directories

LOCAL_SHARE="$HOME/.local/share"
if ! [ -d "$LOCAL_SHARE" ]; then
    mkdir -p "$LOCAL_SHARE"
fi

mkdir -p "$LOCAL_SHARE/hawck/scripts"
mkdir -p "$LOCAL_SHARE/hawck/scripts-enabled"
ln -s /usr/share/hawck/LLib "$LOCAL_SHARE/hawck/scripts/LLib" &>/dev/null
ln -s /usr/share/hawck/keymaps "$LOCAL_SHARE/hawck/scripts/keymaps" &>/dev/null
ln -s /usr/share/hawck/LLib/init.lua "$LOCAL_SHARE/hawck/scripts/init.lua" &>/dev/null

zenity --info --ellipsize --text="Installation was successful" --title='Hawck'
