#!/usr/bin/awk -f

##
## Get word frequency in csv format
##
## The input is assumed to be a csv file with the following columns:
##   word,class
## Where class is a number corresponding to the following formula:
##   N = floor(0.5 - log_2(frequency / highest_frequency)
## Where `frequency` denotes the frequency of a word, and
## `highest_frequency` is the frequency of the most used
## word, in English this would be `the`
##

BEGIN {
    FPAT = "([^,]*)|(\"[^\"]+\")"
    PROCINFO["sorted_in"] = "@val_type_asc"
}

{
    word = $1
    class = $2
    split(word, lt, "")

    for (i in lt)
        a[lt[i]] += 1 / (class+1)
}

function quot(s) {
    if (s == "\"")
        return "\"\"\"\""
    return "\"" s "\""
}

END {
    OFS=","
    for (u in a)
        sum += a[u]
    print "letter", "frequency"
    for (u in a) {
        perc = 100*(a[u]/sum)
        if (perc >= 0.01)
            print quot(u), quot(perc)
    }
}
