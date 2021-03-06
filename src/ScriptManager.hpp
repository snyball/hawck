#pragma once

#include "LuaUtils.hpp"
#include "FSWatcher.hpp"
#include "utils.hpp"
#include "Permissions.hpp"
#include "XDG.hpp"
#include "Popen.hpp"
#include "Logging.hpp"
#include <unordered_map>
#include <string>
#include <thread>
#include <mutex>

extern "C" {
    #include <syslog.h>
    #include <sys/stat.h>
    #include <linux/uinput.h>
    #include <linux/input.h>
}

void openIfaces(Lua::Script *script) {}

template <class IT, class... T>
void openIfaces(Lua::Script *script,
                std::pair<Lua::LuaIface<IT>, std::string> iface,
                T... ts) {
    script->open(iface.first, iface.second);
    openIfaces(script, ts...);
}

/**
 * Manages any number of scripts.
 *
 * This way of managing the scripts is specifically made for
 * when you have a collection of scripts with the same entry
 * point, and you need to call each of them in succession
 * with different parameters.
 *
 * The entry function should return a Boolean value, when
 * this value is false the script execution stops at that
 * script.
 */
class ScriptManager {
    std::unordered_map<std::string, Lua::Script *> scripts;
    std::vector<Lua::Script> script_order;
    std::string home_dir;
    std::vector<std::string> lib_blacklist;
    std::mutex scripts_mtx;
    XDG xdg;
    FSWatcher fsw;
    bool stop_on_err = false;

protected:
    bool disable_on_err = true;

    virtual void handleLuaError(Lua::Script *sc,
                                const Lua::LuaError& e) noexcept {
        sc->setEnabled(!disable_on_err);
        Log::error("LUA:{}", e.fmtReport());
        // FIXME: ScriptManager needs to be able to show notifications here.
        // if (notify_on_err)
        //     notify("Lua error", report);
    }

public:
    ScriptManager(std::string home_dir,
                  const std::vector<std::string> *lib_blacklist = 0);

    virtual ~ScriptManager();

    template <class... T>
    void loadScript(const std::string& path, T... ts) {
        if (stringStartsWith(path, ".") || !(stringEndsWith(path, ".lua") ||
                                             stringEndsWith(path, ".hwk"))) {
            Log::notice("Not loading: {}, filename must end in .lua or .hwk "
                        "and may not start with a leading '.'",
                        path);
            return;
        }

        auto rpath = realpath_safe(path);
        if (!Permissions::checkFile(rpath, "frwxr-xr-x ~:*"))
            return;

        XDG xdg("hawck");
        auto sc = mkuniq(new Lua::Script());
        auto chdir = xdg.cd(XDG_DATA_HOME, "scripts");
        sc->call("require", "init");
        openIfaces(sc.get(), ts...);
        if (stringEndsWith(path, ".hwk")) {
            sc->exec(pathBasename(path), Popen("hwk2lua", path).readOnce());
        } else if (stringEndsWith(path, ".lua")) {
            sc->from(path);
        }
        auto name = pathBasename(path);
        if (scripts.find(name) != scripts.end()) {
            delete scripts[name];
            scripts.erase(name);
        }
        syslog(LOG_INFO, "Loaded script: %s", path.c_str());
        scripts[name] = sc.release();
        // FIXME: ScriptManager needs to be able to show notifications here.
    }

    void unloadScript(const std::string& rel_path);

    template <class... Args>
    Lua::Script *runAllScripts(std::string entry_func, Args... args) {
        static bool had_stack_leak_warning = false;
        std::lock_guard<std::mutex> lock(scripts_mtx);
        for (auto [src, sc] : scripts) {
            try {
                auto [succ] = sc->call(entry_func, args...);
                if (lua_gettop(sc->getL()) != 0) {
                    if (!had_stack_leak_warning) {
                        Log::warn("Lua API misuse causing stack leak of {} elements",
                                  lua_gettop(sc->getL()));
                        had_stack_leak_warning = true;
                    }
                    lua_settop(sc->getL(), 0);
                }
                if (sc->isEnabled() && !succ)
                    return sc;
            } catch (const Lua::LuaError &e) {
                handleLuaError(sc, e);
            }
        }
        return nullptr;
    }

    virtual Lua::Script *initScript(Lua::Script *sc) noexcept = 0;

    void startScriptWatcher();

    void initScriptDir(const std::string& dir_path);
};
