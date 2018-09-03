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
    std::string luacfg_path;

public:

    explicit LuaConfig(const std::string& fifo_path,
                       const std::string& ofifo_path,
                       const std::string& luacfg_path);

    template <class T>
    void addOption(const std::string& name, std::atomic<T> *val) {
        addOption<T>(name, [val](T got) {*val = got;});
    }

    template <class T>
    void addOption(std::string name, const std::function<void(T)> &callback) {
        option_setters[name] = [this, name, callback] {
                                   try {
                                       auto [ret] = lua.call<T>("getConfigs", name);
                                       callback(ret);
                                   } catch (const Lua::LuaError& e) {
                                       syslog(LOG_ERR,
                                              "Unable to set option %s: %s", name.c_str(), e.what());
                                   }
                               };
    }

    /** Handle the raw Lua code */
    virtual std::string handleMessage(const char *msg, size_t sz) override;
};
