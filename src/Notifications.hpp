#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <fmt/format.h>

extern "C" {
    #include <libnotify/notify.h>
}

namespace Notifications {
    extern std::mutex last_notification_mtx;
    extern std::tuple<std::string, std::string> last_notification;
}

void libnotify_notify(std::string title, std::string msg, std::string icon);

template <class... T>
inline void notify(std::string title,
                   std::string fmt,
                   T&&... args) {
    auto msg = fmt::format(fmt, args...);
    libnotify_notify(title, msg, "hawck");
}

