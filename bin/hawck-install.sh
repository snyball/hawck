#!/bin/bash

##
## Install all Hawck files, this script is run by Meson.
##

function setup-files() {
    pushd "${MESON_SOURCE_ROOT}" &>/dev/null

    HAWCK_SHARE="$DESTDIR/usr/share/hawck"
    HAWCK_BIN="$HAWCK_SHARE/bin"
    for src in src/scripts/*.sh; do
        install -m 755 -D "$src" "$HAWCK_BIN/$(basename "$src")"
    done
    install -m 755 -D bin/hawck-user-setup.sh "$HAWCK_BIN/hawck-user-setup"
    install -m 755 -D src/tools/lskbd.rb "$HAWCK_BIN/lskbd"
    install -m 644 -D bin/__UNSAFE_MODE.csv  "$HAWCK_SHARE/keys/__UNSAFE_MODE.csv"
    install -m 644 -D bin/hawck-macrod.desktop "$HAWCK_SHARE/bin/hawck-macrod.desktop"
    install -m 644 -D bin/cfg.lua "$DESTDIR/etc/hawck/cfg.lua"
    install -m 644 -D src/macro-scripts/example.hwk "$DESTDIR/etc/hawck/scripts/example.hwk"
    install -m 644 -D bin/hawck-macrod.service "$DESTDIR/usr/lib/systemd/user/hawck-macrod.service"

    for lib in src/Lua/*.lua; do
        install -m 755 -D "$lib" "$HAWCK_SHARE/LLib/$(basename "$lib")"
    done
    cp -r keymaps $HAWCK_SHARE/keymaps
    cp -r icons $HAWCK_SHARE/icons

    BIN="$DESTDIR/usr/local/bin/"

    install -m 755 src/scripts/install-hwk-script.sh "$BIN/hawck-add"
    install -m 755 src/hwk2lua/hwk2lua.py "$BIN/hwk2lua"

    ## Copy icons
    pushd "icons" &>/dev/null
    for res in 32 64 128 256 512; do
        res2="$res"'x'"$res"
        icon_src="alt_hawck_logo_v2_red_$res2.png"
        if [ -f "$icon_src" ]; then
            install -m 644 -D "$icon_src" "$DESTDIR/usr/share/icons/hicolor/$res2/apps/hawck.png"
        fi
    done
    popd &>/dev/null

    ## Install rules to make /dev/uinput available to uinput users
    install -m 644 -D bin/99-hawck-input.rules "$DESTDIR/etc/udev/rules.d/99-hawck-input.rules"

    ## Make sure the uinput module is loaded, this isn't necessary on
    ## some systems, but is required on Arch Linux. If this isn't done
    ## the 99-hawck-input.rules file will have no effect.
    install -m 644 -D bin/hawck-uinput.conf "/etc/modules-load.d/hawck-uinput.conf"

    install -m 644 -D bin/hawck-inputd.service "$DESTDIR/etc/systemd/system/hawck-inputd.service"
    # TODO: Copy hawck-macrod.service

    chmod -R a+r "$HAWCK_SHARE"
    find "$HAWCK_SHARE" -type d -exec chmod a+x '{}' \;

    popd &>/dev/null ## Done installing files
}

setup-files
