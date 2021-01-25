#pragma once

extern "C" {
    #include <linux/version.h>
}

#include <tuple>
#include "SystemError.hpp"

std::tuple<size_t, size_t, size_t> getLinuxVersion();

static inline size_t getLinuxVersionCode() {
    auto [major, minor, patch] = getLinuxVersion();
    return KERNEL_VERSION(major, minor, patch);
}
