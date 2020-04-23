#!/usr/bin/python3

# =============================================================================
# hwk2lua
#
# Copyright © 2020 Jonas Møller <jonas.moeller2@protonmail.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ==============================================================================

"""
Hwk to Lua transpiler.

Makes the following transformations:
  <pattern> `=>` <action>
    -> __match[<pattern>] = <action>
  <pattern> `=>` { <code> }
    -> __match[<pattern>] = MatchScope.new(function (__match)
           <code>
       end)
"""

from re import MULTILINE, compile as recomp
from itertools import takewhile
from typing import List
import sys

WS_RX = recomp(r"^[\t ]*")
OP_RX = recomp(r"(^|(--)?\[=*\[|(--)?\]=*\]|--|\"(?:[^\"\\]|\\.)*\"|'(?:[^'\\]|\\.)*'|=>|{|})",
               MULTILINE)
CMNT_BEGIN_RX = recomp(r"(--)?\[(=*)\[")
CMNT_END_RX = recomp(r"(--)?\](=*)\]")


class ParseException(Exception):
    """Parse exception"""


def tokenize(hwk_code: str) -> List[str]:
    """Tokenize `hwk_code` according to `OP_RX`"""
    last = 0
    segments = []
    for match in OP_RX.finditer(hwk_code):
        start, end = match.span()
        if start != last:
            segments.append(hwk_code[last:start])
        segments.append(match.group())
        last = end
    segments.append(hwk_code[last:])
    return segments


def hwk2lua(hwk_code: str) -> str:
    """
    Convert `hwk_code` from hwk to lua.
    """
    out = []
    segments = tokenize(hwk_code)

    # Edit the segments into outputs
    last = ""
    scopes = []
    lcomment = None
    in_comment = False
    for seg in segments:
        if seg == "{" and not (in_comment or lcomment):
            scopes.append(last == "=>")
            out.append("MatchScope.new(function (__match)"
                       if scopes[-1] else "{")
        elif seg == "}" and not (in_comment or lcomment):
            if not scopes:
                raise ParseException("Unbalanced curly braces (too many: '}')")
            out.append("end)" if scopes.pop() else "}")
        elif seg == "=>" and not (in_comment or lcomment):
            # Pop from output until an empty string (represents newline) is
            # found.
            pops = (out.pop() for _ in range(len(out)))
            match = "".join(reversed(list(takewhile(lambda x: x, pops))))
            _, ws_end = WS_RX.match(match).span()
            out.append(f"{match[:ws_end]}__match[{match.strip()}] =")
        else:
            out.append(seg)

        if seg == "":
            in_comment = False
        elif seg == "--":
            in_comment = True
        elif CMNT_BEGIN_RX.match(seg) and not lcomment:
            lcomment = (seg.startswith("--"), seg.count("="))
        elif (CMNT_END_RX.match(seg) and
              (seg.startswith("--"), seg.count("=")) == lcomment):
            lcomment = None

        if not seg.isspace():
            last = seg

    if scopes:
        raise ParseException("Unbalanced curly braces (too many: '{')")
    return "".join(out)


if __name__ == "__main__":
    try:
        with open(sys.argv[1]) as f:
            print(hwk2lua(f.read()))
        sys.exit(0)
    except IndexError:
        print("Usage: hwk2lua <file>")
    except ParseException as err:
        print(str(err))
    sys.exit(1)
