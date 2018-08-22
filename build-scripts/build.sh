#!/bin/bash

if [ $(basename $(pwd -P)) = 'build-scripts' ]; then
    cd ..
fi

if ! [ -d build ]; then
    mkdir build
fi
cp build-scripts/run-tests.sh build/
cp build-scripts/run-hawck.sh build/
cd build
meson
meson configure -Ddesktop_user=$(whoami)
ninja
