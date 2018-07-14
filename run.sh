#!/bin/bash

KBD="/dev/input/by-path/pci-0000:00:14.0-usb-0:3:1.0-event-kbd"
sudo chgrp $(whoami) "$KBD"
echo "Changed groups, running hawck ..."
if [ "$1" = "--debug" ]; then
    ./hawck "$KBD" &
else
    nohup ./hawck "$KBD" &>/dev/null &
fi
echo "Done."
