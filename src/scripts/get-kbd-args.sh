#!/bin/bash

##
## Load keyboard name arguments from /var/lib/hawck-input/*
## and look up their entries in /dev/input/event*
##
## This script is meant to be used like this:
##   hawck-inputd $(get-kbd-args.sh)
##

IFS=$'\n'

## Get regex from keyboards.txt
rx=$(cat /var/lib/hawck-input/keyboards.txt | sed '/^ *$/d' | sed '/^#/d' | tr '\n' '|' | sed 's/|$//g')

## Filter lsinput output
for device in $(lsinput -s | awk -vFS='\t' -vRX="$rx" '$2 ~ RX && $4 !~ /if0/ && $2 !~ "Hawck virtual keyboard" {print $1}'); do
    echo "--kbd-device $device"
done | sort | uniq | tr '\n' ' '

