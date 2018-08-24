#!/usr/bin/gawk -f

##
## Consume output from ninja and turn it into a series
## of percentages.
##

BEGIN {
    mod_perc = 1
    if (ARGC >= 3) {
        start_perc = ARGV[1]
        mod_perc = ARGV[2] / 100
    }
    for (i = 0; i < ARGC; i++)
        delete ARGV[i]
}

match($1, /\[(.*)\/(.*)\]/, a) {
    print int((a[1] / a[2]) * mod_perc * 100)+start_perc "%"
    fflush(stdout)
}
