project('Hawck', 'cpp',
        version : '0.4',
        license : 'BSD-2',
        default_options : ['c_std=c11',
                           'cpp_std=c++17',
                          ])

## Hide warnings emitted by GTK+
add_global_arguments('-Wno-parentheses',
                     language : 'cpp')

## Optimize for size
add_global_arguments('-Os',
                     language : 'cpp')

add_global_arguments('-I/usr/include/lua5.3',
                     language : 'cpp')

inc = include_directories('src')

subdir('src')
subdir('tests')
subdir('hawck-ui')

de_user = get_option('desktop_user')
if de_user == ''
  warning('Option desktop_user not defined, run `meson configure -Ddesktop_user=$(whoami)`')
endif

meson.add_install_script('build-scripts/hawck-setup.sh', de_user)
#meson.add_install_script('build-scripts/install-hawck-ui.sh')
#meson.add_install_script('build-scripts/arg-test.sh', 'abc')

