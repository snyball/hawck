#!/bin/bash

##
## Post installation script
##

function setup-users() {
    useradd hawck-input
    usermod -aG input hawck-input
    usermod --home "$DESTDIR/var/lib/hawck-input" hawck-input
    groupadd hawck-input-share
    usermod -aG hawck-input-share hawck-input
    groupadd uinput
    usermod -aG uinput hawck-input
}

function setup-files() {
    ## Set up hawck-input home directory
    mkdir -p "$DESTDIR/var/lib/hawck-input/keys"

    chown hawck-input:hawck-input-share "$DESTDIR/var/lib/hawck-input"
    chmod 770 /var/lib/hawck-input

    ## Make sure the keys are locked down with correct permissions.
    chown hawck-input:hawck-input-share "$DESTDIR/var/lib/hawck-input/keys"
    chmod 750 /var/lib/hawck-input/keys

    ## Make sure that hawck-input can create virtual input devices.
    ## After a reboot this will be done by the 99-hawck-input.rules file.
    chown root:uinput /dev/uinput
    chmod 660 /dev/uinput
}

setup-users
setup-files
