require "json"
rx = /#define (\w+)\s+(0x[0-9a-fA-F]+|\d+|\(.*?\)|\w+)\s*(\/\* *(.*?) *\*\/)?/m
obj = {}
File.read("/usr/include/linux/input-event-codes.h").scan(rx).each do |i|
  obj[i[0]] = i[1].to_i(16)
  i[1].scan(/(\w+)/).each do |j|
    if obj[j[0]] then
      obj[i[0]] = obj[j[0]]
    end
  end
  i[1].scan(/\((\w+)([\+-])(\d)\)/).each do |j|
    u, v = obj[j[0]], j[2].to_i
    obj[i[0]] = if j[2] == "-"
                  u - v
                else
                  u + v
                end
  end
end
puts(JSON.dump(obj))
