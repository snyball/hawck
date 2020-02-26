#!/usr/bin/python3

## TODO: Handle strings like this: [[ string ]]

from re import MULTILINE, compile
from itertools import takewhile
import sys

ws_rx = compile(r"^[\t ]*")
op_rx = compile('(^|\-\-|".*?"|\'.*?\'|=>|{|})', MULTILINE)

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
    in_comment = False
    for s in segments:
        if s == "{" and not in_comment:
            scopes.append(last == "=>")
            out.append("MatchScope.new(function (__match)" if scopes[-1] else "{")
        elif s == "}" and not in_comment:
            if not scopes:
                raise Exception("Unbalenced curly braces (too many: '}')")
            out.append("end)" if scopes.pop() else "}")
        elif s == "=>" and not in_comment:
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

        if not s.isspace():
            last = s

    if scopes:
        raise Exception("Unbalenced curly braces (too many: '{')")
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
