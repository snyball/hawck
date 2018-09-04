#!/bin/bash

########################################################################################
##                                                                                    ##
## Shell script for installing hwk scripts.                                           ##
##                                                                                    ##
## Copyright (C) 2018 Jonas Møller (no) <jonasmo441@gmail.com>                        ##
## All rights reserved.                                                               ##
##                                                                                    ##
## Redistribution and use in source and binary forms, with or without                 ##
## modification, are permitted provided that the following conditions are met:        ##
##                                                                                    ##
## 1. Redistributions of source code must retain the above copyright notice, this     ##
##    list of conditions and the following disclaimer.                                ##
## 2. Redistributions in binary form must reproduce the above copyright notice,       ##
##    this list of conditions and the following disclaimer in the documentation       ##
##    and/or other materials provided with the distribution.                          ##
##                                                                                    ##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND    ##
## ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED      ##
## WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE             ##
## DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE       ##
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL         ##
## DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR         ##
## SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER         ##
## CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,      ##
## OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE      ##
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.               ##
##                                                                                    ##
########################################################################################

HAWCKD_INPUT_USER=hawck-input
SCRIPTS_DIR="$HOME/.local/share/hawck/scripts/"

script_path="$1"
name=$(basename "$script_path" | sed -r 's/\.[^.]+$//')
keys_filename="$name.csv"
real_keys="/var/lib/hawck-input/keys/$keys_filename"

## Transpile hwk script to Lua
hwk_out=$(hwk2lua "$script_path")
if [ $? != 0 ]; then
    echo "!ERROR: $hwk_out" >&2
    exit 1
fi

## Move script into the script directory
script_path=$(realpath "$SCRIPTS_DIR/$name.lua")
echo "Installing script to: $script_path" >&2
echo "$hwk_out" > "$script_path"
chmod 744 "$script_path"

cd "$SCRIPTS_DIR"

## Check the script for correctness
if ! lua5.3 -l init "$script_path" >&2; then
    echo "!ERROR: Lua" >&2
    exit 1
fi

## Create CSV file with proper header and permissions
tmp_keys=$(mktemp)
chmod 644 "$tmp_keys"
echo "key_name,key_code" > "$tmp_keys"

## Write keys to CSV file.
## Output is sorted as the Lua table iteration is
## not deterministic.
lua5.3 -l init -l "$name" -e '
for name, _ in pairs(__keys) do
    local succ, key = pcall(function ()
        return kbd:getKeysym(name)
    end)
    if not succ then
        key = -1
    end
    print("\"" .. name .. "\"" .. "," .. key)
end
' | sort >> "$tmp_keys"

## Either the files are different or the $real_keys file does not exist.
if ! cmp "$real_keys" "$tmp_keys" >&2; then
    ## Check if we can execute the copy command:
    if [ "$(whoami)" = "$HAWCKD_INPUT_USER" ]; then
       cp "$tmp_keys" "$real_keys"
       chmod 644 "$real_keys"
    else
        ## Send key requests to the input daemon.
        notify-send -a "Hawck install" "Require authentication as hawck-input" &>/dev/null
        echo -n "$tmp_keys"
        exit 123
    fi
fi
