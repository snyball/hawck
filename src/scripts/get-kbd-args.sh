#!/bin/bash

##
## Load keyboard name arguments from /var/lib/hawck-input/*
## and look up their entries in /dev/input/event*
##

xargs -d"\n" /usr/share/hawck/bin/get-kbd.sh  < /var/lib/hawck-input/keyboards.txt
