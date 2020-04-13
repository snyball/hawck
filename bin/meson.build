conf = configuration_data()
conf.set('PREFIX', get_option('prefix'))

systemd_prefix = '/usr/lib/systemd'
if get_option('prefix') == '/usr/local'
  systemd_prefix = '/etc/systemd'
endif

configure_file(
  input: 'hawck-inputd.service.in',
  output: 'hawck-inputd.service',
  install_dir: systemd_prefix / 'system',
  configuration: conf
)

configure_file(
  input: 'hawck-macrod.service.in',
  output: 'hawck-macrod.service',
  install_dir: systemd_prefix / 'user',
  configuration: conf
)
