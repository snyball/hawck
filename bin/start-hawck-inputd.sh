#!/bin/bash

##
## Start hawck-inputd on all detected keyboards.
##

export PATH="$PATH:/usr/share/hawck/bin"

PID=$(cat /var/lib/hawck-input/pid)
if [ "$PID" != "" ] && [ -L /proc/$PID/exe ] && [ "$(basename $(readlink /proc/$PID/exe))" == 'hawck-inputd' ]; then
    echo "Already running, run systemctl stop hawck-inputd to kill it."
    exit 1
fi

KBD_ARGS="$(lsinput -s | get-kbds.awk)"
exec hawck-inputd $KBD_ARGS "$@"
