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

@su.do("hawck-input")
def setUnsafeMode(enabled):
    from hawck_ui.locations import LOCATIONS
    import shutil, os, grp
    if enabled:
        dst = LOCATIONS["unsafe_mode_dst"]
        shutil.copy(LOCATIONS["unsafe_mode_src"], dst)
        os.chown(dst, os.getuid(), grp.getgrnam("hawck-input-share").gr_gid)
        os.chmod(dst, 0o644)
    else:
        os.unlink(LOCATIONS["unsafe_mode_dst"])

@su.do("root")
def setInputDSystemDAutostart(enabled):
    from subprocess import Popen
    if enabled:
        Popen(["/usr/bin/systemctl", "enable", "hawck-inputd"]).wait()
    else:
        Popen(["/usr/bin/systemctl", "disable", "hawck-inputd"]).wait()

@su.do("root")
def startStopInputD(start):
    from subprocess import Popen
    if start:
        Popen(["/usr/bin/systemctl", "start", "hawck-inputd"]).wait()
    else:
        Popen(["/usr/bin/systemctl", "stop", "hawck-inputd"]).wait()

