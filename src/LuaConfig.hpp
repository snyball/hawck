#pragma once

extern "C" {
    #include <syslog.h>
}

#include "LuaUtils.hpp"
#include "FIFOWatcher.hpp"
#include <unordered_map>

class LuaConfig : public FIFOWatcher {
private:
    Lua::Script lua;
    std::unordered_map<std::string, std::function<void()>> option_setters;

public:

    explicit LuaConfig(const std::string& fifo_path, const std::string& luacfg_path);

    template <class T>
    void addOption(std::string name, std::atomic<T> *val) {
        option_setters[name] = [this, val, name]() {
                                   try {
                                       auto [new_val] = lua.call<T>("getConfigs", name);
                                       *val = new_val;
                                   } catch (const Lua::LuaError& e) {
                                       syslog(LOG_ERR, "Unable to set option %s: %s", name.c_str(), e.what());
                                   }
                               };
    }

    /** Handle the raw Lua code */
    virtual std::tuple<std::unique_ptr<char[]>, uint32_t> handleMessage(const char *msg, size_t sz) override;
};
