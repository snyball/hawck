--[====================================================================================[
   Interact with a virtual keyboard.
   
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

require "match"
require "Keymap"
require "utils"

local kbd = {
  keys_held = {}
}

local meta = {}

-- Values directly from <linux/input-event-codes.h>
local Event = {
  SYN       = 0x00,
  KEY       = 0x01,
  REL       = 0x02,
  ABS       = 0x03,
  MSC       = 0x04,
  SW        = 0x05,
  LED       = 0x11,
  SND       = 0x12,
  REP       = 0x14,
  FF        = 0x15,
  PWR       = 0x16,
  FF_STATUS = 0x17,
  MAX       = 0x1f,
}

local KeyMode = {
  UP     = 0,
  DOWN   = 1,
  REPEAT = 2,
}

function kbd:init(keymap)
  setKeymap(keymap)
end

function kbd.getKeysym(key)
  local keysym 
  if type(key) == "number" then
    return key
  else
    return getKeysym(key)
  end
end

function kbd:emit(event_type, event_code, event_value)
  udev:emit(event_type, event_code, event_value)
  udev:emit(Event.SYN, 0, 0)
end

function kbd:pressN(event_code)
  for i = 1, 100 do
    udev:emit(Event.KEY, event_code, KeyMode.DOWN)
    udev:emit(Event.SYN, 0, 0)
    udev:emit(Event.KEY, event_code, KeyMode.UP)
    udev:emit(Event.SYN, 0, 0)
  end
  udev:flush()
end

function kbd:up(code)
  code = kbd.getKeysym(code)
  self:emit(Event.KEY, code, KeyMode.UP)
end

function kbd:down(code)
  code = kbd.getKeysym(code)
  self:emit(Event.KEY, code, KeyMode.DOWN)
end

function kbd:press(code)
  self:down(code)
  self:up(code)
  udev:flush()
end

function kbd:prepare()
  self.event_value = __event_value_num
  self.event_code = __event_code
  self.event_type = __event_type_num

  if __event_value == "DOWN" then
    self.keys_held[__event_code] = true
  elseif __event_value == "UP" then
    self.keys_held[__event_code] = nil
  end

  self.has_down = false
  self.has_up = false
  self.has_repeat = false

  if __event_type == "KEY" then
    self.has_down = __event_value == "DOWN"
    self.has_up = __event_value == "UP"
  end
end

function kbd:hadKeyDown()
  return self.has_down
end

function kbd:hadKeyUp()
  return self.has_up
end

function kbd:hadKey(code)
  code = self.getKeysym(code)
  return self.event_code == code
end

function kbd:keyIsDown(code)
  code = self.getKeysym(code)
  return self.keys_held[code]
end

function kbd:keyIsUp(code)
  return not self:keyIsDown(code)
end

function kbd:withCleanMods(f)
  -- Clear all modifiers by sending key-up events for them
  for code, _ in pairs(self.keys_held) do
    if isModifier(code) then
      self:up(code)
    end
  end

  udev:flush()

  f()

  -- Reset the modifiers by sending key-down events
  for code, _ in pairs(self.keys_held) do
    if isModifier(code) then
      self:down(code)
    end
  end

  udev:flush()
end

function kbd:pressMod(...)
  local keys = {...}

  self:withCleanMods(function ()
      for i = 1, #keys - 1 do
        self:down(keys[i])
      end
      udev:flush()

      self:press(keys[#keys])

      udev:flush()

      for i = 1, #keys - 1 do
        self:up(keys[i])
      end

      udev:flush()
  end)
end

function kbd:echo()
  if not self.event_type or not self.event_code or not self.event_value then
    error("No key to echo")
  end

  udev:emit(self.event_type, self.event_code, self.event_value)
end

-- The keyboard state is global for now
local kbd_expose = kbd

setmetatable(kbd_expose, meta)

return kbd_expose
