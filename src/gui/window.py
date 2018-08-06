# window.py
#
# Copyright 2018 Jonas Møller
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import gi
from gi.repository import Gtk
from gi.repository import GObject
gi.require_version('GtkSource', '4')
from gi.repository import GtkSource
from gi_composites import GtkTemplate

#@GtkTemplate(ui='/org/gnome/Hawck-Ui/window.ui')
# @GtkTemplate(ui='window.ui')
class HawckMainWindow(Gtk.ApplicationWindow):
    __gtype_name__ = 'HawckMainWindow'

    label = GtkTemplate.Child()

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        GObject.type_register(GtkSource.View)
        self.init_template()
        self.builder = Gtk.Builder()
        self.builder.add_from_file('window.ui')
        self.window = self.builder.get_object('HawckMainWindow')
        self.window.present()
        self.builder.connect_signals(self)
