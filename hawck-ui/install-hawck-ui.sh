#!/bin/bash

cd "${MESON_SOURCE_ROOT}/hawck-ui"

## Assume that setup.py build has already been run, this
## is to avoid root owning all the build files.
if ! ./setup.py install --skip-build; then
    echo "Failure in ./setup.py install"
    exit 1
fi

## Will be owned by root, just remove it now.
rm -r dist

## Install executable python entry point.
install -m 755 bin/hawck-ui.py /usr/local/bin/hawck-ui
