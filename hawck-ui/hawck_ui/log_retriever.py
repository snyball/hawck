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
import os

from hawck_ui.locations import HAWCK_HOME, LOCATIONS

class LogRetriever:
    def __init__(self):
        self.logs = []
        self.max_logs = 5 
        self.last_log_text = ""
        self.last_monotonic_time = 0
        self.dismissed = set()

    def append(self, log):
        """
        Return 1 if any logs needed to be popped, 0 otherwise.
        """
        num_removed = 0
        if len(self.logs) >= self.max_logs:
            self.logs = self.logs[1:]
            num_removed = 1
        self.logs.append(log)
        return num_removed

    def mklog(log):
        lua_err_rx = re.compile(r"^(.+):(\d)+: (.*)")
        log["TYPE"], _, log["MESSAGE"] = log["MESSAGE"].partition(":")
        if log["TYPE"].upper() == "LUA":
            m = lua_err_rx.match(log["MESSAGE"])
            log["LUA_FILE"], log["LUA_LINE"], log["LUA_ERROR"] = m.groups()
            log["LUA_LINE"] = int(log["LUA_LINE"])
        items = list(log.items())
        for (k, v) in items:
            if type(v) == str and v.isdigit():
                log[k] = int(v)
        return log

    def dismiss(self, log_message):
        """
        Filter out logs with log["MESSAGE"] == log_message.
        """
        print(f"Filtering of {log_message!r} is not implemented")
        pass

    def update(self):
        """
        Update logs, return new logs as well as the number of logs that
        were removed.
        """
        p = Popen(
            [os.path.join(LOCATIONS["hawck_bin"],
                            "get-lua-errors.sh"),
            ], stdout=PIPE, stderr=STDOUT_REDIR)
        ret = p.wait()
        if ret != 0:
            return [], 0
        out = p.stdout.read().decode("utf-8")
        self.last_log_text = out
        added = []
        objs = []
        for line in out.splitlines():
            obj = json.loads(line.strip())
            objs.append(LogRetriever.mklog(obj))
        objs = sorted(objs, key=lambda o: o["__MONOTONIC_TIMESTAMP"])
        truncated_objs = []
        last_log = None
        if self.logs:
            last_log = self.logs[-1]
        skips = 0
        for obj in objs:
            ## Skip same messages
            if last_log and obj["MESSAGE"] == last_log["MESSAGE"]:
                skips += 1
            else: ## New log
                if last_log:
                    last_log["DUP"] = last_log.get("DUP", 1) + skips
                    truncated_objs.append(last_log)
                skips = 0
                last_log = obj
        if last_log:
            last_log["DUP"] = last_log.get("DUP", 1) + skips
            truncated_objs.append(last_log)
        added = list(o for o in truncated_objs if o["__MONOTONIC_TIMESTAMP"] > self.last_monotonic_time)
        if added:
            self.last_monotonic_time = added[-1]["__MONOTONIC_TIMESTAMP"]
        if len(added) > self.max_logs:
            added = added[self.max_logs:]
        num_removed = 0
        for obj in added:
            num_removed += self.append(obj)
        return added, num_removed
