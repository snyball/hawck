#!/bin/bash

########################################################################################
##                                                                                    ##
## Shell script for installing hwk scripts.                                           ##
##                                                                                    ##
## Copyright (C) 2018 Jonas Møller (no) <jonas.moeller2@protonmail.com>               ##
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
SCRIPTS_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/hawck/scripts/"

if [ $# != 1 ] || [ "$1" = "--help" ]; then
    echo "Usage: $(basename "$0") <script>"
    [ "$1" = "--help" ] || exit 1
    exit 0
fi

script_path="$1"
name="$(basename "$script_path" | sed -r 's/\.[^.]+$//')"
keys_filename="$name.csv"
real_keys="/var/lib/hawck-input/keys/$keys_filename"

if ! [ -f "$script_path" ]; then
    echo "No such file: $script_path"
    exit 1
fi

chmod 0755 -- "$script_path"

## Transpile hwk script to Lua
hwk_out="$(hwk2lua "$script_path")"
if [ $? != 0 ]; then
    echo "$hwk_out" >&2
    exit 1
fi

## Move script into the script directory
script_path="$(realpath "$SCRIPTS_DIR/$name.lua")"
echo "$hwk_out" > "$script_path"

cd "$SCRIPTS_DIR"

## Check the script for correctness
lua5.3 -l init "$script_path" >&2 || exit 1

function rand-file-path() {
    head -c128 /dev/urandom | sha1sum | awk '{print "/var/lib/hawck-input/tmp/"$1}'
}

function mktemp() {
    file="$(rand-file-path)"
    while [[ -f "$file" ]]; do
        file="$(rand-file-path)"
    done
    touch "$file"
    echo "$file"
}

## Create CSV file with proper header and permissions
tmp_keys="$(mktemp)"
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

rm "$script_path"

## Either the files are different or the $real_keys file does not exist.
if ! cmp "$real_keys" "$tmp_keys" &> /dev/null; then
    echo "New keys added, require authentication for whitelisting them ..."
    command sudo --user="$HAWCKD_INPUT_USER" -- install -m 644 "$tmp_keys" "$real_keys"
fi
