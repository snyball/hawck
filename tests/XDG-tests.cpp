#include "XDG.hpp"
#include <catch2/catch.hpp>

TEST_CASE("mkdirIfNotExists", "[XDG]") {
    rmdir("does-not-exist");
    CHECK(XDG::mkdirIfNotExists("does-not-exist"));
    CHECK(!XDG::mkdirIfNotExists("does-not-exist"));
}

TEST_CASE("mkPathIfNotExists relative", "[XDG]") {
    rmdir("does/not/exist");
    rmdir("does/not");
    rmdir("does");
    CHECK(XDG::mkPathIfNotExists("does/not/exist") == 3);
}

TEST_CASE("mkPathIfNotExists absolute", "[XDG]") {
    rmdir("/tmp/hawck-tests/does/not/exist");
    rmdir("/tmp/hawck-tests/does/not");
    rmdir("/tmp/hawck-tests/does");
    CHECK(XDG::mkPathIfNotExists("/tmp/hawck-tests/does/not/exist") >= 3);
}

TEST_CASE("mkPathIfNotExists absolute(ly) wrong", "[XDG]") {
    REQUIRE_THROWS(XDG::mkPathIfNotExists("/does/not/exist"));
}
