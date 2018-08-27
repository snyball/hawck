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

import dill
dill.settings["recurse"] = False

pprint = PrettyPrinter(indent = 4).pprint

PY_CMD = """
import dill, sys
__out = sys.stdout
sys.stdout = sys.stderr
try:
    fn = dill.loads({fn!r})
    ret = (None, fn())
except Exception as e:
    ret = (e, None)
__out.buffer.write({sep!r})
__out.buffer.write(dill.dumps(ret))
"""

class SudoException(Exception):
    pass

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
        fn_str = dill.dumps(lambda: fn(*args),
                            byref=False, fmode=False, recurse=False)
        sep = bytes(f"BEGIN PYTHON DILL DUMP {os.getpid()}-{time.time()}", "utf-8")
        py_cmd = [sys.executable, "-c", PY_CMD.format(fn=fn_str, sep=sep)]
        cmd = self.getcmd(expl, user, py_cmd)
        print("Running privilege escalation command: ")
        pprint(cmd)
        p = Popen(cmd, stdout=PIPE)
        def callback_wrap(callback):
            ret = p.wait()
            # if ret != 0:
            #     raise SudoException(f"Command returned failure status: {ret}")
            out = p.stdout.read()
            print("Output:")
            pprint(out)
            try:
                _, dill_str = out.split(sep)
            except:
                raise SudoException("Unable to retrieve output of function")
            exc, obj = dill.loads(dill_str)
            if exc:
                raise exc
            return callback(exc, obj)
        if callback:
            t = threading.Thread(target=callback_wrap, args=(callback[0],))
            t.start()
        else:
            return callback_wrap(lambda *_: _)

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

    @su.do_async("root")
    def super_greeting(text):
        import os, getpass
        with open(os.path.join(os.environ["HOME"], "greeting.txt"), "w") as wf:
            wf.write(text)
        return f"Hello non-{getpass.getuser()} world"

    @su.do("root")
    def esc(arg):
        print("Checking for a file")
        open("/tmp/does-not-exist.exists?")

    try:
        esc("WHY?")
    except FileNotFoundError as e:
        print(str(e))
    except SudoException as e:
        ## This might happen if the user clicks 'Cancel' on the popup.
        print("Unable to acquire root access")

    # print(super_greeting("Hello root world!"))
