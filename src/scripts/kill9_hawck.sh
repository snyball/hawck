#!/bin/bash

inputd_pid=$(pidof hawck-inputd)
macrod_pid=$(pidof hawck-macrod)
pkexec --user hawck-input "/bin/kill -9 $inputd_pid"
kill -9 $macrod_pid
