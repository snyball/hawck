import os

HAWCK_HOME = os.path.join(os.getenv("HOME"), ".local", "share", "hawck")

LOCATIONS = {
    "scripts": os.path.join(HAWCK_HOME, "scripts"),
    "scripts-enabled": os.path.join(HAWCK_HOME, "scripts-enabled"),
    "first_use": os.path.join(HAWCK_HOME, ".user_has_been_warned"),
    "hawck_bin": "/usr/share/hawck/bin"
}

def resourcePath(*args):
    return os.path.join(os.path.dirname(__file__), *args)
