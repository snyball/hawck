#!/usr/bin/env fish

##
## Test script for running Hawck while developing it.
##
## You need fish shell to run it.
##

set ttl 5

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

mkdir -p macrod-test/hawck/scripts
if ! [ -L macrod-test/hawck/scripts/LLib ]
    ln -s (realpath ../src/Lua macrod-test/hawck/scripts/LLib)
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

sudo systemctl stop hawck-inputd.service
killall hawck-macrod

echo "Killing in $ttl seconds"

echo "--------------------------[hawck]--------------------------"
sudo runuser -u hawck-input -- ./hawck-inputd $inputd_args
./hawck-macrod --no-fork &

sleep $ttl
kill -9 (jobs -p)
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
