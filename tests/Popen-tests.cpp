#include "Popen.hpp"
#include <catch2/catch.hpp>
#include <string>
#include <iostream>

using namespace std;

TEST_CASE("echo", "[Popen]") {
    {
        Popen echo("echo", "-n", "a", "b", "c");
        REQUIRE(echo.readOnce() == "a b c");
    }

    {
        Popen echo("echo", "a", "b", "c");
        REQUIRE(echo.readOnce() == "a b c\n");
    }
}

TEST_CASE("long input", "[Popen]") {
    stringstream long_ss;
    for (int i = 0; i < 10000; i++)
        long_ss << "a\n";
    auto long_s = long_ss.str();
    Popen echo("echo", "-n", long_s);
    REQUIRE(echo.readOnce() == long_s);
}

TEST_CASE("long output", "[Popen]") {
    int n = 10000000;
    char c = 'A';
    stringstream src_ss;
    src_ss << "import sys; sys.stdout.write('" << c << "' * " << n << ")";
    Popen py("python", "-c", src_ss.str());
    auto out = py.readOnce();
    REQUIRE(out.size() == n);
}

TEST_CASE("nonexistant executable", "[Popen]") {
    string former_path(getenv("PATH"));
    setenv("PATH", "", 1);
    Popen ech("ech", "wasd");
    REQUIRE_THROWS(ech.readOnce());
    setenv("PATH", former_path.c_str(), 1);
}
