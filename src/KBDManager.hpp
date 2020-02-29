#pragma once

#include <mutex>
#include <vector>
#include <regex>

#include "Keyboard.hpp"

extern "C" {
    #include <syslog.h>
}

class KBDUnlock {
    std::vector<Keyboard *> kbds;

  public:
    inline KBDUnlock(std::vector<Keyboard*> &kbds) : kbds(kbds) {}
    ~KBDUnlock();
};

class KBDManager {
  private:
    /** Watcher for /dev/input/ hotplug */
    FSWatcher input_fsw;
    /** All keyboards. */
    std::vector<Keyboard *> kbds;
    std::mutex kbds_mtx;
    /** Keyboards available for listening. */
    std::vector<Keyboard *> available_kbds;
    std::mutex available_kbds_mtx;
    /** Keyboards that were removed. */
    std::vector<Keyboard *> pulled_kbds;
    std::mutex pulled_kbds_mtx;
    /** Controls whether or not /unseen/ keyboards may be added when they are
     * plugged in. Keyboards that were added on startup with --kbd-device
     * arguments will always be reconnected on hotplug. */
    bool allow_hotplug = true;

  public:
    inline KBDManager() {}

    ~KBDManager();

    KBDUnlock unlockAll();

    inline void setHotplug(bool val) {
        allow_hotplug = val;
    }

    /**
     * @param path A file path or file name from /dev/input/by-id
     * @return True iff the ID represents a keyboard.
     */
    static inline bool byIDIsKeyboard(const std::string& path) {
        const static std::regex input_if_rx("^.*-if[0-9]+-event-kbd$");
        const static std::regex event_kbd("^.*-event-kbd$");
        // Keyboard devices in /dev/input/by-id have filenames that end in
        // "-event-kbd", but they may also have extras that are available from
        // files ending in "-ifxx-event-kbd"
        return std::regex_match(path, event_kbd) && !std::regex_match(path, input_if_rx);
    }

    /** Listen on a new device.
     *
     * @param device Full path to the device in /dev/input/
     */
    void addDevice(const std::string& device);

    /**
     * Check which keyboards have become unavailable/available again.
     */
    void updateAvailableKBDs();

    /** Set delay between outputted events in µs
     *
     * @param delay Delay in µs.
     */
    void setEventDelay(int delay);

    void startHotplugWatcher();

    void setup();

    bool getEvent(KBDAction *action);
};
