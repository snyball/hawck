#!/bin/bash

pushd "${MESON_SOURCE_ROOT}/hawck-ui"
python3 setup.py install --skip-build
rm -r dist
popd
exit 0
