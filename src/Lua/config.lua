local u = require "utils"
local json = require "json"

local reg = debug.getregistry()

reg["config"] = {}

reg["modified"] = {}

local real_config = reg["config"]
local modified = reg["modified"]

local ConfigMeta = {
  __index = reg["config"]
}

function ConfigMeta.__newindex(t, idx, val)
  print(("metamethod called %s, %s, %s"):format(t, idx, val))
  real_config[idx] = val
  table.insert(modified, idx)
end

config = {}
setmetatable(config, ConfigMeta)

function dumpConfig(path)
  local file = io.open(path, "w")
  if not file then
    error(("Unable to open file for writing: %s"):format(path))
  end
  u.serialize("config", real_config, file)
  file:close()
end

function loadConfig(path)
  local file = io.open(path, "r")
  if not file then
    error(("Unable to open file for reading: %s"):format(path))
  end
  local conf = load(file:read("*a"))()
  for u, v in pairs(conf) do
    real_config[u] = v
  end
end

-- Call relevant functions in MacroD to set the
-- configuration options
function setOptions()
  for i, name in ipairs(modified) do
    local fn_name = "set_" .. name
    local fn = MacroD[fn_name]
    if fn then
      fn(MacroD, real_config[name])
    end
  end
end

function getChanged()
  local was_modified = modified
  reg["modified"] = {}
  modified = reg["modified"]
  return was_modified
end

function getConfigs(...)
  local configs = {}
  for i, name in ipairs({...}) do
    table.insert(configs, real_config[name])
  end
  u.puts(configs)
  return table.unpack(configs)
end

function exec(cmd)
  local env = {
    config = config,
    puts = puts,
  }
  local fn = load(cmd, nil, nil, env)
  local ss = json.sstream.new()
  local ret = {fn()}
  u.puts(ret)
  json.serialize(ret, ss)
  return ss:get()
end

