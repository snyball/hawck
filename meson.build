project('Hawck', 'cpp',
        version : '0.6',
        license : 'BSD-2',
        meson_version : '>=0.45.0',
        default_options : ['c_std=c11',
                           'cpp_std=c++17',
                          ])

os = host_machine.system()
if os != 'linux'
  error('Hawck is currently only supported on Linux')
endif

## Hide warnings emitted by GTK+
add_global_arguments('-Wno-parentheses', language : 'cpp')

## This option is on a completely different level of pedantically ridiculous.
add_global_arguments('-Wno-reorder', language : 'cpp')
add_global_arguments('-Wall', language : 'cpp')
add_global_arguments('-Wextra', language : 'cpp')

if get_option('debug_hawck')
  # add_global_arguments('-Wshadow', language : 'cpp')
  add_global_arguments('-Wcomment', language : 'cpp')
  add_global_arguments('-Wformat', language : 'cpp')
  add_global_arguments('-Wnonnull', language : 'cpp')
  add_global_arguments('-Wuninitialized', language : 'cpp')
  add_global_arguments('-Wswitch', language : 'cpp')
  add_global_arguments('-Winline', language : 'cpp')
  add_global_arguments('-Werror', language : 'cpp')
endif

## Force includes of lua5.3, fails to be included on some systems.
add_global_arguments('-I/usr/include/lua5.3',
                     language : 'cpp')

add_global_arguments('-DMESON_COMPILE=1',
                     language : 'cpp')

inc = include_directories('src')

subdir('src')
subdir('tests')
subdir('hawck-ui')
subdir('docs')

if get_option('use_meson_install')
  meson.add_install_script('bin/hawck-setup.sh')
endif

