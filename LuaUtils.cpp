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

#include "LuaUtils.hpp"

using namespace std;

namespace Lua {
    /** Figure out if a Lua value is callable.
     *
     * It is not sufficient to just use lua_isfunction in certain cases,
     * as that won't handle the cases where the value is actually a
     * callable table. I.e a table that has a __call meta-method, will also
     * handle nested __call metamethods, i.e cases where a __call metamethod
     * is pointing to another table with a __call metamethod and so on.
     *
     * Warning: In case of deeply nested __call metamethods the function
     *          will throw an error. A soft maximum depth of 32 is used,
     *          but in case of low memory it can also fail due to the Lua
     *          stack no longer being resizeable (on most systems this will
     *          cause a segfault due to virtual memory overcommiting.)
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

    /** Expose isCallableHelper with an implicit depth as it is an
     *  implementation detail, see isCallableHelper for implementation.
     */
    bool isCallable(lua_State *L, int idx) {
        return isCallableHelper(L, idx, 0);
    }

    Script::Script(string path) : src(path) {
        L = luaL_newstate();
        luaL_openlibs(L);

        try {
            if (luaL_loadfile(L, path.c_str()) != LUA_OK) {
                string err(lua_tostring(L, -1));
                throw Lua::LuaError("Lua error: " + err);
            }
            if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
                string err(lua_tostring(L, -1));
                throw Lua::LuaError("Lua error: " + err);
            }
        } catch (Lua::LuaError &e) {
            lua_close(L);
            throw e;
        }
    }

    Script::~Script() noexcept {
        lua_close(L);
    }

    void Script::toggle(bool enabled) noexcept {
        this->enabled = enabled;
    }

    lua_State *Script::getL() noexcept {
        return L;
    }
}
