--[====================================================================================[
   Work with freedesktop .desktop files
   
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

-- TODO: Change the calling method into the following:
--       app("firefox"):new_window("some-site.com")
--       app("firefox"):run("some-site.com") -- default action

require "utils"
require "match"

local DESKTOP_FILE_CACHE = {}

function readDesktopFile(path)
  if DESKTOP_FILE_CACHE[path] then
    return DESKTOP_FILE_CACHE[path]
  end

  local file = io.open(path)

  if not file then
    error(("Unable to read desktop file: %s"):format(path))
  end

  local key_value_rx = "^([a-zA-Z%[%]]+)=(.*)%s*$"
  local action_rx = "^%[Desktop Action (.*)%]%s*$"
  local entry_rx = "^%[Desktop Entry%]%s*$"
  local actions = {}

  local current_action = {}
  actions[0] = current_action

  for line in file:lines() do
    local is_entry = line:match(entry_rx)
    -- Skip Desktop Entry line
    if not is_entry then
      local action = line:match(action_rx)
      if action then
        current_action = {}
        actions[action] = current_action
      else
        local key, value = line:match(key_value_rx)
        if key then
          current_action[key] = value
        end
      end
    end
  end

  file:close()

  DESKTOP_FILE_CACHE[path] = actions

  return actions
end

AppMethods = {}

function AppMethods:new(arg)
  return self[0](arg)
end

AppMetaMethods = {}

local function escape(str)
  assert(str)
  str = str:gsub("\"", "\\\"")
  str = str:gsub("%$", "\\$")
  return str
end

function AppMetaMethods.__index(t, key)
  assert(key)

  if AppMethods[key] then
    return AppMethods[key]
  end

  if type(key) == "string" then
    key = key:gsub("_", "-")
  end

  local action = t._actions[key]
  if not action or not action["Exec"] then
    error("No such action: " .. key)
  end

  local exec = action["Exec"]

  return LazyF.new(function (self, arg)
      print(arg)
      local cmd
      if not exec:find("%%u") and arg then
        cmd = exec .. " " .. escape(arg)
      else
        cmd = exec:gsub("%%u", escape(arg) or "")
      end
      io.popen(cmd)
  end)
end

function app(name)
  local t = {
    _actions = readDesktopFile(("/usr/share/applications/%s.desktop"):format(name))
  }
  setmetatable(t, AppMetaMethods)
  return t
end

-- app("chromium")[0]("reddit.com")
-- a = app("chromium"):new_window("reddit.com")
-- a()
-- print(a.run)
-- r = a.run
-- s = a:run("reddit.com")()
-- print(s)
