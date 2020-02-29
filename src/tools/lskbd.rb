#!/usr/bin/ruby

doc = <<DOCOPT
lskbd

Usage:
  #{__FILE__} --list-kbd | -k [--json | -j]
  #{__FILE__} --list-all | -a [--json | -j]
  #{__FILE__} -h | --help
  #{__FILE__} --version

Options:
  -h --help      Show this screen.
  --version      Show version.
  --list-kbd -k  List all keyboards.
  --list-all -a  List all input devices.
  --json -j      Output JSON

DOCOPT

def varfile(path)
  vars = {}
  begin
    File.readlines(path).each do |line|
      _, u, v = line.match(/^(\w+)=(.+)$/).to_a
      vars[u] = v
    end
  rescue Errno::ENOENT
    nil
  end
  vars
end

def collect_devline(line, dev)
  info_line_rx = /^([A-Z]): (.*)$/
  i_rx = /Bus=(\h+) Vendor=(\h+) Product=(\h+) Version=(\h+)$/
  n_rx = /^Name="(.*)"$/
  g_rx = /^([A-Za-z]+)=(.*)$/

  _, t, info = info_line_rx.match(line).to_a
  if t == 'I'
    m = i_rx.match(info).to_a.map { |v| v.to_i(16) }
    _, dev['Bus'], dev['Vendor'], dev['Product'], dev['Version'] = m
  else
    u, v = case t
           when 'N'
             ['Name', n_rx.match(info)[1]]
           when 'H'
             _, key, value = g_rx.match(info).to_a
             [key, value.split(' ')]
           else
             g_rx.match(info).to_a[1, 2]
           end
    dev[u] = v
  end
end

def lsinput
  device_lines = []
  tmp = []

  File.readlines('/proc/bus/input/devices').each do |line|
    if line =~ /^ *$/
      device_lines << tmp
      tmp = []
      next
    end
    tmp << line
  end

  devices = []
  device_lines.each do |dev_lines|
    dev = {}
    dev_lines.each { |line| collect_devline(line, dev) }
    uevent_path = File.join('/sys', dev['Sysfs'], 'device', 'uevent')
    dev['uevent'] = varfile(uevent_path)
    dev['events'] = Dir.glob(File.join('/sys', dev['Sysfs'], 'event*'))
                       .map(&File.method(:basename)).map do |ev|
      devi = File.join('/dev', varfile(File.join('/sys', dev['Sysfs'], ev,
                                                 'uevent'))['DEVNAME'])
      Dir.chdir('/dev/input/by-path') do
        Dir.glob('*').each do |path|
          if File.realpath(File.readlink(path)) == devi
            devi = File.join('/dev/input/by-path', path)
            break
          end
        end
      end
      devi
    end

    dev['ID'] = (dev['Vendor'].to_i.to_s +
                 ':' +
                 dev['Product'].to_i.to_s +
                 ':' +
                 dev['Name'].gsub(/ /, '_'))
    devices << dev
  end

  devices
end

def lskbd
  lsinput.filter do |dev|
    drv = dev['uevent']['DRIVER']
    drv&.match(/kbd$/)
  end
end

def listify_object(obj)
  obj.to_a.map do |pair|
    u, v = pair
    v = listify_object(v) if v.is_a?(Hash)
    { u => v }
  end
end

def delete_empty(obj)
  obj.to_a.each do |u, v|
    if v == ''
      obj.delete(u)
    elsif v.is_a?(Hash)
      delete_empty(obj[u])
    end
  end
end

def print_devs(devs)
  devs.each do |dev|
    name = dev['Name']
    dev.delete('Name')
    delete_empty(dev)
    print YAML.dump({ name => listify_object(dev) })
  end
end

if ARGV.length.zero?
  lskbd.each do |dev|
    dev['events'].each { |ev| puts ev.to_s }
  end
else
  require 'docopt'
  require 'yaml'
  require 'json'

  begin
    opt = Docopt.docopt(doc)
  rescue Docopt::Exit => e
    puts e.message
    exit 1
  end

  if opt['--list-kbd']
    if opt['--json']
      print JSON.dump(lskbd)
    else
      print_devs(lskbd)
    end
  elsif opt['--list-all']
    if opt['--json']
      print JSON.dump(lsinput)
    else
      print_devs(lsinput)
    end
  end
end
