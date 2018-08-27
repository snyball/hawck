#!/usr/bin/awk -f

##
## Filter out keyboards from lsinput -s
##

BEGIN {
    ORS=" "
    FS="\t"
}

$3 ~ /event-kbd$/ && $4 !~ /if01/ && $2 !~ /Hawck virtual device/ {
    print "--kbd-device " $1
}
