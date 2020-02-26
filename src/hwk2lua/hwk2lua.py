#!/usr/bin/python3

## =====================================================================================
## hwk2lua
##
## Copyright © 2020 Jonas Møller <jonas.moeller2@protonmail.com>
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
## 
## 1. Redistributions of source code must retain the above copyright notice, this
##    list of conditions and the following disclaimer.
## 2. Redistributions in binary form must reproduce the above copyright notice,
##    this list of conditions and the following disclaimer in the documentation
##    and/or other materials provided with the distribution.
## 
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
## ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
## DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
## DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
## SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
## CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
## OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
## =====================================================================================

from re import MULTILINE, compile
from itertools import takewhile
import sys

ws_rx = compile(r"^[\t ]*")
op_rx = compile(r"(^|(--)?\[=*\[|(--)?\]=*\]|--|\".*?\"|\'.*?\'|=>|{|})", MULTILINE)
cmnt_begin = compile(r"(--)?\[(=*)\[")
cmnt_end = compile(r"(--)?\](=*)\]")

def hwk2lua(text):
    out = []
    segments = []

    ## Break the text into segments
    last = 0
    for m in op_rx.finditer(text):
        start, end = m.span()
        if start != last:
            segments.append(text[last:start])
        segments.append(m.group())
        last = end
    segments.append(text[last:])

    ## Edit the segments into outputs
    last = ""
    scopes = []
    lcomment = None
    in_comment = False
    for s in segments:
        if s == "{" and not (in_comment or lcomment):
            scopes.append(last == "=>")
            out.append("MatchScope.new(function (__match)" if scopes[-1] else "{")
        elif s == "}" and not (in_comment or lcomment):
            if not scopes:
                raise Exception("Unbalanced curly braces (too many: '}')")
            out.append("end)" if scopes.pop() else "}")
        elif s == "=>" and not (in_comment or lcomment):
            ## Pop from output until an empty string (represents newline) is found.
            pops = (out.pop() for _ in range(len(out)))
            match = "".join(reversed(list(takewhile(lambda x: x, pops))))
            _, ws_end = ws_rx.match(match).span()
            out.append(f"{match[:ws_end]}__match[{match.strip()}] =")
        else:
            out.append(s)

        if s == "":
            in_comment = False
        elif s == "--":
            in_comment = True
        elif cmnt_begin.match(s) and not lcomment:
            lcomment = (s.startswith("--"), s.count("="))
        elif cmnt_end.match(s) and (s.startswith("--"), s.count("=")) == lcomment:
            lcomment = None

        if not s.isspace():
            last = s

    if scopes:
        raise Exception("Unbalanced curly braces (too many: '{')")
    return "".join(out)

if __name__ == "__main__":
    try:
        with open(sys.argv[1]) as f:
            print(hwk2lua(f.read()))
        sys.exit(0)
    except IndexError:
        print("Usage: hwk2lua <file>")
    except Exception as e:
        print(str(e))
    sys.exit(1)
