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
ninja || exit 1
mkdir -p macrod-test

function print_line
    set -l dash "┫" "━" "┣"
    set -l dashes (math (tput cols) - (echo -n "$dash[1]$dash[2]$argv[1]" | string length))
    echo -ne "\r"
    set_color --bold white
    echo -n (string repeat -n (math "floor($dashes / 2)" ) $dash[2])
    echo -n $dash[1]
    set_color $argv[2]
    echo -n $argv[1]
    set_color --bold white
    echo -n $dash[3]
    echo (string repeat -n (math "floor($dashes / 2) + ($dashes % 2)") $dash[2])
    set_color normal
end

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

    print_line "START" yellow
    command sudo runuser -u hawck-input -- $INPUTD_EXE $inputd_args
    switch $argv[1]
        case "killcycle"
            for i in (seq 6)
                $MACROD_EXE --no-fork &
                set -l ttl (math (random) % 5 + 1)
                echo -e "\nmacrod "(set_color green --bold --dim)"ALIVE"(set_color normal)" for $ttl seconds"
                sleep $ttl
                kill -9 (jobs -p)
                set -l ttl (math (random) % 5 + 1)
                echo -e "\nmacrod "(set_color red --bold --dim)"DEAD"(set_color normal)" for $ttl seconds"
                sleep $ttl
            end

        case "valgrind"
            valgrind $MACROD_EXE --no-fork &
            sleep 10
            kill (jobs -p)
            wait

        case "*"
            echo "Killing in $ttl second(s)"
            $MACROD_EXE --no-fork &
            sleep $ttl
            kill -9 (jobs -p)
    end
    sudo killall hawck-inputd
    echo
    print_line "END" red
end

popd
echo "Restarting system Hawck"
sudo systemctl start hawck-inputd.service
hawck-macrod
