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

import gi
from gi.repository import Gtk
from gi.repository import GObject
gi.require_version('GtkSource', '4')
from gi.repository import GtkSource
# from gi_composites import GtkTemplate

#@GtkTemplate(ui='/org/gnome/Hawck-Ui/window.ui')
# @GtkTemplate(ui='window.ui')
class HawckMainWindow(Gtk.ApplicationWindow):
    __gtype_name__ = 'HawckMainWindow'

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        GObject.type_register(GtkSource.View)
        self.init_template()
        self.builder = Gtk.Builder()
        self.builder.add_from_file('window.ui')
        self.window = self.builder.get_object('HawckMainWindow')
        self.window.present()
        self.builder.connect_signals(self)

    def useScript(self, *args):
        print("CLICKED CLICKER")
