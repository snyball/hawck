python3 = import('python3')

if python3.language_version() < '3.6'
  error('Require python version >=3.6')
endif 

conf_data = configuration_data()
conf_data.set_quoted('version', meson.project_version())
configure_file(input : 'setup_config.json.in',
               output : 'setup_config.json',
               configuration : conf_data
              )

run_target('hawck-ui',
  command : [python3.find_python(), 'setup.py', 'build'],
)

if get_option('use_meson_install')
  meson.add_install_script('install-hawck-ui.sh')
endif
