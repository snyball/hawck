#!/bin/bash

##
## This file can pretty much be ignored by package maintainers,
## except as a high-level guide to how things are set up. It just
## ties a few scripts together.
##

##
## Due to changes in this script, it unfortunately requires three
## separate password entries, because it has to drop in and out
## of root at different stages.
##

export SPID=$$
function die() {
    kill $SPID
    exit 1
}

if [ $(whoami) = 'root' ]; then
    echo "Error: Do not run $0 as root, run it under your normal user account."
    exit 1
fi

function run() {
    ## Generate documentation
    if ! doxygen &>/dev/null; then
        echo "Generating documentation failed, skipping." >&2
    fi

    ## Install dependencies
    if which apt &> /dev/null; then
        pkexec xargs apt -y install < bin/dependencies/debian-deps.txt || die "Failed to install dependencies"
    fi
    
    mkdir build
    cp bin/run-hawck.sh build/
    ## Configure, build, install
    pushd "build" &>/dev/null

    meson -Duse_meson_install=true >&2 || die "Failed to create build"
    meson configure -Duse_meson_install=true >&2 || die "Failed to configure meson"

    ninja || die "Unable to compile hawck"
    ninja hawck-ui >&2 || die "Failed to build hawck-ui"
    pkexec bash -c "cd '$(pwd)' && ninja install" >&2 || die "Installation failed"
    popd &>/dev/null
    
    ## Run user setup
    ## This might be moved into hawck-ui, after a startup check for the hawck
    ## directory.
    ##
    ## Package maintainers are advised to inform users that they need to run
    ## this script after installation.
    bash bin/hawck-user-setup.sh
}

run
echo "Installation was successful"

xdg-open ./changelogs/changelog_0.6.0.md
