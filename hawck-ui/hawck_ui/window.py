## ================================================================================
## window.py is a part of hawck-ui, which is distributed under the
## following license:
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
import signal
import errno
import inspect
import pkg_resources as pkg
from subprocess import Popen, PIPE, STDOUT as STDOUT_REDIR
from pprint import PrettyPrinter
from functools import wraps

import gi
from gi.repository import Gtk
from gi.repository import Gdk
from gi.repository import GObject
from gi.repository import GLib
gi.require_version('GtkSource', '3.0')
from gi.repository import GtkSource

from . import priv_actions
from .cfgmsg import sendcfg
from .template_manager import TemplateManager
from .log_retriever import LogRetriever
from .locations import HAWCK_HOME, LOCATIONS, resourcePath
from .privesc import SudoException

pprint = PrettyPrinter(indent = 4).pprint

SCRIPT_DEFAULT = """
require "init"
"""[1:]

MODIFIER_NAMES = {
    "Alt",
    "Alt_L",
    "Alt_R",
    "Control",
    "Control_L",
    "Control_R",
    "Shift",
    "Shift_L",
    "Shift_R",
    "Shift",
    "Shift_L",
    "Shift_R",
    "AltGr",
}

class HawckInstallException(Exception):
    pass

class MainWindow(GObject.GObject):
    def __init__(self, **kwargs):
        super().__init__()
        self.builder = Gtk.Builder()
        rs = pkg.resource_string("hawck_ui", kwargs["ui"])
        self.builder.add_from_string(rs.decode("utf-8"))
        self.window = self.builder.get_object("MainWindow")
        self.window.set_application(kwargs["application"])
        self.window.connect("destroy", lambda *_: sys.exit(0))
        self.window.present()

    def connectAll(self):
        self.builder.connect_signals(self)

    def get(self, obj_name: str):
        return self.builder.get_object(obj_name)

def handle(obj_id, fn):
    def sub(fn):
        fn.__for_object__ = obj_id
        return fn
    return sub

class Pluggable:
    def __init__(self, main=None, builder=None):
        self.builder = builder
        self.main = main

    def get(self, obj_name: str):
        return self.builder.get_object(obj_name)

    def setSwitch(self, obj_name: str, state: bool):
        print(self, obj_name, state)
        sw = self.get(obj_name)
        sw.set_state(state)
        sw.set_active(state)
        return state

    def setCheck(self, obj_name: str, state: bool):
        chk = self.get(obj_name)
        chk.set_active(state)
        return state

    def getBindings(self):
        methods = inspect.getmembers(self, predicate=lambda s: inspect.ismethod(s))
        self.methods = {}
        self.methods.update(methods)
        return self.methods

    def block(self, obj, fn):
        obj.handler_block_by_func(self.methods[fn])

    def unblock(self, obj, fn):
        obj.handler_unblock_by_func(self.methods[fn])

def switchHandler(fn):
    @wraps(fn)
    def sub(self, switch, on):
        try:
            return fn(self, on)
        except Exception as e:
            print(e)
            self.block(switch, fn.__name__)
            switch.set_active(not on)
            switch.set_state(not on)
            self.unblock(switch, fn.__name__)
            return True
    return sub

def checkHandler(fn):
    @wraps(fn)
    def sub(self, chk):
        on = chk.get_active()
        try:
            return fn(self, on)
        except Exception as e:
            print(e)
            self.block(chk, fn.__name__)
            chk.set_active(not on)
            self.unblock(chk, fn.__name__)
            return True
    return sub


class Settings(Pluggable):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        autostart_path = os.path.join(os.getenv("HOME"), ".config", "autostart", "hawck-macrod.desktop")
        self.autostart = self.setSwitch("autostart_switch", os.path.exists(autostart_path))
        self.unsafe_mode = self.setSwitch("unsafe_mode_switch", os.path.exists(LOCATIONS["unsafe_mode_dst"]))
        self.setUnsafeModeText(self.unsafe_mode)
        self.cfg_path = os.path.expandvars("$HOME/.local/share/hawck/lua-comm.fifo")

    @switchHandler
    def on_set_autostart(self, on):
        dst = os.path.join(os.getenv("HOME"), ".config", "autostart", "hawck-macrod.desktop")
        dst_dir = os.path.dirname(dst)
        ## Make sure autostart directory exists
        if not os.path.exists(dst_dir):
            try:
                os.makedirs(dst_dir)
            except OSError as e:
                if e.errno != errno.EEXISTS:
                    raise
        src = os.path.join(LOCATIONS["hawck_bin"], "hawck-macrod.desktop")
        priv_actions.setInputDSystemDAutostart(on)
        if on:
            shutil.copy(src, dst)
        else:
            os.unlink(dst)
        self.autostart = on

    def setUnsafeModeText(self, on):
        label = self.get("unsafe_mode_state_label")
        disabled_mk = "<tt><span fgcolor=\"#4CB940\" font_weight=\"bold\">Disabled</span></tt>" 
        enabled_mk = "<tt><span fgcolor=\"#BF4040\" font_weight=\"bold\">Enabled</span></tt>"
        if on:
            label.set_markup(enabled_mk)
        else:
            label.set_markup(disabled_mk)

    @switchHandler
    def on_set_unsafe_mode(self, on):
        priv_actions.setUnsafeMode(on)
        self.setUnsafeModeText(on)

    @checkHandler
    def on_error_stop_chk(self, on):
        sendcfg(self.cfg_path, f"config.stop_on_err = {str(on).lower()}")

    @checkHandler
    def on_error_notify_chk(self, on):
        sendcfg(self.cfg_path, f"config.notify_on_err = {str(on).lower()}")

class HawckMainWindow(MainWindow):
    __gtype_name__ = 'HawckMainWindow'

    def __init__(self, **kwargs):
        settings = Gtk.Settings.get_default()
        settings.set_property("gtk-application-prefer-dark-theme", True)

        ## Get version
        self.version = kwargs["version"]
        del kwargs["version"]

        super().__init__(
            ui="resources/glade-xml/window.ui",
            **kwargs
        )

        self.window.set_icon_name("hawck")
        self.window.set_default_icon_name("hawck")

        GObject.type_register(GtkSource.View)
        self.edit_pages = []
        self.scripts = {}

        script_dir = LOCATIONS["scripts"]
        self.src_lang_manager = GtkSource.LanguageManager()
        self.scheme_manager = GtkSource.StyleSchemeManager()
        for fname in os.listdir(script_dir):
            _, ext = os.path.splitext(fname)
            if ext == ".hwk":
                self.addEditPage(os.path.join(script_dir, fname))
        self.insert_key_handler_id = self.connect("onKeyCaptureDone", self.insertKeyHandler)
        self.handler_block(self.insert_key_handler_id)
        script_sw = self.builder.get_object("script_enabled_switch")
        self.script_switch_handler_id = script_sw.connect("state-set", self.setScriptEnabled)
        self.prepareEditForPage(0)
        self.checkHawckDRunning()

        ## Key capture stuff
        ## TODO: Separate the key capturing into its own class
        self.keycap_names = []
        self.keycap_codes = []
        self.keycap_done = False

        self.templates = TemplateManager("resources/glade-xml/")
        self.templates.load("error_log.ui")
        self.logs = LogRetriever(self.updateLogs)
        self.logs.daemon = True
        GObject.threads_init()
        self.logs.start()
        self.log_rows = []
        # self.updateLogs()

        hawck_status_version = self.builder.get_object("hawck_status_version")
        hawck_status_version.set_text(f"Hawck v{self.version}")
        hawck_about = self.builder.get_object("hawck_about_dialog")
        hawck_about.set_version(self.version)

        notebook = self.builder.get_object("edit_notebook")
        notebook.set_current_page(0)

        self.settings = Settings(builder = self.builder)
        signals = {}
        for obj in (self.settings, self):
            methods = obj.getBindings()
            signals.update(methods)
        self.builder.connect_signals(signals)

        ## Check for first use, issue warning if the program has not been launched before.
        if not os.path.exists(LOCATIONS["first_use"]):
            warning = self.builder.get_object("hawck_first_use_warning")
            warning.run()
            warning.hide()
            with open(LOCATIONS["first_use"], "w") as f:
                f.write("The user has been warned about potential risks of using the software.\n")

    def getBindings(self):
        self.methods = inspect.getmembers(self, predicate=lambda s: inspect.ismethod(s))
        return self.methods

    def initSettings(self):
        """
        Set settings
        """
        autostart_path = os.path.join(os.getenv("HOME"), ".config", "autostart", "hawck-macrod.desktop")
        auto_enabled = os.path.exists(autostart_path)
        auto_sw = self.builder.get_object("autostart_switch")
        auto_sw.handler_block_by_func(self.onSetAutostart)
        auto_sw.set_state(auto_enabled)
        auto_sw.set_active(auto_enabled)
        auto_sw.handler_unblock_by_func(self.onSetAutostart)

    def updateLogs(self, added):
        loglist = self.builder.get_object("script_error_list")

        Gdk.threads_enter()

        for row in self.log_rows:
            loglist.remove(row)
        self.log_rows = []

        for log in (l for l in added if l["TYPE"] == "LUA"):
            ## Create new row and prepend it
            row, builder = self.templates.get("error_log.ui")
            buf = builder.get_object("error_script_buffer")
            buf.set_text(log["LUA_ERROR"])
            label = builder.get_object("error_script_name")
            label.set_text(os.path.basename(log["LUA_FILE"]))
            num_dup_label = builder.get_object("num_duplicates")
            num_dup_label.set_text(str(log.get("DUP", 1)))
            def openScript(*_):
                edit_pg = self.builder.get_object("edit_script_box")
                stack = self.builder.get_object("main_stack")
                edit_notebook = self.builder.get_object("edit_notebook")
                sname = HawckMainWindow.getScriptName(log["LUA_FILE"])
                script = self.scripts[sname]
                pagenr = script["pagenr"]
                view = script["view"]
                buf = script["buffer"]
                edit_notebook.set_current_page(pagenr)
                stack.set_visible_child(edit_pg)
                text_iter = buf.get_start_iter()
                text_iter.set_line(log["LUA_LINE"])
                # mark = Gtk.TextMark()
                # buf.add_mark(mark, text_iter)
                view.scroll_to_iter(text_iter, 0, True, 0.0, 0.17)
                errbuf = self.builder.get_object("script_error_buffer")
                errbuf.set_text(f"{sname}:{log['LUA_LINE']}: {log['LUA_ERROR']}")
            def dismissError(*_):
                err = log["MESSAGE"]
                self.logs.dismiss(err)
            open_btn = builder.get_object("error_script_btn_open")
            open_btn.connect("clicked", openScript)
            dismiss_btn = builder.get_object("error_script_btn_dismiss")
            dismiss_btn.connect("clicked", dismissError)
            loglist.prepend(row)
            loglist.show_all()
            self.log_rows.append(row)
            row.show_all()

        Gdk.threads_leave()

        return False

    def onClickUpdateLogs(self, *_):
        ret = self.logs.update()
        if ret:
            self.updateLogs(*ret)

    ## TODO: Write this
    def onToggleAutoUpdateLog(self, btn):
        active = btn.get_active()

    def addEditPage(self, path: str):
        scrolled_window = Gtk.ScrolledWindow()
        src_view = GtkSource.View()
        src_view.set_show_line_numbers(True)
        src_view.set_highlight_current_line(True)
        src_view.set_auto_indent(True)
        src_view.set_monospace(True)
        src_view.set_vexpand(True)
        buf = src_view.get_buffer()
        lua_lang = self.src_lang_manager.get_language("lua")
        # print(f"Schemes: {self.scheme_manager.get_scheme_ids()}")
        scheme = self.scheme_manager.get_scheme("oblivion")
        buf.set_language(lua_lang)
        buf.set_style_scheme(scheme)
        with open(path) as f:
            buf.set_text(f.read())
        name = os.path.basename(path)
        notebook = self.builder.get_object("edit_notebook")
        scrolled_window.add(src_view)
        notebook.append_page(scrolled_window, Gtk.Label(name))
        notebook.show_all()
        pagenr = len(self.edit_pages)
        notebook.set_current_page(pagenr)
        self.edit_pages.append(path)
        sname = HawckMainWindow.getScriptName(path)
        self.scripts[sname] = {
            "pagenr": pagenr,
            "buffer": buf,
            "view": src_view
        }

    def onImportScriptOK(self, *_):
        file_chooser = self.builder.get_object("import_script_file_button")
        name_entry = self.builder.get_object("import_script_name_entry")
        name = name_entry.get_text()
        dst_path = os.path.join(LOCATIONS["scripts"], name + ".hwk")
        src_path = file_chooser.get_filename()
        shutil.copy(src_path, dst_path)
        self.addEditPage(dst_path)

    def prepareEditForPage(self, pagenr: int):
        if pagenr >= len(self.edit_pages):
            return

        switch_obj = self.builder.get_object("script_enabled_switch")
        name = HawckMainWindow.getScriptName(self.edit_pages[pagenr])
        enabled_path = os.path.join(LOCATIONS["scripts-enabled"], name + ".lua")
        is_enabled = os.path.exists(enabled_path)
        with switch_obj.handler_block(self.script_switch_handler_id):
            switch_obj.set_state(is_enabled)
            switch_obj.set_active(is_enabled)

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
        p = Popen(
            [
                os.path.join(LOCATIONS["hawck_bin"],
                             "install-hwk-script.sh"),
                path
            ], stdout=PIPE, stderr=PIPE)
        out = p.stdout.read()
        ret = p.wait()

        ## Need to install the new keys required by the script
        if ret == 123:
            ## If unsafe mode is enabled we don't need to install any keys.
            if not self.settings.unsafe_mode:
                try:
                    priv_actions.copyKeys(out.strip(), self.getCurrentScriptName())
                except SudoException as e:
                    print(f"Unable to copy keys: {e}")
        ## Handle error
        elif ret != 0:
            lines = out.splitlines()
            _ = lines.pop()
            raise HawckInstallException("\n".join(l.decode("utf-8") for l in lines))

    @staticmethod
    def getScriptName(hwk_path):
        name, _ = os.path.splitext(os.path.basename(hwk_path))
        return name

    def getCurrentScriptName(self):
        hwk_path = self.getCurrentEditFile()
        name, _ = os.path.splitext(os.path.basename(hwk_path))
        return name

    def onPopdown(self, p, *_):
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

    def setScriptEnabled(self, switch_obj: Gtk.Switch, enabled: bool):
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
        name = self.getCurrentScriptName()
        HawckMainWindow.disableScript(name)
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

    def insertKeyHandler(self, window, names, codes):
        self.handler_block(self.insert_key_handler_id)
        print(f"Captured: {names}")
        buf = self.getCurrentBuffer()
        text = "down + "
        text += " + ".join(n.lower() for n in names[:-1])
        text += f" + key \"{names[-1].lower()}\" => "
        buf.insert_at_cursor(text)

    def onInsertKey(self, *_):
        self.handler_unblock(self.insert_key_handler_id)
        self.captureKey()

    def checkHawckDRunning(self):
        inputd_label = self.builder.get_object("inputd_status")
        macrod_label = self.builder.get_object("macrod_status")
        pgrep_loc = "/usr/bin/pgrep"

        run_str = "<tt><span fgcolor=\"#4CB940\" font_weight=\"bold\">Running</span></tt>" 
        stop_str = "<tt><span fgcolor=\"#BF4040\" font_weight=\"bold\">Stopped</span></tt>"

        ret = Popen([pgrep_loc, "hawck-inputd"]).wait()
        inputd_label.set_markup(run_str if not ret else stop_str)

        ret = Popen([pgrep_loc, "hawck-macrod"]).wait()
        macrod_label.set_markup(run_str if not ret else stop_str)

    def onNewFocus(self, *_):
        stack = self.builder.get_object("main_stack")
        print(f"Focus change: {stack.get_visible_child_name()}")

    def onPanicBtn(self, *_):
        Popen(["killall", "-9", "hawck-macrod"]).wait()
        priv_actions.killHawckInputD(signal.SIGKILL)
        self.checkHawckDRunning()

    def onKeyCaptureCancel(self, *_):
        win = self.builder.get_object("key_capture_window")
        win.hide()

    def onKeyCaptureOK(self, *_):
        win = self.builder.get_object("key_capture_window")
        win.hide()
        names = self.keycap_names
        codes = self.keycap_codes
        self.keycap_names = []
        self.keycap_codes = []
        self.emit("onKeyCaptureDone", names, codes)

    def onKeyCaptureKeyRelease(self, window, ev):
        if self.keycap_done:
            return

        oname = ev.string.strip()
        ev_name = oname or Gdk.keyval_name(ev.keyval)
        is_modifier = ev_name in MODIFIER_NAMES
        if not is_modifier and len(ev_name) == 1:
            ev_name = ev_name.upper()
        try:
            idx = self.keycap_names.index(ev_name)
            self.keycap_codes.remove(self.keycap_codes[idx])
            self.keycap_names.remove(ev_name)
        except ValueError:
            return
        self.setKeyCaptureLabel(self.keycap_names)

    def onKeyCaptureReset(self, *_):
        self.keycap_names = []
        self.keycap_codes = []
        self.keycap_done = False
        self.setKeyCaptureLabel([])

    def onKeyCaptureKeyPress(self, window, ev):
        if self.keycap_done:
            return

        oname = ev.string.strip()
        ev_name = oname or Gdk.keyval_name(ev.keyval)

        # print(f"name: {ev_name}")
        # print(f"ev.keycode: {ev.keyval}")
        # print(f"ev.hardware_keycode: {ev.hardware_keycode}")
        # print(f"ev.string.strip(): {oname}")
        # print(f"Gdk.keyval_name(ev.keyval): {Gdk.keyval_name(ev.keyval)}")
        # print(f"Gdk.keyval_to_unicode(ev.keyval): {Gdk.keyval_to_unicode(ev.keyval)}")
        # print("")

        is_modifier = ev_name in MODIFIER_NAMES

        if not is_modifier and len(ev_name) == 1:
            ev_name = ev_name.upper()

        ## Repeat key
        if self.keycap_names and self.keycap_names[-1] == ev_name:
            return

        self.keycap_names.append(ev_name)
        self.keycap_codes.append(ev.hardware_keycode)
        self.setKeyCaptureLabel(self.keycap_names)

        ## Check if we received a terminal key:
        if not is_modifier:
            self.keycap_done = True

    def setKeyCaptureLabel(self, names):
        fmt = " - ".join(names)
        label = self.builder.get_object("key_capture_display")
        label.set_text(fmt)

    @GObject.Signal(flags=GObject.SignalFlags.RUN_LAST,
                    arg_types=(object, object),
                    return_type=bool,
                    accumulator=GObject.signal_accumulator_true_handled)
    def onKeyCaptureDone(self, *_):
        print(f"Keycap done: {_}")
        self.onKeyCaptureReset()

    def captureKey(self):
        win = self.builder.get_object("key_capture_window")
        win.show_all()

    def onSearchKeyboardUpdate(self, *_):
        pass ## Not implemented
