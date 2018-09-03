--[====================================================================================[
   Hawck main library.
   
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

local u = require "utils"

local cfg = dofile(u.expandEnv("$HOME/.local/share/hawck/cfg.lua"))
local kbmap = require "Keymap"

require "match"
require "app"
kbd = require "kbd"
local unpack = table.unpack

FALLTHROUGH = 0x3141592654

-- Keeps track of keys that are requested by the script.
__keys = {}

-- Root match scope
__match = MatchScope.new()
match = __match
_G["__match"] = __match

cont = ConcatF.new(function ()
    return FALLTHROUGH
end)

any = Cond.new(function ()
    return true
end)
always = any
all = any

never = Cond.new(function ()
    return false
end)

noop = function () end
nothing = noop
disable = noop
off = noop

key = function (key_name)
  __keys[key_name] = true
  local key_code = kbd:getKeysym(key_name)
  return LazyCondF.new(function (key_code)
      return kbd:hadKey(key_code)
  end)(key_code)
end

press = LazyF.new(function (key)
    kbd:press(key)
end)

write = LazyF.new(function (text)
  kbd:withCleanMods(function ()
    for p, c in utf8.codes(text) do
      local succ, _ = pcall(function ()
        getEntryFunction(utf8.char(c))()
      end)
      if not succ then
        print("No such key: " .. utf8.char(c))
      end
    end
  end)
end)


function getEntryFunction(sym)
  local succ, code

  succ, code = pcall(u.curry(kbd.map.getKeysym, kbd.map, sym))
  if succ then
    return function ()
      kbd:press(code)
    end
  end

  succ, code = pcall(u.curry(kbd.map.getCombo, kbd.map, sym))
  if succ then
    return function ()
      kbd:pressMod(unpack(code))
    end
  end

  error(("No such symbol: %s"):format(sym))
end

insert = LazyF.new(function (str)
    kbd:withCleanMods(getEntryFunction(str))
end)

replace = LazyF.new(function (key)
    kbd:withCleanMods(function ()
        if kbd:hadKeyDown() then
          kbd:down(key)
        elseif kbd:hadKeyUp() then
          kbd:up(key)
        end
    end)
end)
sub = replace

echo = ConcatF.new(function ()
    kbd:echo()
end)

held = LazyCondF.new(function (key_name)
    return kbd:keyIsDown(key_name)
end)

down = Cond.new(function ()
    return kbd:hadKeyDown()
end)

up = Cond.new(function ()
    return kbd:hadKeyUp()
end)

modkeys = {
  shift_r = "Shift_R",
  shift_l = "Shift",
  ctrl_l = "Control",
  ctrl_r = "Control_R",
  alt = "Alt",
}

for key, sys_key in pairs(modkeys) do
  _G[key] = held(sys_key)
end

ctrl = ctrl_l / ctrl_r
shift = shift_l / shift_r
control_l = ctrl_l
control_r = ctrl_r
control = ctrl

fn_keys = {}
for i = 1, 12 do
  table.insert(fn_keys, "F" .. tostring(i))
end

fn_keycodes = {}
for i, key in pairs(fn_keys) do
  fn_keycodes[kbd:getKeysym(key)] = true
end

function __match.prepare(...)
    kbd:prepare(...)

    -- Don't act on Ctrl+Alt+F(n) keys
    if (ctrl + alt)() and fn_keycodes[kbd.event_code] then
      return false
    end

    return true
end

notify = LazyF.new(function (title, message)
    local p = io.popen(("notify-send -a '%s' -u normal -i hawck -t 3000 '%s'"):format(title, message))
    p:close()
end)

say = LazyF.new(function (message)
    notify("Hawck", message)()
end)

function mode(name, cond)
  local state = true
  local messages = {
    [true] = "%s was enabled.",
    [false] = "%s was disabled."
  }
  return Cond.new(function ()
      if cond() then
        state = not state
        notify("Hawck", messages[state]:format(name))()
      end
      return state
  end)
end

-- __match[prepare] = echo
ProtectedMeta = {
  __index = function (t, name)
    if not rawget(t, name) then
      error(("Undefined variable: %s"):format(name))
    end
    return rawget(t, name)
  end
}

local OMNIPRESENT = {"Control_R",
                     "Control_L",
                     "Control",
                     "Shift_L",
                     "Shift_R",
                     "AltGr",
                     "Alt",
                     "Shift"}

for idx, name in ipairs(OMNIPRESENT) do
  __keys[name] = true
end

function __setup() end

setmetatable(_G, ProtectedMeta)

