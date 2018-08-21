python3 = import('python3')

if python3.language_version() < '3.6'
  error('Require python version >=3.6')
endif 

run_target('hawck-ui',
  command : [python3.find_python(), 'setup.py', 'build'],
)

meson.add_install_script('install-hawck-ui.sh')
