#!/usr/bin/python

import sys
from pkg_resources import load_entry_point

if __name__ == "__main__":
    sys.exit(
            load_entry_point("hawck_ui", "main_entry", "start")()
            )
