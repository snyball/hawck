project('Hawck', 'cpp',
        version : '0.7',
        license : 'BSD-2',
        meson_version : '>=0.45.0',
        default_options : ['c_std=c18',
                           'cpp_std=c++17',
                          ])

os = host_machine.system()
if os != 'linux'
  error('Hawck is currently only supported on Linux')
endif

## This option is on a completely different level of pedantically ridiculous.
add_global_arguments('-Wno-reorder', language : 'cpp')

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
subdir('bin')

meson.add_install_script('bin/hawck-install.sh')
