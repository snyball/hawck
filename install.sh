#!/bin/bash

export SPID=$$
function die() {
    echo "100%"
    echo "$1." >&2
    zenity --error --ellipsize --text="$1"
    kill $SPID
}

if [ $(whoami) = 'root' ]; then
    echo "Error: Do not run $0 as root, run it under your normal user account."
    exit 1
fi

function run() {
    ## Generate documentation
    echo "Generating documentation ..."
    if ! doxygen &>/dev/null; then
        echo "Failed, skipping." >&2
    fi
    echo "10%"

    ## Install dependencies
    if which apt; then
        pkexec xargs apt -y install < bin/dependencies/debian-deps.txt || die "Failed to install dependencies"
    fi
    
    echo "20%"

    mkdir build
    cp bin/run-hawck.sh build/
    ## Configure, build, install
    pushd "build" &>/dev/null
    meson -Ddesktop_user=$(whoami) >&2 || die "Failed to create build"
    meson configure -Ddesktop_user=$(whoami) >&2 || die "Failed to configure meson"
    echo "40%"
    ninja -j4 >&2 || die "Failed to build hawck-macrod and hawck-inputd"
    echo "60%"
    ninja hawck-ui >&2 || die "Failed to build hawck-ui"
    echo "80%"
    pkexec bash -c "cd $(pwd) && ninja install" >&2 || die "Installation failed"
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
    
    echo "100%"
}

if which zenity &>/dev/null && [ "$1" == "--zenity" ]; then
    zenity --info --ellipsize --text="Will now install Hawck, you will be prompted for your password twice during the installation." --title="Hawck installation"
    run 2>install_log.txt | tee | zenity --progress --auto-close
    sed -e 's/\x1b\[[;0-9]*m//g' -i install_log.txt
    zenity --info --ellipsize --text="Installation was successful" --title='Hawck'
else
    run
    echo "Installation was successful"
fi
