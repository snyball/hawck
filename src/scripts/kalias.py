#!/usr/bin/python3

import threading
import sys
import shutil
import os
import time
from subprocess import Popen, PIPE, STDOUT as STDOUT_REDIR
from pprint import PrettyPrinter
pprint = PrettyPrinter(indent = 4).pprint

import gi
gi.require_version('Gtk', '3.0')

from gi.repository import Gtk, Gio
from gi.repository import Gdk
from gi.repository import GObject

HAWCK_HOME = os.path.join(os.getenv("HOME"), ".local", "share", "hawck")

class KAliasWindow(Gtk.ApplicationWindow):
    __gtype_name__ = 'KAliasWindow'

    def __init__(self, **kwargs):
        settings = Gtk.Settings.get_default()
        settings.set_property("gtk-application-prefer-dark-theme", True)
        super().__init__(**kwargs)
        self.builder = Gtk.Builder()
        self.builder.add_from_file('./kalias_window.glade')
        self.window = self.builder.get_object("kalias_window")
        self.window.connect("destroy", lambda *_: sys.exit(0))
        self.window.present()
        self.builder.connect_signals(self)
        self.keys = []
        # def monitorFifo():
        #     os.remove("/tmp/kalias.fifo")
        #     os.mkfifo("/tmp/kalias.fifo")
        #     while True:
        #         with open("/tmp/kalias.fifo") as f:
        #             l = f.readline()
        #             if l.strip() == "Done":
        #                 break
        #     os.remove("/tmp/kalias.fifo")
        #     self.done()
        # t = threading.Thread(target=monitorFifo)
        # t.daemon = True
        # t.start()
        self.sendScript()

    def sendScript(self):
        path = os.path.join(HAWCK_HOME, "scripts-enabled", "get_aliases.lua")
        try:
            os.remove(path)
        except Exception:
            pass
        os.chmod("./get_aliases.lua", 0o744)
        shutil.copy("./get_aliases.lua", path)

    def onRedo(self, btn):
        return
        entry = self.builder.get_object("text_entry")
        entry.get_buffer().set_text("")
        self.keys = []
        self.sendScript()

    def onContinue(self, btn):
        self.done()

    def onKeyPress(self, window, ev):
        oname = ev.string.strip()
        ev_name = Gdk.keyval_name(ev.keyval)
        if any(ev_name.lower().startswith(s) for s in ("control", "shift", "alt", "iso_")):
            return
        print(f"name: {ev_name}")
        print(f"ev.string.strip(): {oname}")
        print(f"Gdk.keyval_name(ev.keyval): {Gdk.keyval_name(ev.keyval)}")
        uni_name = chr(Gdk.keyval_to_unicode(ev.keyval))
        print(f"Gdk.keyval_to_unicode(ev.keyval): {uni_name}")
        print("")
        if uni_name != '\x00':
            self.keys.append(uni_name)
            buf = self.builder.get_object("text_entry").get_buffer()
            buf.insert_at_cursor(uni_name + "\n")

    def done(self):
        with open("/tmp/keys.out", "w") as f:
            f.write("\n".join(self.keys))
            f.write("\n")
        with open("/tmp/aliases.txt") as f:
            aliases = f.readlines()

        pprint(self.keys)
        pprint(aliases)

        if len(self.keys) != len(aliases):
            print("Unable to join keys with aliases")
            print(f"len(self.keys) = {len(self.keys)}; len(aliases) = {len(aliases)}")
            return

class Application(Gtk.Application):
    def __init__(self):
        super().__init__(application_id='org.gnome.Hawck-Ui',
                         flags=Gio.ApplicationFlags.FLAGS_NONE)

    def do_activate(self):
        win = KAliasWindow(application=self)

def main():
    app = Application()
    app.run(sys.argv)

if __name__ == "__main__":
    main()
