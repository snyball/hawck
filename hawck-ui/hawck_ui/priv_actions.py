from hawck_ui.privesc import getSudoMethod, SudoException
su = getSudoMethod()

@su.do("hawck-input")
def copyKeys(src, script_name):
    import shutil, os
    dst = os.path.join("/", "var", "lib", "hawck-input", "keys", script_name + ".csv")
    shutil.copy(src, dst)
    os.chmod(dst, 0o644)
