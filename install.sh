#!/bin/bash

##
## No-frills install script for all the distributions that don't have a Hawck
## package.
##

## DWIM if the user decided to run the install script with sudo.
sudo () { command sudo "$@"; }
sudown () { "$@"; }
if [ $(whoami) = root ]; then
    sudo () { "$@"; }
    sudown () { su -s /bin/bash "$(logname)" -c "$@"; }
fi

install () {
    source ../bin/hawck-git/hawck-git.install || exit 1
    ninja install || exit 1
    post_install || exit 1
}

mkdir -p build || exit 1
pushd build
meson .. || exit 1
ninja || exit 1
sudo bash -c "$(declare -pf install); install" || exit 1
popd
sudown bin/hawck-user-setup.sh
echo "Done."
