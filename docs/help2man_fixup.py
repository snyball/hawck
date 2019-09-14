#!/usr/bin/python

##
## help2man is a really neat tool, but it messes
## up unicode characters, transforming them into
## ??, this script fixes that.
##
## It requires that no unicode characters are
## present in the help output.
##
## TODO: Make this work for programs that have
##       unicode characters in their -h output.

"""
Usage:
    help2man_fixup.py <manpage> <h2m> <program>
"""

import re, sys, docopt
from subprocess import Popen, PIPE

args = docopt.docopt(__doc__)
try:
    with open(args["<manpage>"]) as rf:
        manp = rf.read()
    with open(args["<h2m>"]) as rf:
        h2m = rf.read()
except FileNotFoundError as e:
    print(f"Unable to open file: {e.filename!r}")
    sys.exit(1)

program = args["<program>"]
p = Popen([program, "-h"], stdout=PIPE)
help_out = p.stdout.read()
if p.wait() != 0:
    print(f"Subcommand {program!r} returned exit status with argument '-h'")
    sys.exit(1)

qrx = re.compile(r"\?{2}")
chars = (c for c in h2m if ord(c) > 0x7f)
segments = qrx.split(manp)

for seg in segments:
    sys.stdout.write(seg)
    try:
        sys.stdout.write(next(chars))
    except StopIteration:
        pass
