#!/usr/bin/gawk -f
match($1, /\[(.*)\/(.*)\]/, a) {print int((a[1] / a[2]) * 100) "%"}
