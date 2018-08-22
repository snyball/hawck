--[====================================================================================[
   Pattern matching library.
   
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

local unpack = table.unpack

PatternScopeMeta = {
  __call = function (t)
    for u, patt in ipairs(t.patterns) do
      local result = patt()
      if result == FALLTHROUGH then
        ;
      elseif result then
        return true
      end
    end
    return false
  end,

  __newindex = function (t, pattern, action)
    u.append(t.patterns, Pattern.new(pattern, action))
  end,

  __index = function(t, idx)
    return t.patterns[idx]
  end
}

MatchScope = {
  new = function (fn)
    local scope = {
      patterns = {}
    }
    setmetatable(scope, PatternScopeMeta)
    if fn then
      fn(scope)
    end
    return scope
  end
}

LazyFMeta = {
  __call = function (t, ...)
    return ConcatF.new(u.curry(t.fn, ...))
  end
}

LazyF = {
  new = function (fn)
    assert(fn)
    local t = {fn = fn}
    setmetatable(t, LazyFMeta)
    return t
  end
}

ConcatFMeta = {
  __call = function(t, ...)
    return t.fn(...)
  end,
  __concat = function (this, other)
    t = {}
    assert(this and other)
    function t.fn()
      this()
      return other()
    end
    setmetatable(t, ConcatFMeta)
    return t
  end,
  __mul = function(this, n)
    if type(this) == "number" then
      this, n = n, this
    end
    return ConcatF.new(function ()
        local last
        for i = 1, n do
          last = this()
        end
        return last
    end)
  end
}

ConcatF = {
  new = function (fn)
    assert(fn)
    local t = {fn = fn}
    setmetatable(t, ConcatFMeta)
    return t
  end
}

CondMeta = {
  __call = function (t, ...)
    return t.cond()
  end,

  __add = function (this, other)
    return Cond.new(function ()
        return this() and other()
    end)
  end,

  __div = function (this, other)
    return Cond.new(function ()
        return this() or other()
    end)
  end,

  __unm = function (this)
    return Cond.new(function ()
        return not this()
    end)
  end,

  __le = function (this, other)
    assert(this and other)
    match[this] = other
  end
}

CondMeta.__band = CondMeta.__add
CondMeta.__bnot = CondMeta.__unm
CondMeta.__bor = CondMeta.__div

Cond = {
  new = function (cond)
    local t = {cond = cond}
    setmetatable(t, CondMeta)
    return t
  end
}

LazyCondFMeta = {
  __call = function(t, ...)
    local pre = {...}
    return Cond.new(function ()
        return t.cond(unpack(pre))
    end)
  end
}

LazyCondF = {
  new = function (cond)
    local t = {cond = cond}
    setmetatable(t, LazyCondFMeta)
    return t
  end
}

PatternMeta = {
  __call = function (t)
    if t.pattern() then
      -- If we've got a sub-scope
      if getmetatable(t.action) == PatternScopeMeta then
        return t.action()
      end
      t.action()
      return true
    end
    return false
  end
}

Pattern = {
  new = function (pattern, action)
    local t = {
      pattern = pattern,
      action = action
    }
    setmetatable(t, PatternMeta)
    return t
  end
}

