from hawck_ui.privesc import getSudoMethod, SudoException
su = getSudoMethod()

@su.do("hawck-input")
def copyKeys(src, script_name):
    import shutil, os
    dst = os.path.join("/", "var", "lib", "hawck-input", "keys", script_name + ".csv")
    shutil.copy(src, dst)
    os.chmod(dst, 0o644)

@su.do("hawck-input")
def killHawckInputD(sig):
    import os
    with open("/var/lib/hawck-input/pid") as pidf:
        pid = int(pidf.read().strip())
    if os.path.basename(os.readlink(f"/proc/{pid}/exe")) != "hawck-inputd":
        ## pid file is invalid
        return
    os.kill(pid, sig)
    ## Make sure the pid file disappears
    os.unlink("/var/lib/hawck-input/pid")

