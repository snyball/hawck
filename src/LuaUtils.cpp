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

#include <memory>

extern "C" {
    #include <unistd.h>
}

#include "LuaUtils.hpp"
#include "utils.hpp"

using namespace std;
using namespace std::chrono;

namespace Lua {
    /**
     * Implementation of isCallable
     */
    static bool isCallableHelper(lua_State *L, int idx, int depth) {
        static const int soft_max_depth = 32;

        if (!lua_checkstack(L, 2))
            luaL_error(L, "Maximum recursion depth exceeded (%s)", __func__);
        else if (depth > soft_max_depth)
            luaL_error(L,
                       "Unable to determine if function is callable,"   \
                       "__call metamethods nested beyond %d layers (%s)",
                       soft_max_depth, __func__);

        if (lua_isfunction(L, idx))
            return true;
        lua_getmetatable(L, idx);
        if (!lua_istable(L, -1)) {
            lua_pop(L, 1);
            return false;
        }
        lua_getfield(L, -1, "__call");

        // Nested __call metamethods are possible
        bool is_callable = isCallableHelper(L, -1, depth+1);
        lua_pop(L, 2);

        return is_callable;
    }

    /** Figure out if a Lua value is callable, will detect tables
     *  with __call metamethods as callable with a maximum nesting
     *  depth of 32.
     *
     * @param L Lua state.
     * @param idx Index on Lua stack.
     */
    bool isCallable(lua_State *L, int idx) {
        return isCallableHelper(L, idx, 0);
    }

    Script::Script(string path) : src(path) {
        if (src.size() == 0)
            throw Lua::LuaError("No path given");

        auto L = unique_ptr<lua_State, decltype(&lua_close)>(luaL_newstate(),
                                                             &lua_close);
        this->L = L.get();
        luaL_openlibs(L.get());

        from(path);

        L.release();
    }

    Script::Script() {
        L = luaL_newstate();
        luaL_openlibs(L);
    }

    /*
    void Script::sandboxFrom(const std::string& path) {
        from("sandbox.lua");
        call("init", realpath_safe(path), "__match");
        auto src = call<string>("getSource");
    }
    */

    void Script::from(const std::string& path) {
        if (luaL_loadfile(L, path.c_str()) != LUA_OK) {
            string err(lua_tostring(L, -1));
            throw Lua::LuaError("Lua error: " + err);
        }
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            string err(lua_tostring(L, -1));
            throw Lua::LuaError("Lua error: " + err);
        }

        abs_src = realpath_safe(path);
    }

    void Script::reset() {
        lua_close(L);
        auto L = unique_ptr<lua_State, decltype(&lua_close)>(luaL_newstate(), &lua_close);
        this->L = L.get();
        luaL_openlibs(L.get());
        L.release();
    }

    void Script::reload() {
        from(abs_src);
    }

    Script::~Script() noexcept {
        lua_close(L);
    }

    lua_State *Script::getL() noexcept {
        return L;
    }

    void Script::exec(const std::string &str) {
        if (luaL_dostring(L, str.c_str()) != LUA_OK) {
            throw LuaError("Error in exec of: " + str);
        }
    }

    std::mutex TimeoutHook::script_end_times_mtx;
    std::unordered_map<uintptr_t, milliseconds> TimeoutHook::script_end_times;

    TimeoutHook::TimeoutHook(lua_State *L, milliseconds time)
        : Hook(L, lua_timeout_hook, TimeoutHook::NUM_INST)
    {
        lock_guard<mutex> lock(script_end_times_mtx);
        milliseconds ms = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
        );
        script_end_times[reinterpret_cast<uintptr_t>(L)] = ms + time;
    }

    TimeoutHook::~TimeoutHook() {
        lock_guard<mutex> lock(script_end_times_mtx);
        script_end_times.erase(reinterpret_cast<uintptr_t>(L));
    }

#define MK_LUA_ERROR_HOOK(_name, _msg)                                  \
    static void lua_##_name##_error_hook(lua_State *L, lua_Debug *ar) { \
        (void) ar;                                                      \
        luaL_error(L, _msg);                                            \
    }

    extern "C" {
        MK_LUA_ERROR_HOOK(timeout, "Timeout Error")
    }
#undef MK_LUA_ERROR_HOOK

    extern "C" void lua_timeout_hook(lua_State *L, lua_Debug *ar) noexcept
    {
        (void) ar;
        lock_guard<mutex> lock(TimeoutHook::script_end_times_mtx);
        milliseconds end_time = TimeoutHook::script_end_times[reinterpret_cast<uintptr_t>(L)];
        milliseconds now = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
        );
        if (now > end_time) {
            lua_sethook(L, NULL, 0, 0);
            lua_sethook(L, lua_timeout_error_hook, LUA_MASKCOUNT, 1);
        }
    }

    extern "C" int hwk_lua_error_handler_callback(lua_State *L) noexcept
    {
        lua_Debug ar;
        size_t errmsg_sz = 0;
        const char *errmsg_c = lua_tolstring(L, -1, &errmsg_sz);
        string errmsg(errmsg_c, errmsg_sz);
        vector<lua_Debug> traceback;
        for (int lv = 0; lua_getstack(L, lv, &ar); lv++) {
            lua_getinfo(L, "Sunl", &ar);
            traceback.push_back(ar);
        }
        cout << "traceback size: " << traceback.size() << endl;
        lua_pushlightuserdata(L, new LuaError(errmsg, traceback));
        return 1;
    }

    /**
     * This __gc metamethod is used by LuaObject.
     * It is given two upvalues:
     *   1. The reference counter.
     *   2. The destructor function to use.
     */
    extern "C" int hwk_lua_object_destructor_cb(lua_State *L) noexcept
    {
        void *obj_ref = * (void **) lua_touserdata(L, -1);
        atomic<int> *rc = * (atomic<int> **) lua_touserdata(L, lua_upvalueindex(1));
        cout << "[from Lua] RC: " << *rc - 1 << endl;
        if (--*rc <= 0) {
            cout << "[from Lua] Destroying LuaObject userdata and reference-counter." << endl;
            typedef void (*Destructor)(void*);
            void (*destructor)(void*) = * (Destructor *) lua_touserdata(L, lua_upvalueindex(2));
            destructor(obj_ref);
            delete rc;
        }
        return 0;
    }

    std::atomic<uint64_t> id_incr;
}
