local u = require "utils"

local json = {}

local serializeJSONRecursive

local indent_str = "  "

---
--- Print the string `c` to standard output `n` times
---
local function printN(c, n, out)
  local out = out or io.stdout
  for i = 1, n do
    out:write(c)
  end
end

local function keyString(v)
  if type(v) ~= "string" then
    error("Key must be a string")
  end
  return u.reprString(v)
end

local function serialize_array(t, indent, path, visited, links, out)
  local indent = indent or 0
  local visited = visited or {}
  local path = path or {"root"}
  local links = links or {}
  local out = out or io.stdout

  local per_line = 8
  out:write "[\n"
  local i = 1
  printN(indent_str, indent + 1, out)
  local len = #t
  for i, v in ipairs(t) do
    if type(v) == "table" and not table.empty(v) and i % per_line ~= 0 then
      out:write "\n"
      printN(indent_str, indent + 1, out)
      i = 0
    end
    serializeJSONRecursive(v, indent + 1, path, visited, links, out)
    if i ~= len then
      out:write ", "
    end
    if i % per_line == 0 and i ~= len then
      out:write "\n"
      printN(indent_str, indent + 1, out)
    end
    i = i + 1
  end
  out:write "\n"
  printN(indent_str, indent, out)
  out:write "]"
end

local function isAtomic(x)
  return type(x) ~= "table" or table.empty(x)
end

local function putsAtomic(x, out)
  local out = out or io.stdout
  if x == nil then
    out:write("null")
  elseif type(x) == "string" then
    out:write(u.reprString(x))
  elseif type(x) == "boolean" or type(x) == "number" then
    out:write(tostring(x))
  elseif type(x) == "function" then
    error("Functions are not serializable to json")
  elseif type(x) == "userdata" or type(x) == "thread" then
    error("Userdatum are not serializable to json")
  elseif table.empty(x) then
    out:write "{}"
  else
    return false
  end
  return true
end

local function isArray(t)
  local i = 0
  for u, v in pairs(t) do
    i = i + 1
    if t[i] == nil then
      return false
    end
  end
  return true
end

function serializeJSONRecursive(t, indent, path, visited, links, out)
  local indent = indent or 0
  local visited = visited or {}
  local path = path or {"root"}
  local links = links or {}
  local out = out or io.stdout

  if putsAtomic(t, out) then
  else
    if visited[t] then
      error("Table has recursive dependencies")
    end

    visited[t] = table.concat(path, "")

    if isArray(t) then
      serialize_array(t, indent, path, visited, links, out)
    else
      out:write "{\n"

      local keys = {}
      for k, v in pairs(t) do
        table.insert(keys, k)
      end

      for i, k in ipairs(keys) do
        local v = t[k]
        printN(indent_str, indent+1, out)
        out:write(keyString(k) .. ": ")
        table.insert(path, keyString(k, false))
        serializeJSONRecursive(v, indent + 1, path, visited, links, out)
        path[#path] = nil
        if i ~= #keys then
          out:write(",\n")
        end
      end
      out:write "\n"

      printN(indent_str, indent, out)
      out:write "}"
    end
  end

  if indent == 0 then
    out:write "\n"
  end
end

function json.serialize(t, out)
  out = out or io.stdout
  serializeJSONRecursive(t, 0, {}, {"root"}, {}, out)
end

local sstream = {}

json.sstream = sstream

function sstream:write(str)
  self.arr[#self.arr + 1] = str
end

function sstream:get()
  return table.concat(self.arr, "")
end

function sstream.new(str)
  local buf = {
    arr = {str or ""}
  }
  local meta = {
    __index = sstream
  }
  setmetatable(buf, meta)
  return buf
end

return json
