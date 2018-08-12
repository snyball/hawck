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

import os
import sys
import shutil
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
    "first_use": os.path.join(HAWCK_HOME, ".user_has_been_warned"),
}

SCRIPT_DEFAULT = """
require "init"

-- Sample mappings:
ctrl  + alt + key "h" => say "Hello world"
shift + alt + key "w" => app("firefox"):new_window("https://github.com/snyball/Hawck")
ctrl  + alt + key "k" => function ()
  p = io.popen("fortune")
  say(p:read())()
  p:close()
end
"""[1:]

class HawckMainWindow(Gtk.ApplicationWindow):
    __gtype_name__ = 'HawckMainWindow'

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        GObject.type_register(GtkSource.View)
        self.edit_pages = []
        self.init_template()
        self.builder = Gtk.Builder()
        self.builder.add_from_file('window.ui')
        self.window = self.builder.get_object('HawckMainWindow')
        script_dir = LOCATIONS["scripts"]
        for fname in os.listdir(script_dir):
            self.addEditPage(os.path.join(script_dir, fname))
        self.window.connect("destroy", lambda *_: sys.exit(0))
        self.window.present()
        self.builder.connect_signals(self)

        ## Check for first use, issue warning if the program has not been launched before.
        if not os.path.exists(LOCATIONS["first_use"]):
            warning = self.builder.get_object("hawck_first_use_warning")
            warning.run()
            warning.hide()
            with open(LOCATIONS["first_use"], "w") as f:
                f.write("The user has been warned about potential risks of using the software.\n")

    def addEditPage(self, path: str):
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
        scheme = scheme_manager.get_scheme("oblivion")
        buf.set_language(lua_lang)
        buf.set_style_scheme(scheme)
        with open(path) as f:
            buf.set_text(f.read())
        name = os.path.basename(path)
        notebook = self.builder.get_object("edit_notebook")
        notebook.append_page(src_view, Gtk.Label(name))
        notebook.show_all()
        self.edit_pages.append(path)

    def onImportScriptOK(self, *args):
        file_chooser = self.builder.get_object("import_script_file_button")
        name_entry = self.builder.get_object("import_script_name_entry")
        name = name_entry.get_text()
        dst_path = os.path.join(LOCATIONS["scripts"], name + ".hwk")
        src_path = file_chooser.get_filename()
        shutil.copy(src_path, dst_path)
        self.addEditPage(dst_path)

    def prepareEditForPage(self, pagenr):
        print(f"Now editing: {self.edit_pages[pagenr]}")

    def onEditChangePage(self, notebook, obj, pagenr):
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

    def useScript(self, *_):
        print("USE")
        print(self.getCurrentEditFile())
        # Gtk.main_quit()

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
