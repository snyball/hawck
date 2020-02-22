#pragma once

#include <unordered_map>

extern "C" {
    #include <linux/uinput.h>
    #include <linux/input.h>
}

struct InputIDHash
{
  std::size_t operator()(const struct input_id& k) const {
    using std::hash;
    return ((hash<int>()(k.bustype)
             ^ (hash<int>()(k.vendor) << 1)) >> 1)
             ^ (hash<int>()(k.product) << 1
             ^ (hash<int>()(k.version) << 3));
  }
};

inline bool operator==(const struct input_id &a, const struct input_id &b) {
  return (a.bustype == b.bustype) && (a.product == b.product) &&
         (a.vendor == b.vendor) && (a.version == b.version);
}

/**
 * TODO: Refactor the Keyboard class to use KBDInfo
 */
class KBDInfo {
private:
    std::string name;
    std::string drv;
    std::string phys;
    std::string uevent;
    std::string dev_uevent;
    struct input_id id;

    void initFrom(const std::string& path) noexcept(false);

public:
    KBDInfo(const struct input_id *id) noexcept(false);
    inline KBDInfo() noexcept {}

    std::string getID() noexcept;
};


/**
 * Keyboard database, accepts `struct input_id` and retrieves the rest of the
 * information from scanning sysfs.
 *
 * Currently its only used to manage human-readable keyboard IDs.
 */
class KBDB {
private:
    std::unordered_map<struct input_id, KBDInfo, InputIDHash> info;

public:
    KBDB();
    ~KBDB();

    std::string getID(const struct input_id *id) noexcept(false) {
        if (info.count(*id) == 0) {
            KBDInfo kinfo(id);
            info[*id] = kinfo;
            return kinfo.getID();
        }
        return info[*id].getID();
    }
};
