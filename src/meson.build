project('Hawck', 'cpp', version : '0.4',
        default_options : ['c_std=c11',
                           'cpp_std=c++17',
                           '-O=s',
                          ]
       )

luadep = dependency('lua', version : '>=5.3')
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
                           notifydep]
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
           dependencies : [pthreaddep]
          )

tests_src = [
  'tests/CSV-tests/CSV-tests.cpp',
  'tests/FSWatcher-tests/FSWatcher-tests.cpp',
  'tests/tests-main.cpp',
  'FSWatcher.cpp',
  'CSV.cpp'
]
executable('hawck-tests',
           tests_src,
           dependencies : [pthreaddep]
          )

executable('lsinput', 'tools/lsinput.cpp')
