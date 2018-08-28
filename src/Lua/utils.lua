--[====================================================================================[
   Various utility functions for Lua
   
   Copyright (C) 2018 Jonas Møller (no) <jonasmo441@gmail.com>
   All rights reserved.
   
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   
   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--]====================================================================================]

u = {}
local indent_str = "  "
local builtins = {} -- require "builtins"
local string_byte, string_char, concat = string.byte, string.char, table.concat
local serialize_recursive
local unpack = table.unpack

function u.append(t, val)
  t[#t + 1] = val
end

-- Example:
--   join({1,2,3},{4,5,6},{"a","b","c"}) -> {1,2,3,4,5,6,"a","b","c"}
function u.join(...)
  local t = {}
  for _, table in pairs({...}) do
    for _, x in pairs(table) do
      t[#t + 1] = x
    end
  end
  return t
end

-- Example:
--   curry((function (a, b) return a + b end), 2) ->
--     function (b) return 2 + b end -- (Essentially)
function u.curry(f, ...)
  local pre = {...}
  return function (...)
    return f(unpack(u.join(pre, {...})))
  end
end

---
--- Print the string `c` to standard output `n` times
---
local function printN(c, n)
  for i = 1, n do
    io.write(c)
  end
end

---
--- Check if the given table `t` is an array.
---
function u.isArray(t)
  local i = 0
  for u, v in pairs(t) do
    i = i + 1
    if t[i] == nil then
      return false
    end
  end
  return true
end

local function hexDigit(d)
  local letters = {
    "a", "b", "c", "d", "e", "f"
  }
  return (d <= 9 and tostring(d)) or letters[d - 9]
end

function u.hex(i, pad)
  local pad = pad or 0
  local digits = {}
  while i ~= 0 do
    table.insert(digits, i % 16)
    i = i // 16
  end
  local str = "x"
  for i = 1, pad - #digits do
    str = str .. "0"
  end
  for i = #digits, 1, -1 do
    str = str .. hexDigit(digits[i])
  end
  return str
end

local LateLinkMeta = {
  __what = "LateLink"
}

function LateLinkMeta.__call(t)
  return t.link()
end

local LateLink = {}

function LateLink.new(f)
  local t = {
    link = f
  }
  setmetatable(t, LateLinkMeta)
  return t
end

local string_escapes = {
  [string.byte("\a")] = "a",
  [string.byte("\b")] = "b",
  [string.byte("\t")] = "t",
  [string.byte("\n")] = "n",
  [string.byte("\v")] = "v",
  [string.byte("\f")] = "f",
  [string.byte("\r")] = "r"
}

local function reprString(v)
  local quote = string_byte("\"")
  local bytes = {string_byte(v, 1, #v)}
  local str = ""
  for i = 1, #bytes do
    local c = bytes[i]
    if c < 0x20 or c >= 0x7f then
      bytes[i] = "\\" .. (string_escapes[c] or u.hex(c, 2))
    elseif c == quote then
      bytes[i] = "\\\""
    else
      bytes[i] = string_char(c)
    end
  end
  return "\"" .. concat(bytes, "") .. "\""
end

local function keyString(v, nice)
  if nice == nil then nice = true end

  if type(v) == "string" then
    if not v:match("%s") and not v:match("^%d") and nice then
      return v
    end
    return "[" .. reprString(v) .. "]"
  elseif type(v) == "function" then
    if builtins[v] then
      return "[" .. builtins[v] .. "]"
    else
      return keyString(tostring(v), false)
    end
  elseif type(v) == "userdata" and builtins[v] then
    return "[" .. builtins[v] .. "]"
  elseif type(v) == "number" or type(v) == "boolean" then
    return "[" .. tostring(v) .. "]"
  else
    return keyString(tostring(v))
  end
end

function table.empty(t)
  for _ in pairs(t) do
    return false
  end
  return true
end

local function serialize_array(t, indent, path, visited, links)
  local indent = indent or 0
  local visited = visited or {}
  local path = path or {"root"}
  local links = links or {}

  local per_line = 8
  print "{"
  local i = 1
  printN(indent_str, indent + 1)
  local len = #t
  for _, v in ipairs(t) do
    if type(v) == "table" and not table.empty(v) and i % per_line ~= 0 then
      print ""
      printN(indent_str, indent + 1)
      i = 0
    end
    serialize_recursive(v, indent + 1, path, visited, links)
    io.write ", "
    if i % per_line == 0 and i ~= len then
      print ""
      printN(indent_str, indent + 1)
    end
    i = i + 1
  end
  print ""
  printN(indent_str, indent)
  io.write "}"
end

function isAtomic(x)
  return type(x) ~= "table" or table.empty(x)
end

function putsAtomic(x)
  if x == nil then
    io.write("nil")
  elseif type(x) == "string" then
    io.write(reprString(x))
  elseif type(x) == "boolean" or type(x) == "number" then
    io.write(tostring(x))
  elseif type(x) == "function" then
    local success, bytecode = pcall(function ()
        return string.dump(x)
    end)
    if success then
      io.write("load(")
      io.write(reprString(bytecode))
      io.write(")")
    elseif builtins[x] then
      io.write(builtins[x] .. " --[[ builtin ]]")
    else
      io.write(("error(\"Not serializable: %s\")"):format(tostring(x)))
    end
  elseif type(x) == "userdata" or type(x) == "thread" then
    if builtins[x] then
      io.write(builtins[x] .. " --[[ builtin ]]")
    else
      io.write(("error(\"Not serializable: %s\")"):format(tostring(x)))
    end
  elseif table.empty(x) then
    io.write "{}"
  else
    return false
  end
  return true
end

function serialize_recursive(t, indent, path, visited, links)
  local indent = indent or 0
  local visited = visited or {}
  local path = path or {"root"}
  local links = links or {}

  if putsAtomic(t) then
  else
    if visited[t] then
      io.write(("nil --[[ %s ]]"):format(visited[t]))
      table.insert(links, {visited[t], concat(path, "")})
      return
    end

    visited[t] = concat(path, "")

    if u.isArray(t) then
      serialize_array(t, indent, path, visited, links)
    else
      print "{"

      for u, v in pairs(t) do
        printN(indent_str, indent+1)
        io.write(keyString(u) .. " = ")
        table.insert(path, keyString(u, false))
        serialize_recursive(v, indent + 1, path, visited, links)
        path[#path] = nil
        print ","
      end

      printN(indent_str, indent)
      io.write "}"
    end
  end

  if indent == 0 then
    print ""
  end
end

---
--- Print `t` in a human readable format.
---
function u.puts(t, indent)
  serialize_recursive(t, indent)
end

local function formatLink(link, name)
  return link[2] .. " = " .. link[1]
end

---
--- Serialize a value `t` with the name `name`
---
function u.serialize(name, t)
  io.write("local " .. name .. " = ")

  local visited = {}
  local links = {}
  serialize_recursive(t, 0, {name}, visited, links)

  for k, v in ipairs(links) do
    print(formatLink(v, name))
  end

  print("return " .. name)
end

function u.serializeAs(t, name)
  print(name .. " = {}")
  for u, v in pairs(t) do
    io.write(name .. "[")
    puts(u, 1)
    io.write("]" .. " = ")
    puts(v, 0, {name, u})
  end
end

---
--- Map `f` over `xs` and return the mapping as an array.
---
function u.map(f, xs)
  for k, v in xs do
    f(k, v)
  end
end

---
--- Map `f` over `xs` and return the mapping as an iterator.
---
function u.mapi(f, xs)
  function iter (xs, i)
    local i = i + 1
    local v = xs[i]
    if v then
      return i, f(v)
    end
  end

  return iter, xs, 0
end

---
--- Expand environment variables in the given string.
--- Environment variables should appear in the form: $variable
---
function u.expandEnv(path)
  for m in path:gmatch("%$([a-zA-Z_]+)") do
    path = path:gsub("%$" .. m, os.getenv(m) or "")
  end

  return path
end

local test = false
if test then
  __puts_test = {
    a = "b",
    c = {
      "d", "e", "f", {3,2,1,2,3,4,1,2,3,4,5,3,{},1,2,{},32,2}, 1, 2, 3, 4, 5, 6, 7, 8
    },
    [true] = "this",
    [1337] = 7331,
    ["This is a \"key\""] = "this\n is\n'\t a \"value\"",
    d = {
      e = {
        f = {
          z = 2,
          ds = 389,
          fn = function (x) print("This: " .. x) end
        }
      }
    }
  }

  -- Recursive test
  __puts_test.self = __puts_test
  __puts_test.a = {}
  __puts_test.a.b = {}
  __puts_test.a.b.c = {}
  __puts_test.a.b.c.d = __puts_test.a
  __puts_test.a.b.d = __puts_test.a.b.c
  __puts_test.a.b.c.fn = function ()
    __puts_test.d.e.f.fn("YEEEEEE BOIIIII")
  end

  -- serialize("__puts_test", __puts_test)
  serialize("globals", _G)
  puts("string")
  puts(function() end)
end

