#include "ScriptManager.hpp"
#include "utils.hpp"
#include "Permissions.hpp"
#include "Popen.hpp"
#include "Notifications.hpp"
#include "Logging.hpp"
#include <filesystem>

extern "C" {
    #include <sys/stat.h>
    #include <syslog.h>
}

using namespace std;
using namespace Lua;
using namespace Permissions;
namespace fs = std::filesystem;

ScriptManager::ScriptManager(string home_dir,
                             const vector<string> *lib_blacklist)
    : home_dir(home_dir),
      xdg("hawck")
{
    if (lib_blacklist != nullptr)
        for (string var : *lib_blacklist)
            this->lib_blacklist.push_back(var);

    this->home_dir = home_dir;
}

ScriptManager::~ScriptManager() {}

void ScriptManager::startScriptWatcher() {
    fsw.setWatchDirs(true);
    fsw.setAutoAdd(true);
    fsw.asyncWatch([this](FSEvent &ev) {
        lock_guard<mutex> lock(scripts_mtx);
        try {
            // Don't react to the directory itself.
            if (ev.path == xdg.path(XDG_CONFIG_HOME, "scripts"))
                return true;

            if (ev.mask & IN_DELETE) {
                syslog(LOG_INFO, "Deleting script: %s", ev.path.c_str());
                unloadScript(ev.name);
            } else if (ev.mask & IN_MODIFY) {
                syslog(LOG_INFO, "Reloading script: %s", ev.path.c_str());
                if (!S_ISDIR(ev.stbuf.st_mode))
                    loadScript(ev.path);
            } else if (ev.mask & IN_CREATE) {
                syslog(LOG_INFO, "Loading new script: %s", ev.path.c_str());
                loadScript(ev.path);
            } else if (ev.mask & IN_ATTRIB) {
                if (ev.stbuf.st_mode & S_IXUSR) {
                    loadScript(ev.path);
                } else {
                    unloadScript(ev.path);
                }
            }
        } catch (exception &e) {
            notify("Script Error", e.what());
            Log::error("Error while loading {}: {}", ev.path, e.what());
        }
        return true;
    });
}

void ScriptManager::initScriptDir(const std::string& dir_path) {
    for (auto entry : fs::directory_iterator(dir_path)) {
        try {
            loadScript(entry.path());
        } catch (exception &e) {
            notify("Hawck Script Error", e.what());
            syslog(LOG_ERR, "Unable to load script '%s': %s", pathBasename(entry.path()).c_str(),
                   e.what());
        }
    }
    fsw.addFrom(dir_path);
}

void ScriptManager::unloadScript(const std::string& rel_path)
{
    string name = pathBasename(rel_path);
    if (scripts.find(name) != scripts.end()) {
        syslog(LOG_INFO, "Deleting script: %s", name.c_str());
        delete scripts[name];
        scripts.erase(name);
        notify(name, "<i>Unloaded</i> script");
    } else {
        syslog(LOG_ERR, "Attempted to delete non-existent script: %s", name.c_str());
    }
}
