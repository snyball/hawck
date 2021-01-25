#include "Version.hpp"
#include <catch2/catch.hpp>
#include <linux/version.h>

TEST_CASE("getVersion", "[Version]") {
    auto [major, minor, patch] = getLinuxVersion();

    // The assumption is that tests are compiled and run on the same machine.
    // This will fail if the kernel has been updated, without a reboot, because
    // of the new kernel headers.
    CHECK(KERNEL_VERSION(major, minor, patch) == LINUX_VERSION_CODE);
}
