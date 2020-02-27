#!/usr/bin/env fish

##
## Test script for running Hawck while developing it.
##
## You need fish shell to run it.
##

set ttl 7
if [ (count $argv) -gt 0 ]
    set ttl $argv[1]
end

cd (dirname (status -f))
while [ (basename $PWD) != "Hawck" ]
    cd ..
end

pushd build
if ! ninja
    echo "Build failed"
    exit 1
end

mkdir -p macrod-test

begin
    set -l MACROD_EXE src/hawck-macrod
    set -l INPUTD_EXE src/hawck-inputd
    set -l XDG_ROOT (realpath macrod-test)
    set -lx XDG_DATA_HOME $XDG_ROOT/.local/share
    set -lx XDG_CONFIG_HOME $XDG_ROOT/.config
    set -lx XDG_CACHE_HOME $XDG_ROOT/.cache

    mkdir -p $XDG_CONFIG_HOME
    mkdir -p $XDG_DATA_HOME
    mkdir -p $XDG_CACHE_HOME

    set SCRIPTS $XDG_DATA_HOME/hawck/scripts
    mkdir -p $SCRIPTS
    [ -L $SCRIPTS/LLib ]                || ln -s (realpath ../src/Lua) $SCRIPTS/LLib
    [ -L $SCRIPTS/init.lua ]            || ln -s (realpath ../src/Lua/init.lua) $SCRIPTS/init.lua
    [ -L $SCRIPTS/keymaps ]             || ln -s (realpath ../keymaps) $SCRIPTS/keymaps
    [ -L $XDG_DATA_HOME/hawck/cfg.lua ] || ln -s (realpath ../bin/cfg.lua) $XDG_DATA_HOME/hawck/cfg.lua

    set kbd (find /dev/input/by-path/ -type l | grep -E "platform-i8042.*-event-kbd\$")
    set inputd_args --no-hotplug --kbd-device $kbd

    sudo systemctl stop hawck-inputd.service
    killall hawck-macrod

    echo "Killing in $ttl second(s)"

    echo "--------------------------[hawck]--------------------------"
    command sudo runuser -u hawck-input -- $INPUTD_EXE $inputd_args
    $MACROD_EXE --no-fork &
    sleep $ttl
    kill -9 (jobs -p)
    sudo killall hawck-inputd
    echo
    echo "---------------------------[DONE]--------------------------"
end

popd
echo "Restarting system Hawck"
sudo systemctl start hawck-inputd.service
hawck-macrod
