## ================================================================================
## log_retriever.py is a part of hawck-ui, which is distributed under the
## following license:
##
## Copyright (C) 2018 Jonas Møller (no) <jonasmo441@gmail.com>
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
## SOFTWARE.
## ================================================================================

import re
import json
from subprocess import Popen, PIPE, STDOUT as STDOUT_REDIR
from collections import defaultdict
import os
import threading
import time
from typing import Callable
from gi.repository import GLib

from hawck_ui.locations import HAWCK_HOME, LOCATIONS

class LogRetriever(threading.Thread):
    def __init__(self, gdk_callback: Callable[[list, int], bool]):
        super().__init__()
        self.gdk_callback = gdk_callback
        self.logs = {}
        self.max_logs = 200
        self.last_time = 0
        self.dismissed = set()
        self.running = True

    def run(self):
        GLib.threads_init()
        while True:
            if self.update():
                GLib.idle_add(self.gdk_callback, [v for k,v in self.logs.items()])

            ## Just stop calling the gdk_callback
            while not self.running:
                time.sleep(1)

            time.sleep(0.5)

    def stop(self):
        self.running = False

    def resume(self):
        self.running = True

    def mklog(log):
        """
        Warning: This function modifies the log passed to it,
                 the same log object is then returned for convenience.
        """
        lua_err_rx = re.compile(r"^(.+):(\d)+: (.*)", re.MULTILINE | re.DOTALL)
        log["TYPE"], _, log["MESSAGE"] = log["MESSAGE"].partition(":")
        if log["TYPE"].upper() == "LUA":
            m = lua_err_rx.match(log["MESSAGE"])
            log["LUA_FILE"], log["LUA_LINE"], log["LUA_ERROR"] = m.groups()
            log["LUA_LINE"] = int(log["LUA_LINE"])
        items = list(log.items())
        for (k, v) in items:
            if type(v) == str and v.isdigit():
                log[k] = int(v)
        log["UTIME"] = log["__MONOTONIC_TIMESTAMP"]
        return log

    def dismiss(self, log_message):
        """
        Filter out logs with log["MESSAGE"] == log_message.
        """
        print(f"Filtering of {log_message!r} is not implemented")

    def update(self):
        """
        Update logs then return the ones that were read.
        Returns None if nothing changed.
        """
        p = Popen(["journalctl", "-n", "10000", "-o", "json"], stdout=PIPE)

        logs = []
        for i, line in enumerate(reversed(p.stdout.readlines())):
            obj = json.loads(line.decode("utf-8").strip())
            if os.path.basename(obj.get("_EXE", "")) != "hawck-macrod":
                continue
            obj = LogRetriever.mklog(obj)
            ## Log has been read, stop
            if obj["UTIME"] <= self.last_time or len(logs) > self.max_logs:
                break
            logs.append(obj)

        p.kill()

        if not logs:
            return

        log = None
        for log in reversed(logs):
            msg = log["MESSAGE"]
            if msg not in self.logs:
                log["DUP"] = 1
                self.logs[msg] = log
            else:
                self.logs[msg]["DUP"] += 1
        self.last_time = log["UTIME"]

        return logs
