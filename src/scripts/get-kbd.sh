#!/bin/bash

RX="$1"
shift

while [ $# -gt 0 ]; do
    RX="$RX|$1"
    shift
done

lsinput -s | grep -v if01 | grep -E "$RX" | awk -vFS='\t' '{print $1}'
