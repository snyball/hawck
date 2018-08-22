#!/bin/bash

##
## Download single-header dependencies
##

SRC="$1"
SUBDIR="$2"

DIR="${MESON_SOURCE_ROOT}/src/include-ext/$SUBDIR"
mkdir -p "$DIR"

cd "$DIR"
wget "$SRC"

