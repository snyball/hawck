#!/bin/bash

##
## Create man pages using pandoc.
##

if [ "$#" -lt 1 ]; then
    echo "Usage: $0 <program-name>"
    exit 1
fi

PROG="$1"
pandoc --from=markdown --standalone "$PROG.md" --to=man -o "$PROG.1"
