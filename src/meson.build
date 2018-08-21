luadep = dependency('lua', version : '>=5.3', required : false)
if not luadep.found()
  luadep = dependency('lua5.3', version : '>=5.3.0')
endif

gtkdep = dependency('gtk+-3.0', version: '>=3.6')
dbusdep = dependency('dbus-1')
gobjectdep = dependency('gobject-2.0')
glibdep = dependency('glib-2.0')
pthreaddep = dependency('threads')
notifydep = dependency('libnotify')

macrod_src = [
  'RemoteUDevice.cpp',
  'Daemon.cpp',
  'MacroDaemon.cpp',
  'LuaUtils.cpp',
  'Keyboard.cpp',
  'FSWatcher.cpp',
  'hawck-macrod.cpp'
]
executable('hawck-macrod',
           macrod_src,
           dependencies : [luadep, gtkdep, dbusdep,
                           gobjectdep, glibdep, pthreaddep,
                           notifydep],
           install : true
          )

inputd_src = [
  'UDevice.cpp',
  'Daemon.cpp',
  'KBDDaemon.cpp',
  'Keyboard.cpp',
  'FSWatcher.cpp',
  'CSV.cpp',
  'hawck-inputd.cpp'
]
executable('hawck-inputd',
           inputd_src,
           dependencies : [pthreaddep],
           install : true
          )

executable('lsinput',
           'tools/lsinput.cpp',
           install : true
          )
