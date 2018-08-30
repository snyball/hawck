#!/usr/bin/python3

"""
Get default privilege escalation method for the system.
We prefer methods in the following order:
  - pkexec
  - gksudo
  - sudo in a terminal (sudo_fallback.py)
"""

from subprocess import Popen, PIPE
from getpass import getuser
from typing import List, Dict, Callable, Any
from pprint import PrettyPrinter
import threading
import re
import sys
import time
import os
import json
import base64

import dill
dill.settings["recurse"] = False

pprint = PrettyPrinter(indent = 4).pprint

PY_CMD = """
import dill, sys, json, base64
fn={fn!r}
sys.stdout.buffer.write({sep!r})
sys.stdout.buffer.write(bytes(dill.loads(base64.b64decode(fn))(), "utf-8"))
"""

class SudoException(Exception):
    pass

class PException(Exception):
    """
    This exception is raised when a function decorated with
    SudoMethod.do raises an exception. The exception name and
    message are kept intact, but the original exception object
    cannot be safely reconstructed as this might allow for
    code execution by i.e raising Popen([...]) inside of
    the sub process.
    """
    def __init__(self, exc_name, msg):
        self.name = exc_name
        self.msg = msg

    def __repr__(self):
        return f"{self.name}({self.msg!r})"
    __str__ = __repr__

class SudoMethod:
    """
    Sudo methods are ways to run commands as other users.
    """
    def __init__(self, base_cmd: str, options: Dict[str, str], *base_flags):
        """
        Checks whether the sudo method exists.
        """
        p = Popen(["which", base_cmd], stdout=PIPE)
        ret = p.wait()
        if ret != 0:
            raise SudoException(f"No such method: {base_cmd}")
        path = p.stdout.read().decode("utf-8").strip()
        self.cmd = path
        self.options = options
        self.base_flags = base_flags

    def getcmd(self, expl: str, user: str, what: List[str]) -> List[str]:
        """
        Get full command for this sudo method.
        """
        full_cmd = [self.cmd]
        full_cmd.extend(self.base_flags)
        if not self.options["user"]:
            full_cmd.append(user)
        else:
            full_cmd.extend([self.options["user"], user,])
        # if "message" in self.options:
        #     full_cmd.extend([self.options["message"], expl])
        # full_cmd.append("--")
        full_cmd.extend(what)
        return full_cmd

    @staticmethod
    def waitP(p: Popen, fn: Callable[[int], None]) -> None:
        ret = p.wait()
        fn(ret)

    def runCommand(self,
                   expl: str,
                   user: str,
                   command: List[str],
                   *callback: Callable[[int], None]) -> None:
        """
        Run a command as a different user.
        Can optionally accept a callback to run the command
        asynchronously in a thread.
        """
        p = Popen(self.getcmd(user, command))
        if callback:
            fn, *_ = callback
            t = threading.Thread(target=SudoMethod.waitP, args=(p, fn))
            t.start()
        else:
            ret = p.wait()
            if ret != 0:
                raise SudoException(f"Command returned failure status: {ret}")

    def run(self, expl: str, user: str, fn: Callable[[Any], Any], args: List[Any], *callback) -> Any:
        """
        This function is used by SudoMethod.do and SudoMethod.do_async, it is not
        meant to be called manually.
        """
        def wrap():
            try:
                ret = json.dumps([None, fn(*args)])
            except Exception as e:
                ret = json.dumps([(type(e).__name__, str(e)), None])
            return ret
        fn_str = dill.dumps(wrap, byref=False, fmode=False, recurse=False)
        sep = bytes(f"BEGIN PYTHON RETURN {os.getpid()}-{time.time()}", "utf-8")
        py_cmd = [sys.executable, "-c", PY_CMD.format(fn=base64.b64encode(fn_str), sep=sep)]
        cmd = self.getcmd(expl, user, py_cmd)
        p = Popen(cmd, stdout=PIPE)
        def callback_wrap(callback):
            ret = p.wait()
            if ret != 0:
                raise SudoException(f"Command returned failure status: {ret}")
            out = p.stdout.read()
            print("Output:")
            try:
                _, json_str = out.split(sep)
            except:
                raise SudoException("Unable to retrieve output of function")
            exc, obj = json.loads(json_str)
            if exc:
                raise PException(*exc)
            return callback(exc, obj)
        if callback:
            t = threading.Thread(target=callback_wrap, args=(callback[0],))
            t.start()
        else:
            return callback_wrap(lambda exc, obj: obj)

    def do(self, user: str, *expl: str):
        """
        Returns decorator for priviliged functions that run synchronously.
        """
        expl, *_ = [*expl, "Python"]
        def sub(fn):
            def sub(*args):
                return self.run(expl, user, fn, args)
            return sub
        return sub

    def do_async(self, user: str, *expl: str):
        """
        Returns decorator for priviliged functions that run asynchronously.
        This will spawn a thread that waits for the function to terminate.
        """
        expl, *_ = [*expl, "Python"]
        def sub(fn):
            def callback(exc, obj):
                if exc:
                    raise exc
            def sub(*args):
                return self.run(expl, user, fn, args, callback)
            return sub
        return sub

def checkPkexec():
    p = Popen(["/sbin/ps", "-Ao", "args"], stdout=PIPE, env={})
    rx = re.compile("^.*polkit-.*authentication-agent.*$", re.MULTILINE)
    s = rx.findall(p.stdout.read().decode("utf-8"))
    ret = p.wait()
    if ret != 0:
        return False
    return s

def assume():
    return True

cmds = [
    [checkPkexec , "pkexec",           {"user": "--user"}, "--disable-internal-agent" ],
    ## GKsudo does not work, specifically it fucks with the output streams of
    ## the attached program, it fails silently, and it misinterprets program arguments
    ## as arguments related to itself. The only redeeming feature is that it lets
    ## you set a custom message.
    #[assume      , "gksudo",           {"user": "--user", "message": "--message"},    ],
    [assume      , "sudo_fallback.py", {"user": None}                                 ]
]

def getSudoMethod():
    methods = []
    for args in cmds:
        try:
            check, *args = args
            if not check():
                continue
            return SudoMethod(*args)
        except SudoException:
            pass
    raise SudoException("No methods found")

if __name__ == "__main__":
    su = getSudoMethod()

    @su.do("root")
    def esc(arg):
        import getpass
        with open("/tmp/test", "w") as wf:
            wf.write(f"Test from {getpass.getuser()}\n")
        if arg:
            raise FileNotFoundError("E")
        return getpass.getuser()

    ret = None
    try:
        ret = esc(None)
    except SudoException:
        print("Unable to acquire root access")
        sys.exit(0)

    print(f"ret: {ret}")

    try:
        ret = esc("WHY?")
    except PException as e:
        print(str(e))
    except SudoException as e:
        ## This might happen if the user clicks 'Cancel' on the popup.
        print("Unable to acquire root access")
        sys.exit(0)

    print(f"ret: {ret}")
