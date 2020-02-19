#include "KBDB.hpp"
#include "utils.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

static_assert(is_same<decltype(((struct input_id*) nullptr)->bustype),
                      decltype(((struct input_id*) nullptr)->vendor)>::value);
static_assert(is_same<decltype(((struct input_id*) nullptr)->vendor),
                      decltype(((struct input_id*) nullptr)->product)>::value);
static_assert(is_same<decltype(((struct input_id*) nullptr)->product),
                      decltype(((struct input_id*) nullptr)->version)>::value);

/**
 * For unprivileged access we can't use ioctl()s, but sysfs is world-readable so
 * we can pull all the data from there.
 */
KBDInfo::KBDInfo(const struct input_id *id) : id(*id) {
  for (auto d : fs::directory_iterator("/sys/class/input")) {
    string path = d.path();
    string name = pathBasename(path);
    if (d.is_directory() && stringStartsWith(name, "event")) {
      auto id_path = pathJoin(path, "device", "id");
      struct input_id cid;
      typedef decltype(cid.vendor) id_t;
      vector<pair<string, id_t *>> parts = {{"bustype", &cid.bustype},
                                            {"vendor", &cid.vendor},
                                            {"product", &cid.product},
                                            {"version", &cid.version}};

      // Read parts of the id
      for (auto [part, ptr] : parts) {
        ifstream idp(pathJoin(id_path, part));
        if (!idp.is_open())
          throw SystemError("Unable to read: " + pathJoin(id_path, part));
        idp >> hex >> *ptr;
      }

      if (!::memcmp(&cid, id, sizeof(cid))) {
        // Found match
        initFrom(pathJoin(path, "device"));
      }
    }
  }
}

std::string KBDInfo::getID() noexcept {
  std::stringstream ss;
  ss << id.vendor << ":" << id.product << ":";
  for (char c : name) {
    if (c == ' ')
      ss << '_';
    else
      ss << c;
  }
  return ss.str();
}

void KBDInfo::initFrom(std::string path) noexcept(false) {
  vector<pair<string, string *>> parts = {{"name", &name},
                                          {"phys", &phys},
                                          {"uevent", &uevent},
                                          {"device/uevent", &dev_uevent}};

  for (auto [name, ptr] : parts) {
    ifstream in(pathJoin(path, name));
    if (!in.is_open())
      throw SystemError("Unable to read: " + pathJoin(path, name));
    getline(in, *ptr);
  }

  drv = pathBasename(realpath_safe(pathJoin(path, "device", "driver")));
}

KBDB::KBDB() {}
KBDB::~KBDB() {}
