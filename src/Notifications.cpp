#include "Notifications.hpp"
#include "Logging.hpp"

using namespace std;

extern "C" {
    #include <libnotify/notify.h>
}

using namespace Notifications;

std::mutex Notifications::last_notification_mtx;
std::tuple<std::string, std::string> Notifications::last_notification;

void libnotify_notify(string title, string msg, string icon) {
    lock_guard<mutex> lock(last_notification_mtx);
    tuple<string, string> notif(title, msg);
    if (notif == last_notification)
        return;
    last_notification = notif;

    // AFAIK you don't have to free the memory manually, but I could be wrong.
    NotifyNotification *n = notify_notification_new(title.c_str(), msg.c_str(), icon.c_str());
    if (n == nullptr)
        goto cannot_notify;
    notify_notification_set_timeout(n, 12000);
    notify_notification_set_urgency(n, NOTIFY_URGENCY_CRITICAL);
    notify_notification_set_app_name(n, "Hawck");
    if (!notify_notification_show(n, nullptr))
        goto cannot_notify;

    return;

cannot_notify:
    Log::warn("D-Bus notifications cannot be shown, logging them to syslog.");
    Log::info("{}: {}", title, msg);
}
