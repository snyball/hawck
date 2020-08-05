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

local strict = require "strict"
local u = require "utils"
local ALIASES = require "keymaps/aliases"
local kbmap = {}
local KEYMAP_MODS = {"Shift",
                     "AltGr",
                     "Control",
                     "Alt",
                     "Shift_L",
                     "Shift_R",
                     "Control_L",
                     "Control_R",
                     "CapsShift"}
local SYNONYMS = {zero = "0",
                  one = "1",
                  two = "2",
                  three = "3",
                  four = "4",
                  five = "5",
                  six = "6",
                  seven = "7",
                  eight = "8",
                  nine = "9"}
local readLinuxKBMap

--- Call readLinuxKBMap on an include file
-- @param path Path to the original file.
-- @param include_name Name of the included file.
-- @return Result of readLinuxKBMap on the include file.
local function getInclude(path, include_name)
  -- Walk up to find the two include directories.
  -- I.e for `/usr/share/kbd/keymaps/i386/qwerty/no.map` this would be
  --   - /usr/share/kbd/keymaps/i386
  --   - /usr/share/kbd/keymaps
  local paths = {}
  local path_parts = u.dirname(path):split("/+")
  table.pop(path_parts)
  table.insert(paths, table.concat(path_parts, "/"))
  table.pop(path_parts)
  table.insert(paths, table.concat(path_parts, "/"))

  -- If the include ends in .map, we can assume it is gzipped
  if include_name:endswith(".map") or include_name:endswith(".kmap") then
    include_name = include_name .. ".gz"
  -- If the include has some other file extension, keep it
  elseif include_name:match("%.(.*)$") then
      -- Do nothing.
  -- Otherwise it is a .inc file.
  else
    include_name = include_name .. ".inc"
  end

  for i, inc_dir in ipairs(paths) do
    local include_path = u.joinpaths({inc_dir, "include", include_name})
    -- If the original file cannot be found, just try appending .gz
    if not io.open(include_path) then
      include_path = include_path .. ".gz"
    end
    -- Attempt to load the map
    local succ, submap, subcombi, submods = pcall(readLinuxKBMap, include_path)
    -- If successful, return it
    if succ then
      return submap, subcombi, submods
    end
  end
  error("No such include: " .. include_name)
end

--- Read a Linux keymap .map.gz or .inc file, this will also pull
--  in their dependencies
--
-- @param path Path to the Linux keymap file, should be in:
--             /usr/share/kbd/keymaps/<machine>/<layout type>/<language>[-<extension>].map.gz
--             For Norwegian on i386, this would be
--             /usr/share/kbd/keymaps/i386/qwerty/no-latin1.map.gz
function readLinuxKBMap(path)
  local include_line_rx = "^[\t ]*include \"(.*)\""
  local keymap_line_rx =  "^[\t ]*keycode *([0-9]+) *= *([0-9a-zA-Z+]+) *(.*)$"

  local orig_path = path
  local has_gz = path:endswith(".gz")
  -- Got to unzip it
  if has_gz then
    local dst = u.joinpaths({"/tmp", u.basename(os.tmpname()) .. "_" .. u.basename(path)})
    u.copyfile(path, dst)
    path = u.gunzip(dst)
  end

  local map = {}
  local combos = {}
  local file = io.open(path)

  if not file then
    if has_gz then os.remove(path) end
    error(("Unable to read map: %s"):format(orig_path))
  end

  file:close()

  local combo_pre = {}
  local mod_codes = {}
  local combined_keys = {}

  for line in io.lines(path) do
    -- Skip comments
    line = line:split("#+")[1]:split("!+")[1]

    local code, name, rest = line:match(keymap_line_rx)
    local include_path = line:match(include_line_rx)

    -- Does not start with "keycode", use alternative method for lines
    -- where modifier key codes are explicitly written down.
    if not code then
      local modifiers = {}
      for key in line:gmatch("([a-z]+)") do
        -- This terminates the modifier list.
        if key == "keycode" then
          break
        end
        table.insert(modifiers, key)
      end
      local alt_code, alt_name = line:match(".*keycode *([0-9]+) *= *([a-zA-Z0-9+]+)")
      -- We only care about the plain ones for now.
      if modifiers[1] == "plain" then
        code, name, rest = alt_code, alt_name, ""
      end
    end

    if code and name and name ~= "nul" then
      -- Some keys are formatted as '+A' where A is a literal character,
      -- we just remove this plus.
      local is_letter = false
      if name:sub(1, 1) == "+" then
        is_letter = true
        name = name:sub(2)
      end
      code = tonumber(code)
      combo_pre_entry = {code, name}
      -- Keymaps are layed out like this:
      --   keycode <code> = <clean> <with Shift> <with AltGr> <with Control> <with Alt>
      -- We don't yet know the keycodes of the modifiers, so we save the extra
      -- modifier+key mappings for later.
      for v in rest:gmatch("[^%s]+") do
        table.insert(combo_pre_entry, v)
      end
      -- Hack for broken .map files that don't include a shift+letter upper case variant.
      -- Will obviously not do the correct thing for all alphabets/scripts.
      if is_letter and #combo_pre_entry == 2 then
        table.insert(combo_pre_entry, name:sub(1,1):upper() .. name:sub(2))
      end
      table.insert(combo_pre, combo_pre_entry)
      -- Swap out names with synonyms if available.
      if SYNONYMS[name] then
        name = SYNONYMS[name]
      end
      -- A second instance of a key means it is the right version of that key, i.e
      -- Control and Control_R
      if map[name] then
        map[name .. "_R"] = code
      else
        map[code] = name
        map[name] = code
      end
    elseif include_path then
      local submap, subcombi, submods = getInclude(orig_path, include_path)
      table.update(map, submap)
      table.update(combined_keys, subcombi)
      table.update(mod_codes, submods)
    end
  end

  for i, mod in ipairs(KEYMAP_MODS) do
    local mod_code = map[mod] or 0
    table.insert(mod_codes, mod_code)
  end

  -- Assemble modifier key combos now that we know the key names for
  -- all key codes.
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
      if mod_code ~= 0 and not combined_keys[sym_name] then
        combined_keys[sym_name] = {mod_code, root_code}
      end
    end
  end

  local modkey_lookup = {}
  for i, code in ipairs(mod_codes) do
    if code ~= 0 then
      modkey_lookup[code] = true
    end
  end

  if has_gz then os.remove(path) end
  return map, combined_keys, modkey_lookup
end

local kbmap = {}

local kbmap_meta = {
  __index = kbmap
}

--- Get all installed key maps.
--
-- Requires the `find` command to be present on the system.
--
-- @return Table of key map file paths indexed by language.
function kbmap.getall()
  local kbmap_paths = {
    "/usr/share/kbd/keymaps",
    "/usr/share/keymaps",
    "/lib/kbd/keymaps/legacy/",
    "/lib/kbd/keymaps/xkb/",
  }
  -- Find the path to the keymaps directory.
  local kbmap_path = nil
  for i, v in ipairs(kbmap_paths) do
    if os.execute(("[ -d %s ]"):format(u.shescape(v))) then
      kbmap_path = v
      break
    end
  end
  if not kbmap_path then
    error("Unable to find keymaps directory.")
  end

  local p = io.popen(("find %s -name '*map.gz'"):format(u.shescape(kbmap_path)))
  local keymap_rx = ".*/([a-z0-9]+)/([a-z0-9]+)/([a-z0-9-]+)%.k?map%.gz"
  local keymaps = {}
  for line in p:lines() do
    local machine, layout, lang = line:match(keymap_rx)
    if machine then
      keymaps[lang] = line
    end
  end
  p:close()
  return keymaps
end

--- Create a new kbmap from a Linux keymap file
-- @param lang The key map language.
function kbmap.new(lang)
  assert(lang)
  local maps = kbmap.getall()
  if not maps[lang] then
    error("No such keymap: " .. lang)
  end
  local keymap, combo_map, mod_codes = readLinuxKBMap(maps[lang])
  local map = {
    keymap = keymap,
    combo_map = combo_map,
    mod_codes = mod_codes,
  }
  setmetatable(map, kbmap_meta)
  return map
end

--- Get a combo key, i.e a key that consists of a modifier+key combo.
-- @param key Key name.
function kbmap:getCombo(key)
  if ALIASES and ALIASES[key] then
    key = ALIASES[key]
  end
  return self.combo_map[key] or error(("No such combo key: %s"):format(key))
end

--- Get a key code from a key name.
-- @param key Key name.
function kbmap:getKeysym(key)
  if ALIASES and ALIASES[key] then
    key = ALIASES[key]
  end
  return self.keymap[key] or error(("No such key: %s"):format(key))
end

--- Check if a key is a modifier, i.e one of Control/Control_R/Shift/Shift_R/Alt/AltGr
-- @param key Key code/symbol.
function kbmap:isModifier(key)
  local keycode = key
  if type(key) ~= "number" then
    keycode = getKeysym(key)
  end
  return self.mod_codes[keycode]
end

strict:off()

return kbmap
