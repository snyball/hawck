##
## Compile the project, then:
## Run hawck-inputd and hawck-macrod together,
## and make SIGINT kill both daemons.
##
## This script just makes the edit-compile-run cycle
## a bit easier.
##

## Use
KBD_DEVICE=$(lsinput -s | awk '/Keyboard-event-kbd/{print $1}' | head -n1)

export SPID=$$

function kill_hawck() {
    for pid in $(pidof hawck-inputd hawck-macrod); do
        kill $pid
    done
    kill -9 $SPID
}

trap kill_hawck SIGINT

make -j8
./hawck-inputd "$KBD_DEVICE" &
./hawck-macrod &

## Wait forever
while true; do
    sleep 1
done
