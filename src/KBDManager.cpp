#include "KBDManager.hpp"
#include "utils.hpp"
#include "Permissions.hpp"

using namespace std;
using namespace Permissions;

constexpr int FSW_MAX_WAIT_PERMISSIONS_US = 5 * 1000000;

KBDManager::~KBDManager() {
    for (Keyboard *kbd : kbds)
        delete kbd;
}

void KBDManager::setup() {
    for (auto& kbd : kbds) {
        syslog(LOG_INFO, "Attempting to get lock on device: %s @ %s",
               kbd->getName().c_str(), kbd->getPhys().c_str());
        kbd->lock();
    }

    updateAvailableKBDs();
}

bool KBDManager::getEvent(KBDAction *action) {
    Keyboard *kbd = nullptr;
    bool had_key = false;

    try {
        available_kbds_mtx.lock();
        vector<Keyboard *> kbds(available_kbds);
        available_kbds_mtx.unlock();
        int idx = kbdMultiplex(kbds, 64);
        if (idx != -1) {
            kbd = kbds[idx];
            kbd->get(action);

            // Throw away the key if the keyboard isn't locked yet.
            if (kbd->getState() == KBDState::LOCKED)
                had_key = true;
            // Always lock unlocked keyboards.
            else if (kbd->getState() == KBDState::OPEN)
                kbd->lock();
        }
    } catch (const KeyboardError &e) {
        // Disable the keyboard,
        syslog(LOG_ERR, "Read error on keyboard, assumed to be removed: %s",
               kbd->getName().c_str());
        kbd->disable();
        {
            lock_guard<mutex> lock(available_kbds_mtx);
            auto pos_it = find(available_kbds.begin(), available_kbds.end(), kbd);
            available_kbds.erase(pos_it);
        }
        lock_guard<mutex> lock(pulled_kbds_mtx);
        pulled_kbds.push_back(kbd);
    }

    return had_key;
}

/**
 * Loop until the file has the correct permissions, when immediately added
 * /dev/input/ files seem to be owned by root:root or by root:input with
 * restrictive permissions. We expect it to be root:input with the input group
 * being able to read and write.
 *
 * @return True iff the device is ready to be opened.
 */
static bool waitForDevice(const string& path) {
    const int wait_inc_us = 100;

    gid_t input_gid; {
        auto [grp, grpbuf] = getgroup("input");
        (void) grpbuf;
        input_gid = grp->gr_gid;
    }

    struct stat stbuf;
    unsigned grp_perm;
    int ret;
    int wait_time = 0;
    do {
        usleep(wait_inc_us);
        ret = stat(path.c_str(), &stbuf);
        grp_perm = stbuf.st_mode & S_IRWXG;

        // Check if it is a character device, test is
        // done here because permissions might not allow
        // for even stat()ing the file.
        if (ret != -1 && !S_ISCHR(stbuf.st_mode)) {
            // Not a character device, return
            syslog(LOG_WARNING, "File %s is not a character device", path.c_str());
            return false;
        }

        if ((wait_time += wait_inc_us) > FSW_MAX_WAIT_PERMISSIONS_US) {
            syslog(LOG_ERR,
                   "Could not aquire permissions rw with group input on '%s'",
                   path.c_str());
            // Skip this file
            return false;
        }
    } while (ret != -1 && !(4 & grp_perm && grp_perm & 2) &&
             stbuf.st_gid != input_gid);

    return true;
}

void KBDManager::startHotplugWatcher() {
    input_fsw.add("/dev/input/by-id");
    input_fsw.setWatchDirs(true);
    input_fsw.setAutoAdd(false);
    input_fsw.asyncWatch([this](FSEvent &ev) {
        // Don't react to the directory itself.
        if (ev.path == "/dev/input/by-id")
            return true;

        string event_path = realpath_safe(ev.path);

        syslog(LOG_INFO, "Hotplug event on %s -> %s", ev.path.c_str(), event_path.c_str());

        if (!waitForDevice(event_path))
            return true;

        {
            lock_guard<mutex> lock(pulled_kbds_mtx);
            for (auto it = pulled_kbds.begin(); it != pulled_kbds.end(); it++) {
                auto kbd = *it;
                if (kbd->isMe(ev.path.c_str())) {
                    syslog(LOG_INFO, "Keyboard was plugged back in: %s", kbd->getName().c_str());
                    kbd->reset(ev.path.c_str());
                    kbd->lock();
                    {
                        lock_guard<mutex> lock(available_kbds_mtx);
                        available_kbds.push_back(kbd);
                    }
                    pulled_kbds.erase(it);
                    break;
                }
            }
        }

        if (!allow_hotplug)
            return true;

        // If the keyboard is not in pulled_kbds, we want to
        // make sure that it actually is a keyboard.
        if (!KBDManager::byIDIsKeyboard(ev.path))
            return true;

        {
            lock_guard<mutex> lock(kbds_mtx);
            Keyboard *kbd = new Keyboard(event_path.c_str());
            kbds.push_back(kbd);
            syslog(LOG_INFO, "New keyboard plugged in: %s", kbd->getID().c_str());
            kbd->lock();
        }
        updateAvailableKBDs();

        return true;
    });
}


KBDUnlock KBDManager::unlockAll() {
                lock_guard<mutex> lock(available_kbds_mtx);
    std::vector<Keyboard *> unlocked;
    for (auto &kbd : available_kbds) {
        try {
            syslog(LOG_INFO, "Unlocking keyboard due to error: \"%s\" @ %s", kbd->getName().c_str(),
                   kbd->getPhys().c_str());
            kbd->unlock();
            unlocked.push_back(kbd);
        } catch (const KeyboardError &e) {
            syslog(LOG_ERR, "Unable to unlock keyboard: %s", kbd->getName().c_str());
            kbd->disable();
        }
    }

    return KBDUnlock(kbds);
}

KBDUnlock::~KBDUnlock() {
    for (auto &kbd : kbds) {
        try {
            kbd->lock();
        } catch (const KeyboardError &e) {
            syslog(LOG_ERR, "Unable to lock keyboard: %s", kbd->getName().c_str());
        }
    }
}

void KBDManager::updateAvailableKBDs() {
    lock_guard<mutex> lock1(available_kbds_mtx);
    lock_guard<mutex> lock2(kbds_mtx);

    available_kbds.clear();
    for (auto &kbd : kbds)
        if (!kbd->isDisabled())
            available_kbds.push_back(kbd);
}

void KBDManager::addDevice(const std::string& device) {
    lock_guard<mutex> lock(kbds_mtx);
    kbds.push_back(new Keyboard(device.c_str()));
}
