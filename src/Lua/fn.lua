__puts_test = {
  [true] = "this",
  ["This is a \"key\""] = "this\n is\n \ta \"value\"",
  a = {
    b = {
      d = {
        d = function () return __puts_test["a"] end --[[ recursive ]],
        fn = load("\x1bLuaS\x00\x19\x93\r\n\x1a\n\x04\b\x04\b\bxV\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00(w@\x01\v@utils.luaG\x01\x00\x00I\x01\x00\x00\x00\x00\x02\b\x00\x00\x00\x06\x00@\x00\a@@\x00\a\x80@\x00\a\xc0@\x00\a\x00A\x00A@\x01\x00$@\x00\x01&\x00\x80\x00\x06\x00\x00\x00\x04\f__puts_test\x04\x02d\x04\x02e\x04\x02f\x04\x03fn\x04\x10YEEEEEE BOIIIII\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\b\x00\x00\x00H\x01\x00\x00H\x01\x00\x00H\x01\x00\x00H\x01\x00\x00H\x01\x00\x00H\x01\x00\x00H\x01\x00\x00I\x01\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x05_ENV"),
      },
      c = function () return __puts_test["a"]["b"]["d"] end --[[ recursive ]],
    },
  },
  self = function () return __puts_test end --[[ recursive ]],
  c = {
    [1] = "d",
    [2] = "e",
    [3] = "f",
    [4] = {
      [1] = 3,
      [2] = 2,
      [3] = 1,
      [4] = 2,
      [5] = 3,
      [6] = 4,
      [7] = 1,
      [8] = 2,
      [9] = 3,
      [10] = 4,
      [11] = 5,
      [12] = 3,
      [13] = {},
      [14] = 1,
      [15] = 2,
      [16] = {},
      [17] = 32,
      [18] = 2,
    },
    [5] = 1,
    [6] = 2,
    [7] = 3,
    [8] = 4,
    [9] = 5,
    [10] = 6,
    [11] = 7,
    [12] = 8,
  },
  d = {
    e = {
      f = {
        z = 2,
        ds = 389,
        fn = load("\x1bLuaS\x00\x19\x93\r\n\x1a\n\x04\b\x04\b\bxV\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00(w@\x01\v@utils.lua:\x01\x00\x00:\x01\x00\x00\x01\x00\x04\x06\x00\x00\x00F\x00@\x00\x81@\x00\x00\xc0\x00\x00\x00\x9d\xc0\x00\x01d@\x00\x01&\x00\x80\x00\x02\x00\x00\x00\x04\x06print\x04\aThis: \x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x06\x00\x00\x00:\x01\x00\x00:\x01\x00\x00:\x01\x00\x00:\x01\x00\x00:\x01\x00\x00:\x01\x00\x00\x01\x00\x00\x00\x02x\x00\x00\x00\x00\x06\x00\x00\x00\x01\x00\x00\x00\x05_ENV"),
      },
    },
  },
  [1337] = 7331,
}
__puts_test["a"]["b"]["d"]["d"] = __puts_test["a"]["b"]["d"]["d"]()
__puts_test["a"]["b"]["c"] = __puts_test["a"]["b"]["c"]()
__puts_test["self"] = __puts_test["self"]()
return __puts_test
