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

echo "$PWD"

pushd build
if ! ninja
    echo "Build failed"
    exit 1
end

set HWKDIR macrod-test/hawck/scripts
mkdir -p $HWKDIR
if ! [ -L $HWKDIR/LLib ]
    ln -s (realpath ../src/Lua $HWKDIR/LLib)
end
if ! [ -L $HWKDIR/init.lua ]
    ln -s (realpath ../src/Lua/init.lua $HWKDIR/init.lua)
end
if ! [ -L $HWKDIR/keymaps ]
    ln -s (realpath ../keymaps $HWKDIR/keymaps)
end
if ! [ -L macrod-test/hawck/cfg.lua ]
    ln -s (realpath ../bin/cfg.lua macrod-test/hawck/cfg.lua)
end

if set -q XDG_DATA_HOME
    set OLD_XDG_DATA_HOME $XDG_DATA_HOME
end
set -x XDG_DATA_HOME (realpath macrod-test)

pushd src
set kbd (find /dev/input/by-path/ -type l | grep -E "platform-i8042.*-event-kbd\$")
set inputd_args --no-hotplug --kbd-device $kbd

sudo systemctl stop hawck-inputd.service
killall hawck-macrod

echo "Killing in $ttl second(s)"

echo "--------------------------[hawck]--------------------------"
sudo runuser -u hawck-input -- ./hawck-inputd $inputd_args
./hawck-macrod --no-fork &

sleep $ttl
kill (jobs -p)
sudo killall hawck-inputd
echo
echo "---------------------------[DONE]--------------------------"

popd
popd

if set -q OLD_XDG_DATA_HOME
    set -x XDG_DATA_HOME $OLD_XDG_DATA_HOME
else
    set -e XDG_DATA_HOME
end

echo "Restarting system Hawck"
sudo systemctl start hawck-inputd.service
hawck-macrod
