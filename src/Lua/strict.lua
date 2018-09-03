local strict_meta = {
  __newindex = function (t, name, idx)
    error(("Error: undefined variable, did you mean `local %s'?"):format(name))
  end,

  __index = function (t, name)
    if not rawget(t, name) then
      error(("Undefined variable: %s"):format(name))
    end
    return rawget(t, name)
  end
}

local strict = {}

function strict:on(f)
  if not f then
    self.had_meta = getmetatable(_G) or {}
    setmetatable(_G, strict_meta)
  else
    local meta = getmetatable(_G) or {}
    setmetatable(_G, strict_meta)
    local ret = f()
    setmetatable(_G, meta)
    return ret
  end
end

function strict:off(f)
  if not f then
    setmetatable(_G, self.had_meta)
    self.had_meta = nil
  else
    local meta = getmetatable(_G) or {}
    setmetatable(_G, {})
    local ret = f()
    setmetatable(_G, meta)
    return ret
  end
end

strict:on()

return strict
