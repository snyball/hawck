import struct
import os
import json
from functools import partial
from .os_open import OSOpen
from .privesc import getSudoMethod

su = getSudoMethod()

def sendcfg(path, opath, cfg):
    with OSOpen(path, os.O_WRONLY | os.O_NONBLOCK) as fd:
        fd.write(struct.pack("I", len(cfg)))
        fd.write(bytes(cfg, "utf-8"))
        print(f"Wrote: {cfg}")
    print(f"Waiting for response ...")
    with OSOpen(opath, os.O_RDONLY) as fd:
        (sz, ) = struct.unpack("I", fd.read(4))
        print("Got sz")
        json_str = fd.read(sz).decode("utf-8")
        print(f"json_str: {json_str}")
        return json.loads(json_str)

sendMacroD = partial(sendcfg,
                     os.path.expandvars("$HOME/.local/share/hawck/lua-comm.fifo"),
                     os.path.expandvars("$HOME/.local/share/hawck/json-comm.fifo"))

@su.do("hawck-input")
def sendInputD(cfg):
    from hawck_ui.cfgmsg import sendcfg
    return sendcfg(os.path.expandvars("/var/lib/hawck-input/lua-comm.fifo"),
                   os.path.expandvars("/var/lib/hawck-input/json-comm.fifo"),
                   cfg)
