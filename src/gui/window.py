## ================================================================================
## window.py
##
## Copyright (C) 2018 Jonas Møller (no) <jonasmo441@gmail.com>
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
## 
## 1. Redistributions of source code must retain the above copyright notice, this
##    list of conditions and the following disclaimer.
## 2. Redistributions in binary form must reproduce the above copyright notice,
##    this list of conditions and the following disclaimer in the documentation
##    and/or other materials provided with the distribution.
## 
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
## ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
## DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
## FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
## DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
## SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
## CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
## OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
## SOFTWARE.
## ================================================================================

import time
import threading
import os
import sys
import shutil
from subprocess import Popen, PIPE, STDOUT as STDOUT_REDIR
from pprint import PrettyPrinter

import gi
from gi.repository import Gtk
from gi.repository import GObject
gi.require_version('GtkSource', '4')
from gi.repository import GtkSource
# from gi_composites import GtkTemplate

pprint = PrettyPrinter(indent = 4).pprint

HAWCK_HOME = os.path.join(os.getenv("HOME"), ".local", "share", "hawck")

LOCATIONS = {
    "scripts": os.path.join(HAWCK_HOME, "scripts"),
    "scripts-enabled": os.path.join(HAWCK_HOME, "scripts-enabled"),
    "first_use": os.path.join(HAWCK_HOME, ".user_has_been_warned"),
    "hawck_bin": "/usr/share/hawck/bin"
}

SCRIPT_DEFAULT = """
require "init"

-- Sample mappings:
down => {
  ctrl  + alt + key "h" => say "Hello world"
  shift + alt + key "w" => app("firefox"):new_window("https://github.com/snyball/Hawck")
  ctrl  + alt + key "k" => function ()
    p = io.popen("fortune")
    say(p:read())()
    p:close()
  end
}
"""[1:]

class HawckInstallException(Exception):
    pass

class HawckMainWindow(Gtk.ApplicationWindow):
    __gtype_name__ = 'HawckMainWindow'

    def __init__(self, **kwargs):
        settings = Gtk.Settings.get_default()
        settings.set_property("gtk-application-prefer-dark-theme", True)
        super().__init__(**kwargs)
        self.internal_call = 0
        GObject.type_register(GtkSource.View)
        self.edit_pages = []
        self.init_template()
        self.builder = Gtk.Builder()
        self.builder.add_from_file('window.ui')
        self.window = self.builder.get_object('HawckMainWindow')
        script_dir = LOCATIONS["scripts"]
        for fname in os.listdir(script_dir):
            _, ext = os.path.splitext(fname)
            if ext == ".hwk":
                self.addEditPage(os.path.join(script_dir, fname))
        self.window.connect("destroy", lambda *_: sys.exit(0))
        self.window.present()
        self.builder.connect_signals(self)
        script_sw = self.builder.get_object("script_enabled_switch")
        self.script_switch_handler_id = script_sw.connect("state-set", self.setScriptEnabled)
        self.prepareEditForPage(0)
        self.checkHawckDRunning()

        ## Check for first use, issue warning if the program has not been launched before.
        if not os.path.exists(LOCATIONS["first_use"]):
            warning = self.builder.get_object("hawck_first_use_warning")
            warning.run()
            warning.hide()
            with open(LOCATIONS["first_use"], "w") as f:
                f.write("The user has been warned about potential risks of using the software.\n")

    def addEditPage(self, path: str):
        scrolled_window = Gtk.ScrolledWindow()
        src_view = GtkSource.View()
        src_view.set_show_line_numbers(True)
        src_view.set_highlight_current_line(True)
        src_view.set_auto_indent(True)
        src_view.set_monospace(True)
        src_view.set_vexpand(True)
        buf = src_view.get_buffer()
        src_lang_manage = GtkSource.LanguageManager()
        lua_lang = src_lang_manage.get_language("lua")
        scheme_manager = GtkSource.StyleSchemeManager()
        print(f"Schemes: {scheme_manager.get_scheme_ids()}")
        scheme = scheme_manager.get_scheme("oblivion")
        buf.set_language(lua_lang)
        buf.set_style_scheme(scheme)
        with open(path) as f:
            buf.set_text(f.read())
        name = os.path.basename(path)
        notebook = self.builder.get_object("edit_notebook")
        scrolled_window.add(src_view)
        notebook.append_page(scrolled_window, Gtk.Label(name))
        notebook.show_all()
        self.edit_pages.append(path)

    def onImportScriptOK(self, *_):
        file_chooser = self.builder.get_object("import_script_file_button")
        name_entry = self.builder.get_object("import_script_name_entry")
        name = name_entry.get_text()
        dst_path = os.path.join(LOCATIONS["scripts"], name + ".hwk")
        src_path = file_chooser.get_filename()
        shutil.copy(src_path, dst_path)
        self.addEditPage(dst_path)

    def prepareEditForPage(self, pagenr: int):
        print(f"Now editing: {self.edit_pages[pagenr]}")
        switch_obj = self.builder.get_object("script_enabled_switch")
        name = HawckMainWindow.getScriptName(self.edit_pages[pagenr])
        enabled_path = os.path.join(LOCATIONS["scripts-enabled"], name + ".lua")
        is_enabled = os.path.exists(enabled_path)
        with switch_obj.handler_block(self.script_switch_handler_id):
            switch_obj.set_state(is_enabled)
            switch_obj.set_active(is_enabled)
        # def fn():
        #     switch_obj.set_state(is_enabled)
        #     switch_obj.set_active(is_enabled)
        # self.internal(fn)

    def onEditChangePage(self, _notebook: Gtk.Notebook, _obj, pagenr: int):
        self.prepareEditForPage(pagenr)
    onEditSelectPage = onEditChangePage
    onEditSwitchPage = onEditChangePage

    def getCurrentEditFile(self):
        notebook = self.builder.get_object("edit_notebook")
        return self.edit_pages[notebook.get_current_page()]

    def onNewScriptOK(self, *_):
        popover = self.builder.get_object("new_script_popover")
        popover.popdown()
        name_entry = self.builder.get_object("new_script_name")
        path = os.path.join(LOCATIONS["scripts"], name_entry.get_text() + ".hwk")
        with open(path, "w") as f:
            f.write(SCRIPT_DEFAULT)
        self.addEditPage(path)

    def getCurrentBuffer(self):
        notebook = self.builder.get_object("edit_notebook")
        view = notebook.get_nth_page(notebook.get_current_page()).get_child()
        return view.get_buffer()

    def onTest(self, *_):
        print("Test")

    def saveCurrentScript(self):
        path = self.getCurrentEditFile()
        buf = self.getCurrentBuffer()
        start_iter = buf.get_start_iter()
        end_iter = buf.get_end_iter()
        text = buf.get_text(start_iter, end_iter, True)
        with open(path, "w") as wf:
            wf.write(text)

    def installScript(self, path: str):
        self.saveCurrentScript()
        p = Popen([os.path.join(LOCATIONS["hawck_bin"], "install_hwk_script.sh"), path], stdout=PIPE, stderr=STDOUT_REDIR)
        out = p.stdout.read()
        ret = p.wait()
        ## Handle error
        if ret != 0:
            lines = out.splitlines()
            _ = lines.pop()
            raise HawckInstallException("\n".join(l.decode("utf-8") for l in lines))
        ## TODO: elif "the script is enabled":
        ##          reinstall the script, so that it is reloaded by the daemon.

    @staticmethod
    def getScriptName(hwk_path):
        name, _ = os.path.splitext(os.path.basename(hwk_path))
        return name

    def getCurrentScriptName(self):
        hwk_path = self.getCurrentEditFile()
        name, _ = os.path.splitext(os.path.basename(hwk_path))
        return name

    def onPopdown(self, p):
        p.popdown()

    def useScript(self, *_):
        current_file = self.getCurrentEditFile()
        buf = self.builder.get_object("script_error_buffer")
        try:
            self.installScript(current_file)
        except HawckInstallException as e:
            ## TODO: Display the error properly
            ## TODO: Parse Lua errors to get the line number of the error, then highlight this
            ##       in the text editor margin.
            popover = self.builder.get_object("use_script_error")
            popover.popup()
            buf.set_text(str(e))
            return
        buf.set_text("OK")
        popover = self.builder.get_object("use_script_success")
        popover.popup()
        HawckMainWindow.enableScript(self.getCurrentScriptName())

    @staticmethod
    def enableScript(name: str):
        HawckMainWindow.disableScript(name)
        name += ".lua"
        os.link(os.path.join(LOCATIONS["scripts"], name),
                os.path.join(LOCATIONS["scripts-enabled"], name))

    @staticmethod
    def disableScript(name: str) -> None:
        try:
            os.unlink(os.path.join(LOCATIONS["scripts-enabled"], name + ".lua"))
        except Exception:
            pass

    def internal(self, *args):
        """
        This function is mainly to avoid handlers like setScriptEnabled
        being executed when the state of switches are set by the script.
        """
        if args:
            fn, *_ = args
            self.internal_call += 1
            fn()
        else:
            was = self.internal_call
            self.internal_call -= 1
            if self.internal_call < 0:
                self.internal_call = 0
            return was

    def setScriptEnabled(self, switch_obj: Gtk.Switch, enabled: bool):
        if self.internal(): return

        hwk_path = self.getCurrentEditFile()
        name = self.getCurrentScriptName()

        if not enabled:
            return HawckMainWindow.disableScript(name)

        buf = self.builder.get_object("script_error_buffer")
        try:
            print("Installing script ...")
            self.installScript(hwk_path)
            print("Success!")
        except HawckInstallException as e:
            print("EXCEPTION")
            # self.internal_call += 2
            # with switch_obj.handler_block(self.script_switch_handler_id):
            popover = self.builder.get_object("use_script_error")
            popover.popup()
            buf.set_text(str(e))
            switch_obj.set_active(False)
            switch_obj.set_state(False)
            return True

        buf.set_text("OK")

        HawckMainWindow.enableScript(name)

        return None

    def deleteScript(self, *_):
        popover = self.builder.get_object("delete_script_popover")
        popover.popdown()
        path = self.getCurrentEditFile()
        notebook = self.builder.get_object("edit_notebook")
        page_num = notebook.get_current_page()
        notebook.remove_page(page_num)
        pg = self.edit_pages
        self.edit_pages = pg[:page_num] + pg[page_num+1:]
        os.remove(path)

    def onClickAboutBtn(self, *_):
        # self.window.show_about_dialog()
        about_dialog = self.builder.get_object("hawck_about_dialog")
        about_dialog.run()
        about_dialog.hide()

    def saveScript(self, *_):
        sav_dialog = Gtk.FileChooserDialog("Save as", self,
                                           Gtk.FileChooserAction.SAVE,
                                           (Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL,
                                            Gtk.STOCK_SAVE, Gtk.ResponseType.OK))
        sav_dialog.set_transient_for(self.window)
        sav_dialog.set_do_overwrite_confirmation(True)
        sav_dialog.set_modal(True)
        sav_dialog.run()
        path = sav_dialog.get_filename()
        if path:
            shutil.copy(self.getCurrentEditFile(), path)
        sav_dialog.destroy()

    def checkHawckDRunning(self):
        inputd_sw = self.builder.get_object("inputd_switch")
        macrod_sw = self.builder.get_object("macrod_switch")
        pgrep_loc = "/usr/bin/pgrep"

        ret = Popen([pgrep_loc, "hawck-inputd"]).wait()
        inputd_sw.set_state(not ret)
        inputd_sw.set_active(not ret)

        ret = Popen([pgrep_loc, "hawck-macrod"]).wait()
        macrod_sw.set_state(not ret)
        macrod_sw.set_active(not ret)

    def onPanicBtn(self, *_):
        p = Popen([os.path.join(LOCATIONS["hawck_bin"], "kill9_hawck.sh")])
        p.wait()
        self.checkHawckDRunning()
