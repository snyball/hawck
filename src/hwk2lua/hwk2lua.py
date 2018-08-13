#!/usr/bin/python

##
## Simple prototype for hwk2lua
##

## TODO: Handle strings like this: [[ string ]]

from re import MULTILINE, compile as rec
import sys

ws_rx = rec(" +")
op_rx = rec('(^|\-\-|".*?"|\'.*?\'|=>|{|})', MULTILINE)

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
    scope = 0
    scopes = []
    in_comment = False
    for s in segments:
        if s.strip() == "{" and not in_comment:
            if last == "=>":
                out.append("MatchScope.new(function (__match)")
                scopes.append(scope)
            else:
                out.append("{")
            scope += 1
        elif s.strip() == "}" and not in_comment:
            scope -= 1
            if scope < 0:
                raise Exception("Unabalenced curly braces (closed too many.)")
            if scopes and scopes.pop() == scope:
                out.append("end)")
        elif s.strip() == "=>" and not in_comment:
            items, item = [], True
            while item:
                item = out.pop()
                items.append(item)
            items.append(item)
            match = "".join(reversed(items))
            ws = ws_rx.match(match)
            _, ws_end = ws.span() if ws else (0, 0)
            out.append(f"{match[:ws_end]}__match[{match[ws_end:]}] =")
        else:
            if s == "":
                in_comment = False
                # out.append("##")
            elif s.strip().startswith("--"):
                in_comment = True
            out.append(s)
        if not s.isspace():
            last = s
    if scope > 0:
        raise Exception("Unabalenced curly braces (not closed.)")

    ## Assemble pieces
    return "".join(out)

if __name__ == "__main__":
    if len(sys.argv) >= 2:
        with open(sys.argv[1]) as f:
            print(hwk2lua(f.read()))
    else:
        print("Usage: hwk2lua <file>")
        sys.exit(1)
