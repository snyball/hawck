conf = configuration_data()
conf.set('PREFIX', get_option('prefix'))

configure_file(
  input: 'hawck-inputd.service.in',
  output: 'hawck-inputd.service',
  install_dir: get_option('libdir') / 'systemd' / 'system',
  configuration: conf
)

configure_file(
  input: 'hawck-macrod.service.in',
  output: 'hawck-macrod.service',
  install_dir: get_option('libdir') / 'systemd' / 'user',
  configuration: conf
)
