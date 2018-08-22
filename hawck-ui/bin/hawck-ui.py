#!/usr/bin/python3

import sys
from pkg_resources import load_entry_point, require

if __name__ == "__main__":
    version = require("hawck_ui")[0].version
    sys.exit(
        load_entry_point("hawck_ui", "main_entry", "start")(version)
    )
