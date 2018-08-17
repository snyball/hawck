#!/bin/bash
##
## Compile the project, then:
## Run hawck-inputd and hawck-macrod together,
## and make SIGINT kill both daemons.
##
## Devices to listen for are given in order of priority
## as command line arguments.
##
## For example:
##    ./run_hawck.sh "Logitech G710" "AT Translated"
##

if ! which lsinput &>/dev/null; then
    echo "Did not find lsinput in PATH, please install it."
    exit 1
fi

echo "Probing devices ..."
DEVICES=( "$@" )
## Loop over arguments to find a suitable device to listen on.
while [ "$#" -ge 1 ]; do
    KBD_MODEL="$1"
    KBD_DEVICE=$(lsinput -s | grep -v if0 | awk "/$KBD_MODEL/"'{print $1}' | head -n1)
    if [ "$KBD_DEVICE" != "" ]; then
        break
    fi
    shift
done

## Did not find any device.
if [ "$KBD_DEVICE" = "" ]; then
    echo "Unable to find any of the following devices:"
    for dev in "${DEVICES[@]}"; do
        echo "    $dev"
    done
    echo "Run lsinput to list input devices"
    exit 1
fi
echo "Found device: $KBD_DEVICE"

## Export PID of current shell to kill_hawck
export SPID=$$

## Interrupt handler for ^C
function kill_hawck() {
    for pid in $(pidof hawck-inputd hawck-macrod); do
        kill $pid
    done
    kill $SPID
}

trap kill_hawck SIGINT

if ! make -j8; then
    exit 1
fi

./hawck-inputd "$KBD_DEVICE" &
./hawck-macrod &

## Wait for interrupt
while true; do
    sleep 1
done
