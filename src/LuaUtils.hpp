/* =====================================================================================
 * Utilitiy functions, macros and templates for binding C++ code to Lua easily
 *
 * Copyright (C) 2018 Jonas Møller (no) <jonasmo441@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */

/** @file LuaUtils.hpp
 *
 * @brief Lua binding utilities.
 */

#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <functional> 
#include <unordered_map>
#include <atomic>

/** FIXME: This library is NOT THREAD SAFE!!!
 *
 * The problem lies with LuaMethod.setState().
 * You have to change the API so that the function
 * returned by LuaMethod.bind() takes the lua_State
 * as an argument.
 *
 * The LuaMethod instance has been set to local, this
 * might work just fine.
 */

/** How to bind classes to Lua using this API:
 *
 * From here on out, when `Module` is written, you should replace it with
 * the name of the class you want to export.
 *
 * At the top of your header file, declare which methods you want to export:
 *
 *     #define Module_lua_methods(M, _) \
 *             M(Module, method_name_01, int(), int(), int()) \
 *             M(Module, method_name_02, float(), std::string())
 *
 * Then, to define the wrapper functions (prototypes):
 *
 *     // Declares prototypes for the functions that will be given to Lua
 *     // later on, these have C linkage.
 *     LUA_DECLARE(Module_lua_methods)
 *
 * Add the following to your class:
 *
 *     class Module : LuaIface<Module> ... {
 *         private:
 *             ...
 *             // Defines `Module::Module_lua_methods` to be a `luaL_Reg`, which
 *             // binds method names to the prototypes defined with `LUA_DECLARE`
 *             LUA_METHOD_COLLECT(Module_lua_methods);
 *         public:
 *             ...
 *             // Extracts methods from your class, so that they can be called like
 *             // this: `Module::extracted_method(module_instance, ...)`
 *             LUA_EXTRACT(Module_lua_methods)
 *     }
 *
 * Then in your implementation file, change your Module::Module initializer
 * so that it intializes LuaIface:
 * 
 *     Module(...) : LuaIface(this, Module_lua_methods) ... {
 *         ...
 *     }
 *
 * And then finally at the bottom of your implementation file:
 * 
 *     LUA_CREATE_BINDINGS(Module_lua_methods)
 *
 * Which will actually create implementations of the functions
 * defined with `LUA_DECLARE`.
 *
 * In order to use this interface for your class, do the following:
 *
 *     // Open up a new Lua state 
 *     lua_State *L = luaL_newstate();
 *     Module *instance = new Module();
 *     // Open the Lua library inside the state with the name "Module"
 *     instance->luaOpen(L, "Module");
 *
 *     // Your class can now be used inside of Lua like this:
 *     luaL_dostring(L, "Module:method_name_01(1, 2, 3)")
 *
 * Runtime argument checking is performed when these functions
 * are called from Lua, errors are in the following format:
 *
 *     Error: expected (userdata, number, number, number) got (userdata, boolean, string, number)
 *
 * Where `userdata` refers to the `Module` instance.
 */

#define LUA_METHOD_NAME(_T, _m) __bind_##_T##_##_m
#define LUA_METHOD_EXTRACT_NAME(_m) __extracted_method_##_m

#define LUA_METHOD_EXTRACT(_T, _method, ...)                            \
        template <class... Atoms>                                       \
        static inline auto LUA_METHOD_EXTRACT_NAME(_method)(Atoms... args) noexcept { \
            return [](_T *self, Atoms... nargs) -> decltype(((_T *) nullptr)->_method(args...)) { \
                return self->_method(nargs...);                         \
            };                                                          \
        }

#define LUA_METHOD_DECLARE(_T, _m, ...)             \
        int LUA_METHOD_NAME(_T, _m)(lua_State *L)

#define LUA_METHOD_BIND(_T, _m, ...)                                    \
        int LUA_METHOD_NAME(_T, _m)(lua_State *L) {                            \
            thread_local static const auto ex_fn = _T::LUA_METHOD_EXTRACT_NAME(_m)(__VA_ARGS__); \
            thread_local static Lua::LuaMethod<_T> bind;                             \
            thread_local static const auto fn = bind.wrap(#_T"::"#_m, ex_fn, (_T *)nullptr, ##__VA_ARGS__); \
            bind.setState(L);                                           \
            return fn();                                                \
        }

#define LUA_DECLARE(_Mmap)                      \
        extern "C" {                            \
            _Mmap(LUA_METHOD_DECLARE, ;);       \
        }
#define LUA_EXTRACT(_Mmap) _Mmap(LUA_METHOD_EXTRACT, )
#define LUA_CREATE_BINDINGS(_Mmap)              \
        extern "C" {                            \
            _Mmap(LUA_METHOD_BIND, )            \
        }

#define _lua_utils_comma ,

#define LUA_METHOD_COLLECT_SINGLE(_T, _m, ...) {#_m , LUA_METHOD_NAME(_T, _m)}

#define LUA_METHOD_COLLECT(_Mmap)                               \
        static constexpr luaL_Reg _Mmap[] = {                   \
            _Mmap(LUA_METHOD_COLLECT_SINGLE, _lua_utils_comma), \
            {NULL, NULL}                                        \
        }

namespace Lua {
    class LuaError : public std::exception {
    private:
        std::string expl;
    public:
        LuaError(std::string expl) : expl(expl) {}

        virtual const char *what() const noexcept {
            return expl.c_str();
        }
    };

    static const std::string lua_type_names[LUA_NUMTAGS] = {
        "nil",           // Lua type: nil
        "boolean",       // Lua type: boolean
        "lightuserdata", // Lua type: lightuserdata
        "number",        // Lua type: number
        "string",        // Lua type: string
        "table",         // Lua type: table
        "function",      // Lua type: function
        "userdata",      // Lua type: userdata
        "thread",        // Lua type: thread
    };

    inline int luaPush(lua_State *L, bool v) noexcept {
        lua_pushboolean(L, v);
        return 1;
    }

    inline int luaPush(lua_State *L, int v) noexcept {
        lua_pushnumber(L, v);
        return 1;
    }

    inline int luaPush(lua_State *L, std::string& v) noexcept {
        lua_pushlstring(L, v.c_str(), v.length());
        return 1;
    }

    inline int luaPush(lua_State *L, const char *c) noexcept {
        lua_pushstring(L, c);
        return 1;
    }

    template <class Any>
    inline int luaPush(lua_State *, Any) noexcept {
        return 0;
    }

    inline bool luaGet(lua_State *L, bool, int idx) noexcept {
        return lua_toboolean(L, idx);
    }

    inline int luaGet(lua_State *L, int, int idx) noexcept {
        return lua_tonumber(L, idx);
    }

    inline std::string luaGet(lua_State *L, std::string, int idx) noexcept {
        return std::string(lua_tostring(L, idx));
    }

    template <class T>
    class LuaMethod {
    private:
        lua_State *L = nullptr;

    public:
        inline void setState(lua_State *L) {
            this->L = L;
        }

        inline T *get() noexcept {
            return (T *) lua_touserdata(L, lua_upvalueindex(1));
        }

        template <class P>
        inline bool checkLuaType(int idx, P *) noexcept {
            // TODO: Add check for type of userdata
            return lua_type(L, idx) == LUA_TUSERDATA;// || lua_type(L, idx) == LUA_TLIGHTUSERDATA;
        }

        inline bool checkLuaType(int idx, int) noexcept {
            return lua_type(L, idx) == LUA_TNUMBER;
        }

        inline bool checkLuaType(int idx, std::string&&) noexcept {
            return lua_type(L, idx) == LUA_TSTRING;
        }

        inline bool checkLuaType(int idx, bool) noexcept {
            return lua_type(L, idx) == LUA_TBOOLEAN;
        }

        inline int luaGetVal(int idx, int) noexcept {
            return lua_tonumber(L, idx);
        }

        inline bool luaGetVal(int idx, bool) noexcept {
            return lua_toboolean(L, idx);
        }

        template <class P>
        inline P* luaGetVal(int idx, P *) noexcept {
            P **ptr = (P **) lua_touserdata(L, idx);
            if (ptr == nullptr) {
                fprintf(stderr, "Received null userdata pointer\n");
                abort();
            }
            return *ptr;
        }

        inline const char *luaGetVal(int idx, const char *) noexcept {
            return lua_tostring(L, idx);
        }

        static inline constexpr int varargLength() noexcept {
            return 0;
        }

        template <class Tp, class... Arg>
        static inline constexpr int varargLength(Tp&&, Arg&&... tail) noexcept {
            return 1 + varargLength(tail...);
        }

        inline bool checkArgs(int) noexcept {
            return true;
        }

        template <class Head, class... Atoms>
        inline bool checkArgs(int idx, Head&& head, Atoms&&... tail) noexcept {
            return checkLuaType(idx, head) and checkArgs(idx+1, tail...);
        }

        /**
         * Finish off recursion in callFromLuaFunction, in this case there
         * are no arguments left to get from the Lua stack, so we just return
         * a function that will finally push the return value onto the stack
         * and return the amount of results that were pushed.
         */
        template <class R, class Fn>
        inline std::function<int()> callFromLuaFunction(int, Fn f) noexcept {
            return [f, this]() -> int {
                // Most (if not all) C++ compilers will complain about a misuse
                // of void if this check isn't in place.
                if constexpr (std::is_void<R>::value) {
                    f();
                    return 0;
                } else
                    return luaPush(L, f());
            };
        }

        /**
         * Recurse downwards creating lambda functions along the way that
         * get values from the Lua stack with the appropriate lua_get* function.
         */
        template <class R, class Fn, class Head, class... Atoms>
        const std::function<int()> callFromLuaFunction(int idx, Fn f, Head head, Atoms... tail) noexcept {
            auto nf = [this, idx, head, f](Atoms... args) -> R {
                return f(luaGetVal(idx, head), args...);
            };
            return callFromLuaFunction<R>(idx + 1, nf, tail...);
        }

        /* switch(Type) { */
        const std::string typeString(std::string  ) noexcept { return "string";  }
        const std::string typeString(const char * ) noexcept { return "string";  }
        const std::string typeString(int          ) noexcept { return "number";  }
        const std::string typeString(float        ) noexcept { return "number";  }
        const std::string typeString(bool         ) noexcept { return "boolean"; }
        template <class P>
        const std::string typeString(P *          ) noexcept { return "userdata"; }
        /* default: */
        template <class Other>
        const std::string typeString(Other        ) noexcept { return "unknown"; }
        /* } */

        const std::string formatArgs() noexcept {
            return "";
        }

        template <class Head>
        const std::string formatArgs(Head&& head) noexcept {
            return typeString(head);
        }

        template <class Head, class... Atoms>
        const std::string formatArgs(Head&& head, Atoms&&... args) noexcept {
            return typeString(head) + ", " + formatArgs(args...);
        }

        const std::string formatArgsLuaH(int idx) noexcept {
            int tnum = lua_type(L, idx);
            if (tnum >= 0 && tnum < LUA_NUMTAGS)
                return lua_type_names[tnum];
            else
                return "unknown";
        }

        const std::string formatArgsLua(int idx) noexcept {
            if (idx == 0) {
                return "";
            } else if (idx == -1) {
                return formatArgsLuaH(idx);
            } else {
                return formatArgsLuaH(idx) + ", " + formatArgsLua(idx+1);
            }
        }

        /**
         * Wrap a function so that it may be called from Lua.
         * The resulting object can be called after setState()
         * has been called. The returned function will then receive
         * all the Lua values from the stack, and push the return
         * values when called. The function returns the amount
         * of values that were pushed onto the Lua stack.
         *
         * Before the call actually happens however, the arguments
         * are checked. If there is a mismatch between the argument
         * types expected by the C++ function and the types given
         * by the Lua call, then an error message in the following
         * form will be raised:
         *   Class::method : expected (type...) got (type...)
         *
         * Note that the function returned from wrap() cannot
         * be called directly from Lua as it will not have C-linkage.
         * You need to wrap this function inside an extern "C"
         * function.
         *
         * See LUA_METHOD_BIND for a usage example of this function.
         */
        template <class Fn, class... Atoms>
        inline auto wrap(std::string fn_name, Fn f, Atoms... atoms) noexcept {
            std::string   errstr  = fn_name + " : expected (" + formatArgs(atoms...) + ")";
            constexpr int idx     = -varargLength(atoms...);
            auto          wrapped = callFromLuaFunction<decltype(f(atoms...))>(idx, f, atoms...);

            return [this, errstr, idx, wrapped, atoms...]() -> int {
                if (!checkArgs(idx, atoms...)) {
                    std::string nerr  = errstr + " got (";
                    nerr             += formatArgsLua(-lua_gettop(L));
                    nerr             += ")";
                    luaL_error(L, nerr.c_str());
                }
                return wrapped();
            };
        }
    };

    // Does not need to be initialized, is simply a magic number used for run-time
    // type-checking in Lua
    static std::atomic<uint64_t> id_incr;

    template <class T>
    struct LuaPtr {
    private:

    public:
        T *ptr;
        uint64_t type_id;

        LuaPtr(T *ptr) {
            type_id   = id_incr++;
            this->ptr = ptr;
        }

        void provide(lua_State *L) const {
            struct LuaPtr *ud = (struct LuaPtr *)lua_newuserdata(L, sizeof(LuaPtr<T>));
            memcpy(ud, this, sizeof(*ud));
        }
    };

    template <class T>
    class LuaIface {
    private:
        const luaL_Reg *regs;
        T              *inst;
        LuaPtr<T>       lua_ptr;

    public:
        LuaIface(T *inst, const luaL_Reg *regs) : lua_ptr(inst) {
            this->regs = regs;
            this->inst = inst;
        }

        virtual ~LuaIface() {}

        virtual void luaOpen(lua_State *L, const char *name) {
            // Provide pointer to Lua
            lua_ptr.provide(L);

            // Metatable for pointer
            lua_newtable(L);

            // Set methods to be indexed
            lua_pushstring(L, "__index");

            // Push method table
            luaL_checkversion(L);
            lua_createtable(L, 0, 32);
            luaL_setfuncs(L, regs, 0);

            lua_settable(L, -3);

            // Set metatable for pointer userdata
            lua_setmetatable(L, -2);

            lua_setglobal(L, name);
        }
    };

    bool isCallable(lua_State *L, int idx);

    class Script {
    private:
        lua_State *L;
        bool enabled = true;

    public:
        std::string src;
        std::string abs_src;

        explicit Script(std::string path);
        ~Script() noexcept;

        lua_State *getL() noexcept;

        template <class T>
        void open(LuaIface<T> *iface, std::string name) {
            iface->luaOpen(L, name.c_str());
        }

        int call_r(int n) noexcept {
            return n;
        }

        template <class A>
        int call_r(int n, A arg) noexcept {
            luaPush(L, arg);
            return n+1;
        }

        template <class A, class... Arg>
        int call_r(int n, A arg, Arg... args) noexcept {
            luaPush(L, arg);
            return call_r(n+1, args...);
        }

        template <class T, class... Arg>
        T call(std::string name, Arg... args) {
            int nargs = call_r(0, args...);
            lua_getglobal(L, name.c_str());
            int nres = 1;
            if constexpr (std::is_void<T>::value) {
                nres = 0;
            }
            if (lua_pcall(L, nargs, nres, 0) != LUA_OK) {
                std::string err(lua_tostring(L, -1));
                lua_pop(L, 1);
                throw LuaError("Lua Error: " + err);
            }
            if constexpr (!std::is_void<T>::value) {
                T ret = luaGet(L, T(), -1);
                lua_pop(L, 1);
                return ret;
            }
        }

        template <class T>
        T get(std::string name);

        template <class T>
        void set(std::string name, T value) {
            if (luaPush(L, value) == 1) {
                lua_setglobal(L, name.c_str());
            }
        }

        void toggle(bool enabled) noexcept;

        inline bool isEnabled() noexcept {
            return enabled;
        }
    };
}
