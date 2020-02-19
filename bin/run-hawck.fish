#!/usr/bin/env fish

##
## Test script for running Hawck while developing it.
##
## You need fish shell to run it.
##

set ttl 10

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
set inputd_args
for device in (lskbd)
    set -a inputd_args "--kbd-device"
    set -a inputd_args $device
end

killall hawck-macrod
sudo systemctl stop hawck-inputd.service

echo "Killing in $ttl seconds"

echo "--------------------------[hawck]--------------------------"
sudo runuser -u hawck-input -- ./hawck-inputd $inputd_args
./hawck-macrod --no-fork &

sleep $ttl
kill (jobs -p)
sudo killall hawck-inputd
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
