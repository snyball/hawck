#!/usr/bin/python3

##
## Written by Jonas Møller, part of Hawck.
## This file is in the public domain
##

"""
This is a fallback sudo method for stubborn clients who
don't have PolicyKit or a graphical version of sudo
configured and installed.

I can't be bothered to automatically figure out the
arguments that need to be passed to the users
terminal emulator, so I use Gtk's vte terminal instead.

Can either be run standalone with the following syntax:
    sudo_fallback.py <user> <program> <args>...
This also works:
    sudo_fallback.py <user> -- <program> <args>...

Or can be safely imported and used like this:
    from sudo_fallback import SudoFallbackWindow
    sudo = SudoFallbackWindow()
    sudo.run(user, program, args...)
"""

##
## FIXME: spawn_sync is deprecated, but spawn_async is not exposed to Python.
##        Stuck between a rock and a hard place on this one.
##

UI_XML = """
<?xml version="1.0" encoding="UTF-8"?>
<!-- Generated with glade 3.22.1 -->
<interface>
  <requires lib="gtk+" version="3.0"/>
  <requires lib="vte-2.91" version="2.91"/>
  <object class="GtkWindow" id="VtMainWindow">
    <property name="can_focus">False</property>
    <property name="title" translatable="yes">Graphical sudo fallback</property>
    <property name="modal">True</property>
    <child type="titlebar">
      <placeholder/>
    </child>
    <child>
      <object class="VteTerminal" id="terminal">
        <property name="visible">True</property>
        <property name="can_focus">True</property>
        <property name="hscroll_policy">natural</property>
        <property name="vscroll_policy">natural</property>
        <property name="allow_hyperlink">True</property>
        <property name="encoding">UTF-8</property>
        <property name="scroll_on_keystroke">True</property>
      </object>
    </child>
  </object>
</interface>
"""

import gi
gi.require_version("Vte", "2.91")
gi.require_version("Gtk", "3.0")
from gi.repository import Gtk, GObject, Vte, GLib
import os
import sys

class SudoFallbackWindow(Gtk.Window):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        GObject.type_register(Vte.Terminal)
        self.builder = Gtk.Builder()
        self.builder.add_from_string(UI_XML)
        self.window = self.builder.get_object("VtMainWindow")
        self.term = self.builder.get_object("terminal")

    def run(self, user, *args):
        self.window.show_all()
        succ, pid = self.term.spawn_sync(
            Vte.PtyFlags.DEFAULT,
            "/",
            ["/sbin/sudo", "-u", user, *args],
            [],
            GLib.SpawnFlags.DO_NOT_REAP_CHILD,
            None,
            None,
        )
        self.term.watch_child(pid)
        def exitSig(status, *_):
            if __name__ == "__main__":
                ## If we were launched as a standalone script, exit
                Gtk.main_quit(status)
                sys.exit(status)
            else:
                self.window.hide()
        self.term.connect("child-exited", exitSig)

if __name__ == "__main__":
    win = SudoFallbackWindow()
    args = sys.argv[2:]
    if args[0] == "--":
        args = args[1:]
    win.run(sys.argv[1], *args)
    Gtk.main()
