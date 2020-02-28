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
local strict = require "strict"
local kbmap = require "Keymap"
local u = require "utils"
local cfg = require "cfg"

local kbd = {
  keys_held = {},
  map = kbmap.new(cfg.keymap),
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
end

function kbd:getKeysym(key)
  if type(key) ~= "number" then
    return self.map:getKeysym(key)
  end
  return key
end

--- Wrapper around udev:emit, implicitly generates a SYN event
--  after the event emission.
--
-- @param event_type See the Event table for possible values.
-- @param event_code The key code.
-- @param event_value See the KeyMode table for possible values.
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

--- Generate a key up event.
-- @param code The key to release.
function kbd:up(code)
  code = self:getKeysym(code)
  self:emit(Event.KEY, code, KeyMode.UP)
end

--- Generate a key down event.
-- @param code The key to press down.
function kbd:down(code)
  code = self:getKeysym(code)
  self:emit(Event.KEY, code, KeyMode.DOWN)
end

--- Press a key by generating a key up followed by a key down.
-- 
-- @param code The key to press.
function kbd:press(code)
  self:down(code)
  self:up(code)
  udev:flush()
end

function kbd:from(kbd_hid)
  return self.event_kbd == kbd_hid
end

--- Prepare the keyboard by retrieving values from MacroD
function kbd:prepare(ev_value, ev_code, ev_type, kbd_hid)
  self.event_value = math.floor(ev_value)
  self.event_code = math.floor(ev_code)
  self.event_type = math.floor(ev_type)
  self.event_kbd = kbd_hid

  if self.event_type ~= Event.KEY then
    return
  end

  if self.event_value == KeyMode.DOWN then
    self.keys_held[self.event_code] = true
  elseif self.event_value == KeyMode.UP then
    self.keys_held[self.event_code] = nil
  end

  self.has_repeat = false
  self.has_down = self.event_value == KeyMode.DOWN
  self.has_up = self.event_value == KeyMode.UP
end

--- Check if we got a key down event
function kbd:hadKeyDown()
  return self.has_down
end

--- Check if we got a key up event
function kbd:hadKeyUp()
  return self.has_up
end

--- Compare a key with the key that was pressed.
-- @param code The key to check for.
function kbd:hadKey(code)
  code = self:getKeysym(code)
  return self.event_code == code
end

--- Check if a key is held down
-- @param code The key to check for.
function kbd:keyIsDown(code)
  code = self:getKeysym(code)
  return self.keys_held[code]
end

--- Check if a key is up
-- @param code The key to check for.
function kbd:keyIsUp(code)
  return not self:keyIsDown(code)
end

--- Perform actions with a clean set of modifier keys, e.g without any of
--  Control/Shift/Alt/AltGr being held down while a function is running.
--
-- @param f The function to run with clean modifiers.
-- @return nil
function kbd:withCleanMods(f)
  -- Clear all modifiers by sending key-up events for them
  local keys_held = self.keys_held
  self.keys_held = {}
  for code, _ in pairs(keys_held) do
    if self.map:isModifier(code) then
      self:up(code)
    end
  end

  udev:flush()

  local succ, res = pcall(f)

  -- Reset the modifiers by sending key-down events
  for code, _ in pairs(keys_held) do
    if self.map:isModifier(code) then
      self:down(code)
    end
  end

  self.keys_held = keys_held

  udev:flush()
end

--- Press a modifier+key combination.
--
-- The parameters should be a series of keys to hold down,
-- followed by a final key that should be pressed while
-- all the preceding ones are still held.
--
-- This function will call kbd:withCleanMods(f)
function kbd:pressMod(...)
  local keys = {...}

  self:withCleanMods(function ()
      for i = 1, #keys - 1 do
        self:down(keys[i])
      end
      self:press(keys[#keys])
      for i = 1, #keys - 1 do
        self:up(keys[i])
      end
  end)

  udev:flush()
end

--- Echo back the key that was pressed.
function kbd:echo()
  if not self.event_type or not self.event_code or not self.event_value then
    error("No key to echo")
  end

  udev:emit(self.event_type, self.event_code, self.event_value)
end

-- The keyboard state is global for now
local kbd_expose = kbd

setmetatable(kbd_expose, meta)

strict:off()

return kbd_expose
