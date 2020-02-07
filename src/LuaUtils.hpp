/* =====================================================================================
 * Utilitiy functions, macros and templates for binding C++ code to Lua easily
 *
 * Copyright (C) 2018 Jonas Møller (no) <jonas.moeller2@protonmail.com>
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

/*
 * FIXME: There are a couple of cases where the library will call abort(), in these
 *        cases this should be logged to the system log rather than just to stdout.
 *
 *        Currently I think syslog is the best option, however that isn't supported
 *        on Windows AFAIK.
 */

#pragma once

#include <functional>
#include <unordered_map>
#include <atomic>
#include <iostream>
#include <sstream>
#include <memory>
#include <cstring>
#include <mutex>
#include <typeinfo>
#include <typeindex>
#include <vector>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
    #include <libgen.h>
}

extern "C" {
    #undef _GNU_SOURCE
    #include <string.h>
    #define _GNU_SOURCE
    #include <execinfo.h>
    #include <stdio.h>
    #include <syslog.h>
}

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

/**
 * Who is this library for, and who is it not for?
 *
 * This library is primarily for developers using mainline Lua 5.3 (i.e not LuaJIT.)
 * It does not (or at least it attempts to) not incur any performance degradation when
 * compared to using the C-API manually.
 *
 * However, if you need very high performance you should not be using the standard C-API
 * at all, you should use LuaJIT and it's C FFI.
 *
 * The caveat with using LuaJIT and it's C FFI is that it is not safe, so in essence
 * this library allows for exposing C++ APIs that may be used safely by non-experts,
 * or if you don't require high-performance but require safety in Lua scripts (which
 * is why we like using them anyway.)
 */

/** How to bind classes to Lua using this API:
 *
 * From here on out, when `Module` is written, you should replace it with
 * the name of the class you want to export.
 *
 * At the top of your header file, declare which methods you want to export:
 *
 *     #define Module_lua_methods(M, _) \ (Note: Multi-line macros are made with '\')
 *             M(Module, method_name_01, int(), int(), int()) _ \
 *             M(Module, method_name_02, int(), int(), int()) _ \ (Note: the _ works as a comma
 *             M(Module, method_name_03, int(), int(), int()) _ \  and is required)
 *             M(Module, method_name_04, float(), LUA_PTR(std::string)) _ \
 *             M(Module, method_name_05, LUA_PTR(MyType), LUA_PTR(char))
 *
 * Note that this wrapper library is designed to work best with functions that
 * take primitive types (which includes pointers to arbitrary classes.)
 * References (&T) are not supported.
 * The system for handling pointer types is type safe, and will perform run-time
 * checks on pointers received from Lua. These run-time checks are very light,
 * they just compare 32-bit type ids.
 *
 * Then, to define the wrapper functions (prototypes):
 *
 *     // Declares prototypes for the functions that will be given to Lua
 *     // later on, these have C linkage. This must be placed ahead of the
 *     // class definition.
 *     LUA_DECLARE(Module_lua_methods)
 *
 * Add the following to your class in the header file (Module.hpp):
 *
 *     class Module : LuaIface<Module> ... {
 *         private:
 *             ...
 *         public:
 *             ...
 *             LUA_CLASS_INIT(Module_lua_methods)
 *     }
 *
 * Then in your implementation file, change your Module::Module initializer
 * so that it intializes LuaIface:
 *
 *     Module(...) : LuaIface(this, Module_lua_methods) ... {
 *         ...
 *     }
 *
 * And then finally at the bottom of your implementation (Module.cpp) file:
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

#define LUA_PTR(_T) ((_T *) nullptr)

#define LUA_CLASS(_T) \
    LUA_DECLARE(_T##_lua_methods) \
    class _T : public LuaIface<_T>

/**
 * Given a class _T and a method _m LUA_METHOD_NAME creates a new name
 * for the extern "C" function that Lua eventually calls.
 *
 * Override this if the default clashes with other extern "C" exports.
 */
#define LUA_METHOD_NAME(_T, _m) __bind_##_T##_##_m
/**
 * Given a method name _m LUA_METHOD_EXTRACT_NAME creates the name for a new class
 * method that when given a pointer to the object followed by the arguments
 * for the _m method, will run object->_m(args...)
 *
 * Override this if the default clashes with other class members.
 */
#define LUA_METHOD_EXTRACT_NAME(_m) __extracted_method_##_m

/**
 * Extract a method such that obj->method(args..) becomes bound_method(obj, args...)
 * which makes it suitable for calling from Lua.
 */
#define LUA_METHOD_EXTRACT(_T, _method, ...) \
        template <class... Atoms> \
        static inline auto LUA_METHOD_EXTRACT_NAME(_method)(Atoms... args) noexcept { \
            return [](_T *self, Atoms... nargs) -> decltype(((_T *) nullptr)->_method(args...)) { \
                return self->_method(nargs...); \
            }; \
        }

/**
 * Pre-declare a method that LUA_METHOD_BIND later defines.
 */
#define LUA_METHOD_DECLARE(_T, _m, ...) \
        int LUA_METHOD_NAME(_T, _m)(lua_State *L)

/**
 * Create a Lua binding for a C++ class method, the function has the
 * type: int(lua_State *)
 */
#define LUA_METHOD_BIND(_T, _m, ...) \
        int LUA_METHOD_NAME(_T, _m)(lua_State *L) { \
            thread_local static const auto ex_fn = _T::LUA_METHOD_EXTRACT_NAME(_m)(__VA_ARGS__); \
            thread_local static Lua::LuaMethod<_T> bind; \
            thread_local static const auto fn = bind.wrap(#_T"::"#_m, ex_fn, (_T *)nullptr, ##__VA_ARGS__); \
            bind.setState(L); \
            return fn(); \
        }

/**
 * Pre-declare the functions that LUA_CREATE_BINDINGS defines.
 */
#define LUA_DECLARE(_Mmap) \
        extern "C" { \
            _Mmap(LUA_METHOD_DECLARE, ;); \
        }

/**
 * Called by LUA_CLASS_INIT
 */
#define LUA_EXTRACT(_Mmap) _Mmap(LUA_METHOD_EXTRACT, )

/**
 * Create the extern "C" bindings that Lua can call.
 */
#define LUA_CREATE_BINDINGS(_Mmap) \
        extern "C" { \
            _Mmap(LUA_METHOD_BIND, ) \
        }

/**
 * Utility for LUA_METHOD_COLLECT
 */
#define _lua_utils_comma ,

/**
 * Used by LUA_METHOD_COLLECT
 */
#define LUA_METHOD_COLLECT_SINGLE(_T, _m, ...) {#_m , LUA_METHOD_NAME(_T, _m)}

/**
 * Called by LUA_CLASS_INIT
 *
 * Collects methods into a luaL_Reg array.
 */
#define LUA_METHOD_COLLECT(_Mmap) \
        static constexpr luaL_Reg _Mmap[] = { \
            _Mmap(LUA_METHOD_COLLECT_SINGLE, _lua_utils_comma), \
            {NULL, NULL} \
        }

/**
 * Put this at the bottom of your class to initialize method extraction
 * for Lua.
 */
#define LUA_CLASS_INIT(_Mmap)                   \
    LUA_METHOD_COLLECT(_Mmap);                  \
    LUA_EXTRACT(_Mmap)

namespace Lua {
    using namespace std::chrono;
    using namespace std::chrono_literals;

    class LuaError : public std::exception {
    private:
        std::string expl;

    public:
        std::vector<lua_Debug> trace;

        explicit LuaError(const std::string& expl,
                          const std::vector<lua_Debug>& trace)
            : expl(expl),
              trace(trace)
        {
            fmtError();
            std::cout << "trace size: " << this->trace.size() << std::endl;
        }

        explicit LuaError(const std::string& expl)
            : expl(expl),
              trace{}
        {
            fmtError();
        }

        /** Lua errors are reported like this:
         *  /long/winding/path/file.lua:<line>: <error message>
         *  We are only interested in this part:
         *  file.lua:<line>: <error message>
         *
         * If your paths have the ':' symbol in them this function will
         * break, but so will many other things, like environment variables
         * that expect ':' to be a safe separator. The script may contain
         * one or more ':' characters without causing any problems.
         */
        inline const char *fmtError() const noexcept {
            const char *ptr = expl.c_str();
            const char *start = ptr;
            for (size_t i = 0; i < expl.size(); i++)
                if (ptr[i] == '/')
                    start = &ptr[i+1];
                else if (ptr[i] == ':')
                    break;
            return start;
        }

        virtual const char *what() const noexcept {
            return fmtError();
        }

        /** Format the Lua error traceback. */
        std::string fmtTraceback() const noexcept {
            std::stringstream err_ss;
            int lv = 0;
            bool in_c;
            for (const auto& ar : trace) {
                if (!strcmp(ar.what, "C")) {
                    in_c = true;
                    continue;
                } else if (in_c) {
                    err_ss << "  [.]: ... C++ ...";
                    err_ss << std::endl;
                    in_c = false;
                }
                err_ss << "  [" << lv++ << "]:";
                err_ss << " func '" << ((ar.name) ? ar.name : "unknown") << "'";
                err_ss << " @ " << basename((char*)ar.short_src);
                err_ss << ":" << ar.linedefined;
                err_ss << std::endl;
            }
            return err_ss.str();
        }

        std::string fmtReport() const noexcept {
            std::stringstream err_ss;
            err_ss << fmtError() << std::endl;
            err_ss << fmtTraceback();
            return err_ss.str();
        }
    };

    inline void checkStack(lua_State *L, int n) noexcept {
        if (!lua_checkstack(L, n)) {
            std::cout << "Unable to allocate enough Lua stack space (FATAL OOM)"
                      << std::endl;
            constexpr int elems = 100;
            void *stack[elems];
            size_t size = backtrace(stack, elems);
            backtrace_symbols_fd(stack + 1, size - 1, fileno(stderr));
            abort();
        }
    }

    template <class T>
    T *newUserdata(lua_State *L) {
        checkStack(L, 1);
        T *ptr = reinterpret_cast<T *>(lua_newuserdata(L, sizeof(T)));
        if (ptr == nullptr) {
            std::cout << "lua_newuserdata returned NULL (FATAL OOM)" << std::endl;
            abort();
        }
        return ptr;
    }

    /**
     * Check if a file name matches ".+\.lua".
     */
    inline bool goodLuaFilename(const std::string& name) {
        return !(
            name.size() < 4 || name[0] == '.' ||
            name.find(".lua") != name.size()-4
        );
    }

    template <class T> struct LuaValue {
        void get(lua_State *, int) {}
    };

    /** Lua integers. */
    template <> struct LuaValue<int> {
        /** Retrieve an integer from the Lua state.
         *
         * @param L Lua state.
         * @param idx The index of the integer on the stack.
         */
        int get(lua_State *L, int idx) {
            if (!lua_isnumber(L, idx))
                throw LuaError("Expected number");
            return lua_tointeger(L, idx);
        }
    };

    /** Lua numbers. */
    template <> struct LuaValue<double> {
        /** Retrieve a number from the Lua state.
         *
         * @param L Lua state.
         * @param idx The index of the number on the stack.
         */
        int get(lua_State *L, int idx) {
            if (!lua_isnumber(L, idx))
                throw LuaError("Expected number");
            return lua_tonumber(L, idx);
        }
    };

    /** Lua strings */
    template <> struct LuaValue<std::string> {
        /** Retrieve a string from the Lua state.
         *
         * @param L Lua state.
         * @param idx The index of the string on the stack.
         */
        std::string get(lua_State *L, int idx) {
            size_t sz;
            const char *s = lua_tolstring(L, idx, &sz);
            return std::string(s, sz);
        }
    };

    /** Lua booleans */
    template <> struct LuaValue<bool> {
        /** Retrieve a boolean from the Lua state.
         *
         * @param L Lua state.
         * @param idx The index of the boolean on the stack.
         */
        bool get(lua_State *L, int idx) {
            if (!lua_isboolean(L, idx))
                throw LuaError("Expected boolean");
            return lua_toboolean(L, idx);
        }
    };

    template <> struct LuaValue<void> {
        void get(lua_State*, int) { }
    };

    /** Lua arrays */
    template <class T> struct LuaValue<std::vector<T>> {
        /** Retrieve an array from the Lua state.
         *
         * @param L Lua state.
         * @param idx The index of the array on the stack.
         */
        std::vector<T> get(lua_State *L, int idx) {
            std::vector<T> vec;
            lua_pushvalue(L, idx);
            if (!lua_istable(L, -1))
                throw LuaError("Expected a table");
            for (int i = 1;; i++) {
                lua_pushinteger(L, i);
                lua_gettable(L, -2);
                if (lua_isnil(L, -1))
                    break;
                vec.push_back(LuaValue<T>().get(L, -1));
                lua_pop(L, 1);
            }
            std::cout << "Got vector: " << std::endl;
            for (const auto& val : vec) {
                std::cout << "  - " << val << std::endl;
            }
            return vec;
        }
    };

    struct UncheckedLuaPtr;

    /**
     * Example usage with UncheckedLuaPtr:
     *   UncheckedLuaPtr ul_ptr;
     *   // ... Retrieve the ul_ptr from a Lua state ...
     *   LuaPtr<MyType> l_ptr;
     *   try {
     *       l_ptr = (LuaPtr<MyType>) ul_ptr;
     *   } catch (const Lua::LuaError& e) {
     *       // Unable to cast ul_ptr to MyType as it had the wrong type.
     *       // Perform reasonable error-handling here
     *       // Depending on your use case you may either abort, or make it
     *       // a soft error.
     *       abort();
     *   }
     *   // You can now know for certain that ptr can be accessed in a
     *   // safe manner.
     *   MyType *ptr = l_ptr.ptr;
     *
     * Note that the name UncheckedLuaPtr is a bit of a misnomer,
     * as the pointer is actually checked and has all the type information.
     * But that type information isn't actually *checked* until you explicitly
     * check it by casting and handling any error.
     */
    template <class T>
    struct LuaPtr {
        T *ptr;

        static constexpr size_t getTypeID() {
            return typeid(T).hash_code();
        }

        explicit LuaPtr(T *ptr) {
            this->ptr = ptr;
        }

        inline void provide(lua_State *L) const;

        operator UncheckedLuaPtr();
    };

    /**
     * Essentially a (void *) pointer that can be safely cast back
     * to a Lua pointer.
     *
     * When casting, a LuaError will be raised if the type cannot
     * be cast. This is a runtime check (I don't really see any
     * other way to do it securely.)
     *
     * FIXME: Currently this does not do perform memory management.
     * TODO: Install a __gc callback that gets called when the pointer
     * goes out of Lua scope. This is a slightly complex problem however,
     * the destructor for UncheckedLuaPointer should in this case decrement
     * an atomic reference counter. The __gc callback should do the same,
     * which means it needs a light_userdata upvalue.
     * However, the UncheckedLuaPtr only contains a (void *) pointer to
     * the actual data, which means that we don't have access to the
     * destructor function. This will have to be looked into further,
     * and will have to be implemented with LuaPtr<T> i honestly
     * don't see any other way (without invoking nasal demons.)
     */
    struct UncheckedLuaPtr {
        void *ptr = nullptr;
        size_t type_id = -1;

        explicit inline UncheckedLuaPtr() {}

        template <class T>
        explicit inline UncheckedLuaPtr(T *ptr) {
            this->ptr = (void *) ptr;
            type_id = LuaPtr<T>::getTypeID();
        }

        explicit inline UncheckedLuaPtr(lua_State *L, int idx) {
            auto buf = reinterpret_cast<UncheckedLuaPtr*>(lua_touserdata(L, idx));
            ptr = buf->ptr;
            type_id = buf->type_id;
        }

        inline void provide(lua_State *L) {
            checkStack(L, 1);
            auto *ud =
                reinterpret_cast<UncheckedLuaPtr *>(
                    lua_newuserdata(L, sizeof(UncheckedLuaPtr)));
            if (ud == nullptr) {
                std::cout << "lua_newuserdata returned NULL (FATAL OOM)" << std::endl;
                abort();
            }
            ud->ptr = ptr;
            ud->type_id = LuaPtr<void>::getTypeID();
        }

        template <class T>
        operator LuaPtr<T>() {
            std::stringstream ss;
            if (ptr == nullptr) {
                ss << "Attempted to cast null pointer to " << typeid(T).name();
                throw LuaError(ss.str());
            }
            if (type_id != LuaPtr<T>::getTypeID()) {
                ss << "Attempted to cast (tid=" << type_id << ") to " << typeid(T).name();
                throw LuaError(ss.str());
            }
            return LuaPtr<T>(reinterpret_cast<T*>(ptr));
        }
    };

    template <class T> struct LuaValue<T*> {
        T *get(lua_State *L, int idx) {
            if (!lua_isuserdata(L, idx))
                throw LuaError("Expected userdata");
            UncheckedLuaPtr ulptr(L, idx);
            LuaPtr<T> lptr = (LuaPtr<T>) ulptr;
            return lptr.ptr;
        }
    };

    template <class T>
    LuaPtr<T>::operator UncheckedLuaPtr() {
        UncheckedLuaPtr ptr;
        ptr.ptr = this->ptr;
        ptr.type_id = LuaPtr<T>::getTypeID();
        return ptr;
    }

    template <class T>
    inline void LuaPtr<T>::provide(lua_State *L) const {
        checkStack(L, 1);
        UncheckedLuaPtr *ud =
            (struct UncheckedLuaPtr *) lua_newuserdata(L, sizeof(UncheckedLuaPtr));
        if (ud == nullptr) {
            std::cout << "lua_newuserdata returned NULL (FATAL OOM)" << std::endl;
            abort();
        }
        ud->ptr = ptr;
        ud->type_id = LuaPtr<T>::getTypeID();
    }


    extern "C" int hwk_lua_error_handler_callback(lua_State *L) noexcept;

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

    inline int luaPush(lua_State *L, const void *ptr) noexcept {
        if (ptr == nullptr)
            lua_pushnil(L);
        else {
            UncheckedLuaPtr ul_ptr(ptr);
            ul_ptr.provide(L);
        }
        return 1;
    }

    template <class T>
    class LuaIface;

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

    /** Tools for binding a C++ function to Lua.
     *
     * This is gathered together in a class mostly to make management of the lua_State
     * dead simple, it's always available as the L member variable.
     *
     * @param T Return type of the wrapped function.
     *
     * XXX: Using any methods of LuaMethod without calling setState first invokes undefined
     *      behaviour, you should not assume that the Lua state stored inside a LuaMethod
     *      still exists, or that the current thread has exclusive access to it (unless
     *      you Know What You're Doing™.)
     *      The safest method to use is to always call setState before doing anything, and
     *      making sure you've handled all the required synchronization (which is outside
     *      the scope of the LuaMethod class.)
     *
     * TODO: The currying method can be replaced with std::apply from C++17
     *       Resource: <https://en.cppreference.com/w/cpp/utility/apply>
     */
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

        inline bool checkLuaType(int& idx, std::string) noexcept {
            return lua_type(L, idx) == LUA_TSTRING;
        }

        inline bool checkLuaType(int idx, bool) noexcept {
            return lua_type(L, idx) == LUA_TBOOLEAN;
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

        /** Finish off recursion in callFromLuaFunction, in this case there
         * are no arguments left to get from the Lua stack, so we just return
         * a function that will finally push the return value onto the stack
         * and return the amount of results that were pushed.
         */
        template <class R, class Fn>
        inline std::function<int()> callFromLuaFunction(int, Fn f) noexcept {
            if constexpr (std::is_void<R>::value) {
                return [f]() -> int {
                            f();
                            return 0;
                        };
            } else {
                return [f, this]() -> int {
                           return luaPush(L, f());
                       };
            }
        }

        /** Recurse downwards creating lambda functions along the way that
         *  get values from the Lua stack with the appropriate lua_get* function.
         */
        template <class R, class Fn, class Head, class... Atoms>
        const std::function<int()>
        callFromLuaFunction(int idx, Fn f, Head head, Atoms... tail) noexcept {
            auto nf = [this, idx, head, f](Atoms... args) -> R {
                return f(LuaValue<Head>().get(L, idx), args...);
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
        const std::string typeString(P *          ) noexcept {
            if constexpr (std::is_base_of<LuaIface<P>, P>::value)
                return typeid(P).name();
            return "userdata";
        }
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

            // TODO:
            if (tnum == LUA_TUSERDATA) {}

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

        /** Wrap a function so that it may be called from Lua.
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
        inline auto wrap(const std::string& fn_name, Fn f, Atoms... atoms) noexcept {
            std::string   errstr  = fn_name + " : expected (" + formatArgs(atoms...) + ")";
            constexpr int idx     = -varargLength(atoms...);
            auto          wrapped = callFromLuaFunction<decltype(f(atoms...))>(idx, f, atoms...);

            return [this, errstr, wrapped, atoms...]() -> int {
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

    /**
     * Lua interfaces are classes that can be opened as objects
     * inside Lua states. By deriving from this class and using the
     * LUA_DECLARE/LUA_CLASS_INIT/LUA_CREATE_BINDINGS macros
     * you can expose a C++ instance to Lua.
     *
     * Note: This class is for objects that you don't want the Lua
     * GC to manage. I.e if you pass a LuaIface into a Lua state the
     * memory should live for as long as the Lua state lives.
     *
     * See GC for objects that can be managed by the Lua GC.
     */
    template <class T>
    class LuaIface {
    private:
        const luaL_Reg *regs;
        T              *inst;
        LuaPtr<T>       lua_ptr;
        const char     *type_name;

    public:

        LuaIface(T *inst, const luaL_Reg *regs) : lua_ptr(inst) {
            type_name = typeid(T).name();
            this->regs = regs;
            this->inst = inst;
        }

        virtual ~LuaIface() {}

        virtual void luaPush(lua_State *L) {
            checkStack(L, 4);

            // Provide pointer to Lua
            lua_ptr.provide(L);

            // Metatable for pointer
            lua_newtable(L);

            // Set methods to be indexed
            lua_pushstring(L, "__index");

            // Push method table
            // luaL_checkversion(L);
            lua_createtable(L, 0, 32);
            luaL_setfuncs(L, regs, 0);

            lua_settable(L, -3);

            // Set metatable for pointer userdata
            lua_setmetatable(L, -2);
        }

        virtual void luaOpen(lua_State *L, const char *name) {
            luaPush(L);
            lua_setglobal(L, name);
        }
    };

    extern "C" int hwk_lua_object_destructor_cb(lua_State *L) noexcept;

    /**
     * On the C++ side it works like a shared_ptr, except that it can also
     * be passed onto Lua states, and allows for the Lua __gc metamethod
     * to decrement the reference count.
     *
     * So, if you want Lua to delete your objects when you're done with
     * them you should do the following:
     *
     *   main.cpp:
     *     Script mkScript() {
     *       // Here, MyObject inherits from LuaIface<MyObject>
     *       GC obj(new MyObject());
     *       Script script;
     *       script.from("main.lua");
     *       script.set("my_object", &obj);
     *       return script;
     *       // GC goes out of scope, now Lua is responsible for
     *       // the memory allocated for MyObject.
     *     }
     *
     *     int main() {
     *       Script script = mkScript();
     *       script.call("main");
     *       script.call("do_full_gc");
     *     }
     *
     *   main.lua:
     *     function main()
     *       my_object:doStuff("with this")
     *       my_object = nil
     *       -- Destructor for MyObject /could/ be called around here.
     *       print("Done doing stuff")
     *     end
     *
     *     function do_full_gc()
     *       -- Destructor for MyObject is definitely called here if it hasn't
     *       -- been called already.
     *       collectgarbage("collect")
     *     end
     */
    template <class T>
    class GC {
        static_assert(std::is_base_of<LuaIface<T>, T>::value,
                      "When instantiating GC<T>, T must derive from LuaIface<T>");

    protected:
        LuaIface<T> *iface;
        std::atomic<int> *rc;

    public:
        inline GC(LuaIface<T> *iface) : iface(iface),
                                        rc(new std::atomic<int>(1))
        {}

        inline GC(const GC<T>& other) : iface(other.iface),
                                        rc(other.rc)
        { ++*rc; }

        virtual ~GC()
        {
            std::cout << "[from C++] RC: " << *rc - 1 << std::endl;
            if (--*rc <= 0) {
                std::cout <<
                "[from C++] Destroying GC userdata and reference-counter." <<
                std::endl;
                delete iface;
                delete rc;
            }
        }

        /**
         * This method exists solely so that the hwk_lua_object_destructor_cb function "knows"
         * which destructor to call for T. It otherwise wouldn't be able to, because it's not
         * generic over any T.
         */
        static void destroy(void *ptr) {
            auto obj = reinterpret_cast<LuaIface<T> *>(ptr);
            delete obj;
        }

        virtual void luaPush(lua_State *L) {
            ++*rc;

            checkStack(L, 6);

            iface->luaPush(L);

            // Retrieve the metatable again and add the __gc metamethod.
            lua_getmetatable(L, -1);
            lua_pushstring(L, "__gc");
            // Push upvalues, then push the static destructor function.
            *newUserdata<std::atomic<int> *>(L) = rc;
            *newUserdata<void (*)(void*)>(L) = &GC<T>::destroy;
            lua_pushcclosure(L, hwk_lua_object_destructor_cb, 2);
            lua_settable(L, -3);

            lua_setmetatable(L, -2);
        }

        virtual void luaOpen(lua_State *L, const char *name) {
            luaPush(L);
            lua_setglobal(L, name);
        }

        inline T *operator->() {
            return reinterpret_cast<T *>(iface);
        }
    };

    template <class T>
    inline int luaPush(lua_State *L, GC<T> ptr) noexcept {
        ptr.luaPush(L);
        return 1;
    }

    template <class T>
    inline int luaPush(lua_State *L, T *ptr) noexcept {
        if constexpr (std::is_base_of<LuaIface<T>, T>::value) {
            ptr->luaPush(L);
        } else {
            UncheckedLuaPtr lptr(ptr);
            lptr.provide(L);
        }
        return 0;
    }

    bool isCallable(lua_State *L, int idx);

    template <int num> constexpr int countT() {
        return num;
    }
    template <int num, class, class... T> constexpr int countT() {
        return countT<num+1, T...>();
    }
    template <class... T> constexpr int countT() {
        return countT<0, T...>();
    }

    class Hook {
        using Callback = void (*)(lua_State *L, lua_Debug *ar);

    protected:
        Callback cb;
        lua_State *L;

    public:
        inline Hook(lua_State *L, Callback cb, int num_instructions) : cb(cb), L(L) {
            lua_sethook(L, cb, LUA_MASKCOUNT, num_instructions);
        }

        virtual ~Hook() {
            lua_sethook(L, NULL, 0, 0);
        }
    };

    extern "C" void lua_timeout_hook(lua_State *L, lua_Debug *ar) noexcept;

    class TimeoutHook : public Hook {
    public:
        static std::mutex script_end_times_mtx;
        static std::unordered_map<uintptr_t, milliseconds> script_end_times;
        static const int NUM_INST = 16384;

        TimeoutHook(lua_State *L, milliseconds time);

        virtual ~TimeoutHook();
    };

    /** C++ bindings to make the Lua API easier to deal with.
     */
    class Script {
    private:
        lua_State *L;
        bool enabled = true;
        long max_run_time_ms = 2000;
        int max_instructions = 16384;
        milliseconds SCRIPT_TIMEOUT = 2000ms;

    public:
        std::string src;
        std::string abs_src;

        /** Initialize a Lua state and load a script.
         *
         * @param path Path to the Lua script.
         */
        explicit Script(std::string path);

        /** Initialize a Lua state. */
        Script();

        /** Destroy a Lua state. */
        virtual ~Script() noexcept;

        /** Get the raw Lua state. */
        lua_State *getL() noexcept;

        /** Load a script into the Lua state.
         *
         * @param path Path to the Lua script.
         */
        virtual void from(const std::string& path);

        /** Open a Lua interface in the Script. */
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

        /** Utility function for Script::call in order to push arguments
         *  onto the Lua stack. */
        template <class A, class... Arg>
        int call_r(int n, A arg, Arg... args) noexcept {
            luaPush(L, arg);
            return call_r(n+1, args...);
        }

        template <int idx>
        std::tuple<> ret_r() noexcept {
            return std::make_tuple();
        }

        /** Utility function for Script::call in order to retrieve returned
         *  values as a tuple. */
        template <int idx, class A, class... Arg>
        std::tuple<A, Arg...> ret_r() noexcept {
            return std::tuple_cat(
                std::tuple<A>(LuaValue<A>().get(L, idx)),
                ret_r<idx-1, Arg...>()
            );
        }

        /** Call a global Lua function and return the result(s).
         *
         * @tparam T... The list of return types.
         * @tparam Arg... The list of function argument types.
         * @param name Name of global Lua function.
         * @param args Any number of arguments that the Lua function requires.
         */
        template <class... T, class... Arg>
        std::tuple<T...> call(std::string name, Arg... args) {
            TimeoutHook hook(L, SCRIPT_TIMEOUT);

            constexpr int nres = countT<T...>();
            constexpr int nargs = countT<Arg...>();
            checkStack(L, nargs + 1);

            lua_pushcfunction(L, hwk_lua_error_handler_callback);
            lua_getglobal(L, name.c_str());
            if (!isCallable(L, -1))
                throw LuaError("Unable to retrieve " + name +
                                " function from Lua state");
            call_r(0, args...);

            // -n-nargs is the position of hwk_lua_error_handler_callback
            // on the Lua stack.
            if (lua_pcall(L, nargs, nres, -2-nargs) != LUA_OK) {
                // Here be dragons
                auto exc = std::unique_ptr<LuaError>(
                    static_cast<LuaError*>(lua_touserdata(L, -1)));
                if (!exc)
                    throw LuaError("Unknown error");
                LuaError err = *exc;
                throw err;
            }

            return ret_r<-nres, T...>();
        }

        /** Retrieve a global Lua value. */
        template <class T>
        T get(std::string name);

        /** Set a global Lua value.
         *
         * @param name Name of global variable.
         * @param value Value to set the variable to.
         */
        template <class T>
        void set(std::string name, T value) {
            luaPush(L, value);
            lua_setglobal(L, name.c_str());
        }

        inline bool isEnabled() noexcept {
            return enabled;
        }

        inline void setEnabled(bool enabled) noexcept {
            this->enabled = enabled;
        }

        /** Run Lua code inside the Lua state.
         *
         * @param str Lua code.
         */
        void exec(const std::string& str);

        /** Reset the Lua state, will destroy all data currently
         *  held within it */
        void reset();

        /** Reload from the file that the Lua state was initially
         *  initialized with. */
        void reload();

    };
}
