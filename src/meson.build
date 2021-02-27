lua_version = ['>=5.3', '<5.4']
## Waiter, I would like a Coke
luadep = dependency('lua', version : lua_version, required : false)
if not luadep.found()
  ## Ok, then I would like a Pepsi
  luadep = dependency('lua5.3', version : lua_version, required : false)
endif
if not luadep.found()
  ## Ok, then I would like a Pepsi Max
  luadep = dependency('lua-5.3', version : lua_version, required : true)
endif
## Thank you

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
  'KBDB.cpp',
  'Popen.cpp',
]
executable('hawck-macrod',
           macrod_src,
           dependencies : [luadep, pthreaddep, notifydep],
           include_directories : conf_inc,
           install : true,
          )

inputd_src = [
  'hawck-inputd.cpp',
  'UDevice.cpp',
  'Version.cpp',
  'Daemon.cpp',
  'KBDDaemon.cpp',
  'Keyboard.cpp',
  'FSWatcher.cpp',
  'CSV.cpp',
  'Permissions.cpp',
  'LuaUtils.cpp',
  'KBDManager.cpp',
]
executable('hawck-inputd',
           inputd_src,
           dependencies : [pthreaddep, luadep],
           include_directories : conf_inc,
           install : true,
          )

if get_option('development_build')
    hawck_chroot_src = [
      'hawck-chroot.c'
    ]
    executable('hawck-chroot',
               hawck_chroot_src,
               dependencies : [],
               include_directories : conf_inc,
               install : false)

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
endif
