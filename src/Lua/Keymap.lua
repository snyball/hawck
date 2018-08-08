--[====================================================================================[
   Load Linux keymap files.
   
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

require "utils"

DEFAULT_KEYMAP = require "keymaps/default_linux"

KEYMAP_MODS = {"Shift",
               "AltGr",
               "Control",
               "Alt",
               "ShiftL",
               "ShiftR",
               "CtrlL",
               "CtrlR",
               "CapsShift"}

local KEYMAP_MOD_CODES = nil
KEYMAP = nil
COMBO_KEYMAP = nil

local SYNONYMS = {
  zero = "0",
  one = "1",
  two = "2",
  three = "3",
  four = "4",
  five = "5",
  six = "6",
  seven = "7",
  eight = "8",
  nine = "9",
}

-- Reverse search
for u, v in pairs(DEFAULT_KEYMAP) do
  DEFAULT_KEYMAP[v] = u
end

function readKeymap(path)
  local keymap_line_regex =  "^ *keycode *([0-9]+) *= *([a-zA-Z+]+) *(.*)$"

  local map = {}
  local combos = {}
  local file = io.open(path)

  if not file then
    error(("Unable to read map: %s"):format(path))
  end

  file:close()

  local combo_pre = {}
  for line in io.lines(path) do
    code, name, rest = line:match(keymap_line_regex)
    if code and name and name ~= "nul" then
      if name:sub(1, 1) == "+" then
        name = name:sub(2)
      end
      name = name:lower()
      code = tonumber(code)
      combo_pre_entry = {code, name}
      for v in rest:gmatch("[^%s]+") do
        table.insert(combo_pre_entry, v)
      end
      table.insert(combo_pre, combo_pre_entry)
      if SYNONYMS[name] then
        name = SYNONYMS[name]
      end
      if map[name] then
        map["right_" .. name] = code
      else
        map[code] = name
        map[name] = code
      end
    end
  end

  local mod_codes = {}
  for i, mod in ipairs(KEYMAP_MODS) do
    local mod_code = map[mod:lower()] or 0
    --print(mod:lower(), mod_code)
    table.insert(mod_codes, mod_code)
  end

  local combined_keys = {}
  for i, combos in ipairs(combo_pre) do
    local key_name = combos[2]
    local root_code = combos[1]
    for n = 3, #combos + 2 do
      local sym_name = combos[n]
      local mod_code = mod_codes[n-2]
      if sym_name == nil then
        break
      end
      if sym_name:sub(1, 1) == "+" then
        sym_name = sym_name:sub(2)
      end
      if mod_code ~= 0 then
        combined_keys[sym_name:lower()] = {mod_code, root_code}
      end
    end
  end

  local modkey_lookup = {}
  for i, code in ipairs(mod_codes) do
    if code ~= 0 then
      modkey_lookup[code] = true
    end
  end

  return map, combined_keys, modkey_lookup
end

function setKeymap(path)
  KEYMAP, COMBO_KEYMAP, KEYMAP_MOD_CODES = readKeymap(path)
  -- io.write("KEYMAP = ")
  -- u.puts(KEYMAP)
  -- io.write("COMBO_KEYMAP = ")
  -- u.puts(COMBO_KEYMAP)
end

function getKeysym(a)
  if KEYMAP and KEYMAP[a] then
    return KEYMAP[a]
  end

  if DEFAULT_KEYMAP[a] then
    return DEFAULT_KEYMAP[a]
  end

  error(("No such key: %s"):format(a))
end

function getCombo(a)
  if COMBO_KEYMAP[a] then
    return COMBO_KEYMAP[a]
  end

  error(("No such combo key: %s"):format(a))
end

function isModifier(key)
  local keycode
  if type(key) == "number" then
    keycode = key
  else
    keycode = getKeysym(key)
  end
  return KEYMAP_MOD_CODES[keycode]
end

setKeymap("./keymaps/qwerty/no.map")
