#!/bin/bash

if [ $(basename $(pwd -P)) = 'bin' ]; then
    cd ..
fi

if ! [ -d build ]; then
    mkdir build
fi
cp bin/run-tests.sh build/
cp bin/run-hawck.sh build/
cd build
meson
meson configure -Ddesktop_user=$(whoami)
ninja
