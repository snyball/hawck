require "init"

-- Press all combo in a safe manner.
function pressAllKeys()
  local aliases = io.open("/tmp/aliases.txt", "w")
  local special = {
    ["return"] = true,

    help = true,
    down = true,
    control = true,
    scroll = true,
    ["macro"] = true,
    ["delete"] = true,
    ["right_delete"] = true,
    ["break"] = true,
    ["select"] = true,
    ["do"] = true,
    ["next"] = true,
    num = true,
    escape = true,
    right_shift = true,
    last = true,
    right = true,
    insert = true,
    up = true,
    right_control = true,
    pause = true,
    caps = true,
    left = true,
    prior = true,
    compose = true,
    kp = true,
    right_kp = true,
    alt = true,
    right_f = true,
    shift = true,
    altgr = true,
    find = true,
    nul = true,
  }
  for n = 1, 24 do
    special["f" .. n] = true
    special["F" .. n] = true
  end

  kbd:withCleanMods(function ()
      for key, code in pairs(KEYMAP) do
        if type(key) ~= "number" then
          if not special[key:lower()] and not key:find("F") then
            kbd:press(key)
            aliases:write(key .. "\n")
          end
        end
      end

      local sorted_keys = {}
      for u, v in pairs(COMBO_KEYMAP) do
        table.insert(sorted_keys, u)
      end
      table.sort(sorted_keys)

      for i, keyname in ipairs(sorted_keys) do
        local combo = COMBO_KEYMAP[keyname]
        local keyname_l = keyname:lower()
        if not (keyname_l:find("control") or keyname:sub(1,1) == "_" or
                keyname_l:find("console") or special[keyname_l])
        then
          for i = 1, #combo - 1 do
            kbd:down(combo[i])
            -- aliases:write(tostring(KEYMAP[combo[i]]) .. "\n")
          end
          udev:flush()

          kbd:press(combo[#combo])
          aliases:write(keyname .. "\n")

          udev:flush()

          for i = 1, #combo - 1 do
            kbd:up(combo[i])
          end

          udev:flush()
        end
      end
  end)

  aliases:close()
end

HAS_RAN = false
 
match[down + ctrl + shift] = function ()
  if HAS_RAN then
    return
  end
  -- HAS_RAN = true

  pressAllKeys()

  -- local f = io.popen("echo Done >> /tmp/kalias.fifo &")
  -- f:close()
end

