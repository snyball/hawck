#!/bin/bash

## Path to keyboard (just try every file in /dev/input/by-path/ until it works.)
KBD_PATH="/dev/input/by-path/pci-0000:00:14.0-usb-0:3:1.0-event-kbd"

if ! stat "$KBD_PATH" &>/dev/null; then
    echo "No such device: $KBD_PATH"
    exit 1
fi

## Follow any links (assumed relative)
KBD="$KBD_PATH"
while [ -L "$KBD" ]; do
   KBD="$(dirname $KBD)/$(readlink $KBD)"
done

## Check if we need to change group permissions.
if [ $(stat -c %G "$KBD") != $(whoami) ]; then
    echo "Need to change group permissions on keyboard file to intercept keys. "
    if ! sudo chgrp $(whoami) "$KBD"; then
        echo "Unable to change group permissions."
        exit 1
    fi
    echo "Successfully changed group permissions for $KBD."
fi

## Run daemon
echo "Running hawck on $KBD_PATH ..."
if [ "$1" = "--debug" ]; then
    ./hawck "$KBD"
else
    nohup ./hawck "$KBD" &>/dev/null &
    disown -a
fi

echo "Done."
