conf = configuration_data()
conf.set('PREFIX', get_option('prefix'))

systemd_prefix = '/usr/lib/systemd'
if get_option('prefix') == '/usr/local'
  systemd_prefix = '/etc/systemd'
endif

conf.set('HAWCK_CFG_DIR', get_option('hawck_cfg_dir'))
conf.set('HAWCK_SHARE_DIR', hawck_full_share_dir)
conf.set('MODULES_LOAD_DIR', get_option('modules_load_dir'))
conf.set('UDEV_RULES_DIR', get_option('udev_rules_dir'))

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

configure_file(
  input: 'hawck-user-setup.sh.in',
  output: 'hawck-user-setup.sh',
  configuration: conf
)

configure_file(
  input: 'hawck-install.sh.in',
  output: 'hawck-install.sh',
  configuration: conf
)

hawck_install_script = join_paths(meson.current_build_dir(), 'hawck-install.sh')
