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

local strict = require "strict"
local u = {}
local indent_str = "  "
local builtins = {} -- require "builtins"
local string_byte, string_char, concat = string.byte, string.char, table.concat
local serialize_recursive
local unpack = table.unpack

--- Append to an array
-- @param t The array to append to.
-- @param val The value to append.
-- @return nil
function u.append(t, val)
  t[#t + 1] = val
end

--- Join a number of arrays together.
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

--- Curry a function.
-- 
-- Example:
--   curry((function (a, b) return a + b end), 2) ->
--     function (b) return 2 + b end -- (Essentially)
--
-- @param f The function to be curried
-- @return The curried function
function u.curry(f, ...)
  local pre = {...}
  return function (...)
    return f(unpack(u.join(pre, {...})))
  end
end

--- Print the string `c` to standard output `n` times
-- @param c String to print
-- @param n How many times to print it
-- @param out Optional output stream [default: io.stdout]
local function printN(c, n, out)
  local out = out or io.stdout
  for i = 1, n do
    out:write(c)
  end
end

--- Check if the given table `t` is an array.
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

--- Get the hexadecimal representation of a number
--
-- @param i The number to represent.
-- @param pad Optional padding zeroes [default: 0]
-- @return The hexadecimal representation of the number i.
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

local string_escapes = {
  [string.byte("\a")] = "a",
  [string.byte("\b")] = "b",
  [string.byte("\t")] = "t",
  [string.byte("\n")] = "n",
  [string.byte("\v")] = "v",
  [string.byte("\f")] = "f",
  [string.byte("\r")] = "r"
}

--- Get the valid Lua representation of a string.
-- @param v The string to represent
-- @return Valid Lua representation of the given string.
function u.reprString(v)
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
    return "[" .. u.reprString(v) .. "]"
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

--- Check if a table is empty
-- @param t The table to check
-- @return True if empty, false otherwise.
function table.empty(t)
  assert(type(t) == "table")
  for _ in pairs(t) do
    return false
  end
  return true
end

local function serialize_array(t, indent, path, visited, links, out)
  local indent = indent or 0
  local visited = visited or {}
  local path = path or {"root"}
  local links = links or {}
  local out = out or io.stdout

  local per_line = 8
  out:write "{\n"
  local i = 1
  printN(indent_str, indent + 1, out)
  local len = #t
  for _, v in ipairs(t) do
    if type(v) == "table" and not table.empty(v) and i % per_line ~= 0 then
      out:write "\n"
      printN(indent_str, indent + 1, out)
      i = 0
    end
    serialize_recursive(v, indent + 1, path, visited, links, out)
    out:write ", "
    if i % per_line == 0 and i ~= len then
      out:write "\n"
      printN(indent_str, indent + 1, out)
    end
    i = i + 1
  end
  out:write "\n"
  printN(indent_str, indent, out)
  out:write "}"
end

local function isAtomic(x)
  return type(x) ~= "table" or table.empty(x)
end

local function putsAtomic(x, out)
  local out = out or io.stdout
  if x == nil then
    out:write("nil")
  elseif type(x) == "string" then
    out:write(u.reprString(x))
  elseif type(x) == "boolean" or type(x) == "number" then
    out:write(tostring(x))
  elseif type(x) == "function" then
    local success, bytecode = pcall(function ()
        return string.dump(x)
    end)
    if success then
      out:write("load(")
      out:write(u.reprString(bytecode))
      out:write(")")
    elseif builtins[x] then
      out:write(builtins[x] .. " --[[ builtin ]]")
    else
      out:write(("error(\"Not serializable: %s\")"):format(tostring(x)))
    end
  elseif type(x) == "userdata" or type(x) == "thread" then
    if builtins[x] then
      out:write(builtins[x] .. " --[[ builtin ]]")
    else
      out:write(("error(\"Not serializable: %s\")"):format(tostring(x)))
    end
  elseif table.empty(x) then
    out:write "{}"
  else
    return false
  end
  return true
end

function serialize_recursive(t, indent, path, visited, links, out)
  local indent = indent or 0
  local visited = visited or {}
  local path = path or {"root"}
  local links = links or {}
  local out = out or io.stdout

  if putsAtomic(t, out) then
  else
    if visited[t] then
      out:write(("nil --[[ %s ]]"):format(visited[t]))
      table.insert(links, {visited[t], concat(path, "")})
      return
    end

    visited[t] = concat(path, "")

    if u.isArray(t) then
      serialize_array(t, indent, path, visited, links, out)
    else
      out:write "{\n"

      for u, v in pairs(t) do
        printN(indent_str, indent+1, out)
        out:write(keyString(u) .. " = ")
        table.insert(path, keyString(u, false))
        serialize_recursive(v, indent + 1, path, visited, links, out)
        path[#path] = nil
        out:write ",\n"
      end

      printN(indent_str, indent, out)
      out:write "}"
    end
  end

  if indent == 0 then
    out:write "\n"
  end
end

--- Print `t` in a human readable format.
-- @param t The value to print
-- @param indent Optional starting indentation [default: 0]
function u.puts(t, indent)
  serialize_recursive(t, indent)
end

local function formatLink(link, name)
  return link[2] .. " = " .. link[1]
end

--- Serialize a value `t` with the name `name`
-- @param name Name of the table in the output.
-- @param t The value to serialize.
-- @param out Optional output stream [default: io.stdout]
function u.serialize(name, t, out)
  local out = out or io.stdout
  out:write("local " .. name .. " = ")

  local visited = {}
  local links = {}
  serialize_recursive(t, 0, {name}, visited, links, out)

  for k, v in ipairs(links) do
    print(formatLink(v, name))
  end

  out:write("return " .. name)
  out:write("\n")
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

--- Map `f` over `xs` and return the mapping as an array.
-- @tparam function f Function to map over xs
-- @tparam table xs Array of values to be mapped over.
-- @treturn table Result of mapping.
function u.map(f, xs)
  for k, v in xs do
    f(k, v)
  end
end

--- Map `f` over `xs` and return the mapping as an iterator.
-- @tparam function f Function to map over xs
-- @tparam table xs Array of values to be mapped over.
-- @treturn function Iterator.
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

function u.expect(typen, var)
  local got = type(var)
  assert(got == typen, ("Expected %s, got: %s"):format(typen, got))
end

--- Expand environment variables in the given string.
-- 
-- Environment variables should appear in the form: $variable
--
-- @tparam string path
-- @treturn string The original path with $vars expanded.
function u.expandEnv(path)
  -- FIXME: This may also expand variables with '$var' inside them.
  for m in path:gmatch("%$([a-zA-Z_]+)") do
    path = path:gsub("%$" .. m, os.getenv(m) or "")
  end
  return path
end

--- Split a string based on a given pattern string.
-- @tparam string rx Pattern string to use for splitting.
-- @treturn table An array of strings.
function string:split(rx)
  assert(type(rx) == "string", "Expected a pattern string")
  local i = 1
  local parts = {}
  while true do
    local beg, mark = self:find(rx, i)
    if not beg then
      break
    end
    table.insert(parts, self:sub(i, beg-1))
    i = mark+1
  end
  table.insert(parts, self:sub(i, #self))
  return parts
end

--- Check if a string ends with another string
-- @param str The string to check for.
-- @return boolean
function string:endswith(str)
  return self:sub(#self - #str + 1, #self) == str
end

--- Check if a string starts with another string
-- @param str The string to check for.
-- @return boolean
function string:startswith(str)
  return self:sub(1, #str) == str
end

--- Pop a value from the top of a table.
-- @param t
-- @return The top element.
function table.pop(t)
  assert(type(t) == "table", "Expected a table")
  local ret = t[#t]
  t[#t] = nil
  return ret
end

--- Update a table with keys from another table
--
-- Does not perform a deep copy
--
-- @param dst Destination table
-- @param src Source table
-- @return dst
function table.update(dst, src)
  for k, v in pairs(src) do
    dst[k] = v
  end
  return dst
end

--- Get directory name of path
-- @param path File system path.
-- @return The directory of the given file path.
function u.dirname(path)
  assert(type(path) == "string", "Expected a string")
  local parts = path:split("/+")
  repeat
    local top = table.pop(parts)
  until #parts == 0 or top ~= ""
  return table.concat(parts, "/")
end

--- Get file name of path
-- @tparam string path File system path.
-- @treturn string The file name.
function u.basename(path)
  assert(type(path) == "string", "Expected a string")
  local top
  local parts = path:split("/+")
  repeat
    top = table.pop(parts)
  until #parts == 0 or top ~= ""
  return top
end

--- Join paths together.
-- @param paths An array of path strings.
-- @return A single path string.
function u.joinpaths(paths)
  local npaths = {}
  for i, v in ipairs(paths) do
    if v:match("/+") == v then
      npaths[i] = "/"
    else
      npaths[i] = v:gsub("/+$", "")
    end
  end
  return table.concat(npaths, "/")
end

--- Shell escape string
-- @param str The string to be sent into a shell
-- @return The escaped string (with quotes around it.)
function u.shescape(str)
  assert(str)
  str = str:gsub("\"", "\\\"")
  str = str:gsub("%$", "\\$")
  str = str:gsub("`", "\\`")
  return "\"" .. str .. "\""
end

--- Copy a file
-- @param src The source file path
-- @param dst The destination file path
-- @return nil
function u.copyfile(src, dst)
  local src_f = io.open(src, "r")
  if not src_f then
    error("Unable to open file for reading: " .. src)
  end
  local dst_f = io.open(dst, "w")
  if not dst_f then
    src_f:close()
    error("Unable to open file for writing: " .. dst)
  end
  local src_txt = src_f:read("*a")
  dst_f:write(src_txt)
  src_f:close()
  dst_f:close()
end

--- Unzip a gzipped file, returns new file path
--
-- This function requires that gunzip is installed and available
-- in the current $PATH.
--
-- Warning: The original file will not be available after the
-- unzipping. Use the returned file path instead to access the file.
--
-- @param path Path to zipped file, must have .gz file extension
function u.gunzip(path)
  assert(type(path) == "string")
  assert(path:endswith(".gz"))

  local ret_path = path:sub(1, #path - 3)
  if not os.execute("gunzip -f " .. u.shescape(path)) then
    error("Unable to gunzip file at: " .. path)
  end

  return ret_path
end

strict:off()

return u
