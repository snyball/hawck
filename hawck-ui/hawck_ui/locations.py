import os

HAWCK_HOME = os.path.join(os.getenv("HOME"), ".local", "share", "hawck")

LOCATIONS = {
    "scripts": os.path.join(HAWCK_HOME, "scripts"),
    "scripts-enabled": os.path.join(HAWCK_HOME, "scripts-enabled"),
    "first_use": os.path.join(HAWCK_HOME, ".user_has_been_warned"),
    "hawck_share": "/usr/share/hawck",
    "hawck_bin": "/usr/share/hawck/bin",
    "hawck_keys": "/var/lib/hawck-input/keys",
    "unsafe_mode_src": "/usr/share/hawck/keys/__UNSAFE_MODE.csv",
}
LOCATIONS["unsafe_mode_dst"] = os.path.join(LOCATIONS["hawck_keys"], "__UNSAFE_MODE.csv")

def resourcePath(*args):
    return os.path.join(os.path.dirname(__file__), *args)
