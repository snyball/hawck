#!/bin/bash

gsettings set org.gtk.Settings.Debug enable-inspector-keybinding true 
GTK_DEBUG=interactive hawck-ui
