project('Hawck', 'c', 'cpp',
        version : '0.7.1',
        license : 'BSD-2',
        meson_version : '>=0.53.0',
        default_options : ['c_std=c11',
                           'cpp_std=c++17'])

os = host_machine.system()
if os != 'linux'
  error('Hawck is currently only supported on Linux')
endif

## This option is on a completely different level of pedantically ridiculous.
add_global_arguments('-Wno-reorder', language : 'cpp')

## Force includes of lua5.3, fails to be included on some systems.
add_global_arguments('-I/usr/include/lua5.3', language : 'cpp')

add_global_arguments('-DMESON_COMPILE=1', language : 'cpp')

inc = include_directories('src')

hawck_full_share_dir = join_paths(get_option('prefix'), get_option('hawck_share_dir'))

subdir('src')
subdir('tests')
subdir('docs')
subdir('bin')

meson.add_install_script(hawck_install_script)

summary({'redirect_std' : get_option('redirect_std').to_string('yes', 'no'),
         'use_meson_install' : get_option('use_meson_install').to_string('yes', 'no'),
         'development_build' : get_option('development_build').to_string('yes', 'no'),
         'with_systemd' : get_option('with_systemd').to_string('yes', 'no'),
        }, section: 'Configuration')
summary({'prefix' : get_option('prefix'),
         'hawck_share_dir' : hawck_full_share_dir,
         'hawck_cfg_dir' : get_option('hawck_cfg_dir'),
         'modules_load_dir' : get_option('modules_load_dir'),
         'udev_rules_dir' : get_option('udev_rules_dir'),
        }, section: 'Installation paths')
