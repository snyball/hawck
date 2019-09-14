#!/bin/bash

journalctl -r -ojson | head -n 1000 | grep --color=never hawck-macrod | head -n 20
