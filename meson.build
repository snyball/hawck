project('Hawck', 'c', 'cpp',
        version : '0.7.1',
        license : 'BSD-2',
        meson_version : '>=0.49.0',
        default_options : ['c_std=c18',
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
subdir('hawck-ui')
subdir('docs')
subdir('bin')

meson.add_install_script(hawck_install_script)

# summary() instead of the message at the bottom would be neat, it's
# available only in 0.53.0
message(
'\n------------------------------------------------------------------------\n'+
'  '+meson.project_name()+' ' +meson.project_version()+':  Automatic configuration OK.'+'\n'+
'\n  General configuration:'+'\n'+
'\n    redirect_std: ................ '+get_option('redirect_std').to_string('yes', 'no')+
'\n    use_meson_install: ........... '+get_option('use_meson_install').to_string('yes', 'no')+
'\n    development_build: ........... '+get_option('development_build').to_string('yes', 'no')+'\n'+
'\n  Installation paths:'+'\n'+
'\n    prefix: ...................... '+get_option('prefix')+
'\n    hawck_share_dir: ............. '+hawck_full_share_dir+
'\n    hawck_cfg_dir: ............... '+get_option('hawck_cfg_dir')+
'\n    modules_load_dir: ............ '+get_option('modules_load_dir')+
'\n    udev_rules_dir: .............. '+get_option('udev_rules_dir')+'\n'
)
