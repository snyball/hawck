#!/usr/bin/ruby

doc = <<DOCOPT
lskbd

Usage:
  #{__FILE__} --list-kbd | -k
  #{__FILE__} --list-kbd-json | -K
  #{__FILE__} --list-all | -l
  #{__FILE__} --list-all-json | -L
  #{__FILE__} --hawck-args
  #{__FILE__} -h | --help
  #{__FILE__} --version

Options:
  -h --help             Show this screen.
  --version             Show version.
  --hawck-args          Print out arguments for hawck-inputd.
  --list-kbd -k         List all keyboards.
  --list-kbd-json -K    List all keyboards in JSON format.
  --list-all -l         List all input devices.
  --list-all-json -L    List all input devices in JSON format.

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

def lsinput
  device_lines = []
  tmp = []

  File.readlines("/proc/bus/input/devices").each do |line|
    if line =~ /^ *$/ then
      device_lines << tmp
      tmp = []
      next
    end
    tmp << line
  end

  info_line_rx = /^([A-Z]): (.*)$/
  i_rx = /Bus=(\d*) Vendor=(\d*) Product=(\d*) Version=(\h*)$/
  n_rx = /^Name="(.*)"$/
  g_rx = /^([A-Za-z]+)=(.*)$/
  devices = []
  device_lines.each do |dev_lines|
    dev = {}
    dev_lines.each do |line|
      _, t, info = info_line_rx.match(line).to_a
      if t == "I" then
        _, dev["Bus"], dev["Vendor"], dev["Product"], dev["Version"] = i_rx.match(info).to_a
      else
        u, v = case t
                 when "N"
                   ["Name", n_rx.match(info)[1]]
                 when "H"
                   _, key, value = g_rx.match(info).to_a
                   [key, value.split(" ")]
                 else
                   g_rx.match(info).to_a[1,2]
               end
        dev[u] = v
      end
    end

    uevent_path = File.join("/sys", dev["Sysfs"], "device", "uevent")
    dev["uevent"] = varfile(uevent_path)
    dev["events"] = Dir.glob(File.join("/sys", dev["Sysfs"], "event*")).map(&File::method(:basename)).map do |ev|
      dev_input = File.join("/dev", varfile(File.join("/sys", dev["Sysfs"], ev, "uevent"))["DEVNAME"])
      # Prefer by-path
      Dir.chdir("/dev/input/by-path") do
        Dir.glob("*").each do |path|
          if File.realpath(File.readlink(path)) == dev_input then
            dev_input = File.join("/dev/input/by-path", path)
            break
          end
        end
      end
      dev_input
    end
    devices << dev
  end

  return devices
end

def lskbd()
  ## $ date -I
  ## 2020-02-10
  ## $ fd 'kbd' linux-master
  # kbd_drivers = ["hid-holtek-kbd", "jornada680_kbd", "parkbd",
  #                "hilkbd", "opencores-kbd", "amikbd",
  #                "atakbd", "hpps2atkbd", "q40kbd",
  #                "locomokbd", "ioc3kbd", "rpckbd",
  #                "xtkbd", "jornada720_kbd", "hil_kbd",
  #                "lkkbd", "atkbd", "newtonkbd",
  #                "sunkbd", "usbkbd", "atarikbd", "nvec_kbd"]
  ## All the keyboard drivers appear to end in "kbd", so kbd_drivers isn't used,
  ## but it stays here as justification for why i do `drv.match(/kbd$/)`.

  lsinput.filter do |dev|
    drv = dev["uevent"]["DRIVER"]
    drv and drv.match(/kbd$/)
  end
end

def listify_object(obj)
  obj.to_a.map do |pair|
    u, v = pair
    if v.is_a?(Hash) then
      v = listify_object(v)
    end
    {u => v}
  end
end

def delete_empty(obj)
  obj.to_a.each do |u, v|
    if v == "" then
      obj.delete(u)
    elsif v.is_a?(Hash) then
      delete_empty(obj[u])
    end
  end
end

def print_devs(devs)
  devs.each do |dev|
    name = dev["Name"]
    dev.delete("Name")
    delete_empty(dev)
    print YAML.dump({name => listify_object(dev)})
  end
end

if ARGV.length == 0
  lskbd.each do |dev|
    dev["events"].each { |ev| puts "#{ev}" }
  end
else
  require "docopt"
  require "yaml"
  require "json"

  begin
    opt = Docopt::docopt(doc)
  rescue Docopt::Exit => e
    puts e.message
    exit 1
  end

  if opt["--list-kbd"]
    print_devs(lskbd)
  elsif opt["--list-all"]
    print_devs(lsinput)
  elsif opt["--list-all-json"]
    print JSON.dump(lsinput)
  elsif opt["--list-kbd-json"]
    print JSON.dump(lskbd)
  elsif opt["--hawck-args"]
    lskbd.each do |dev|
      dev["events"].each { |ev| print "--kbd-device #{ev} " }
    end
  end
end
