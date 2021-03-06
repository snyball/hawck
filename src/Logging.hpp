#pragma once

#include <fmt/format.h>

extern "C" {
    #include <syslog.h>
}

class Log {
    template <class... T> static inline auto sendLog(int level, const std::string& fmt, T&&... args) {
        auto msg = fmt::format(fmt, args...);
        return ::syslog(level, "%s", msg.c_str());
    }

  public:
    template <class... T> static inline auto error(const std::string& fmt, T&&... args) {
        return sendLog(LOG_ERR, fmt, args...);
    }

    template <class... T> static inline auto warn(const std::string& fmt, T&&... args) {
        return sendLog(LOG_WARNING, fmt, args...);
    }

    template <class... T> static inline auto info(const std::string& fmt, T&&... args) {
        return sendLog(LOG_INFO, fmt, args...);
    }

    template <class... T> static inline auto debug(const std::string& fmt, T&&... args) {
        return sendLog(LOG_DEBUG, fmt, args...);
    }

    template <class... T> static inline auto notice(const std::string& fmt, T&&... args) {
        return sendLog(LOG_NOTICE, fmt, args...);
    }

    template <class... T> static inline auto crit(const std::string& fmt, T&&... args) {
        return sendLog(LOG_CRIT, fmt, args...);
    }

    template <class... T> static inline auto alert(const std::string& fmt, T&&... args) {
        return sendLog(LOG_ALERT, fmt, args...);
    }

    template <class... T> static inline auto emerg(const std::string &fmt, T &&...args) {
        return sendLog(LOG_EMERG, fmt, args...);
    }

    template <class... T> static inline auto panic(const std::string &fmt, T &&...args) {
        return sendLog(LOG_EMERG, fmt, args...);
    }
};
