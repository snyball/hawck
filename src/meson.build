luadep = dependency('lua', version : '>=5.3', required : false)
if not luadep.found()
  luadep = dependency('lua5.3', version : '>=5.3')
endif

gtkdep = dependency('gtk+-3.0', version: '>=3.6')
dbusdep = dependency('dbus-1')
gobjectdep = dependency('gobject-2.0')
glibdep = dependency('glib-2.0')
pthreaddep = dependency('threads')
notifydep = dependency('libnotify')

conf_data = configuration_data()
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set_quoted('MACROD_VERSION', meson.project_version())
conf_data.set_quoted('INPUTD_VERSION', meson.project_version())
conf_data.set10('REDIRECT_STD_STREAMS', get_option('redirect_std'))
configure_file(output : 'hawck_config.h',
               configuration : conf_data
              )
conf_inc = include_directories('.')

macrod_src = [
  'hawck-macrod.cpp',
  'RemoteUDevice.cpp',
  'Daemon.cpp',
  'MacroDaemon.cpp',
  'LuaUtils.cpp',
  'Keyboard.cpp',
  'FSWatcher.cpp',
  'Permissions.cpp',
  'FIFOWatcher.cpp',
  'LuaConfig.cpp',
  'XDG.cpp',
]
executable('hawck-macrod',
           macrod_src,
           dependencies : [luadep, gtkdep, dbusdep,
                           gobjectdep, glibdep, pthreaddep,
                           notifydep],
           include_directories : conf_inc,
           install : true,
          )

inputd_src = [
  'hawck-inputd.cpp',
  'UDevice.cpp',
  'Daemon.cpp',
  'KBDDaemon.cpp',
  'Keyboard.cpp',
  'FSWatcher.cpp',
  'CSV.cpp',
  'Permissions.cpp',
  'LuaUtils.cpp',
]
executable('hawck-inputd',
           inputd_src,
           dependencies : [pthreaddep, luadep],
           include_directories : conf_inc,
           install : true,
          )

luatest_src = [
  'UDevice.cpp',
  'Keyboard.cpp',
  'FSWatcher.cpp',
  'CSV.cpp',
  'Permissions.cpp',
  'LuaUtils.cpp',
  'LuaTest.cpp',
]
executable('luatest',
           luatest_src,
           dependencies : [pthreaddep, luadep],
           include_directories : conf_inc,
           install : false)

