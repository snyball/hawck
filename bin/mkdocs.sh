#!/bin/bash

BINARIES=(hawck-inputd lsinput hawck-macrod)

for bin in ${BINARIES[@]}; do
    help2man $bin -i "docs/$bin.h2m" -o "docs/$bin.1"
done
