#!/bin/bash

##
## Create man pages using help2man.
##

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <program-name>"
    exit 1
fi

PROG="$1"
PROGNAME=$(basename "$PROG")
help2man "$PROG" -i "$PROGNAME.h2m" -N > "$PROGNAME.1.err"
./help2man_fixup.py "$PROGNAME.1.err" "$PROGNAME.h2m" "$PROG" > "$PROGNAME.1"
rm "$PROGNAME.1.err"
