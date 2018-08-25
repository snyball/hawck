#include "MacroDaemon.hpp"
#include "Daemon.hpp"
#include <iostream>
#if MESON_COMPILE
#include <hawck_config.h>
#else
#define MACROD_VERSION "unknown"
#endif

using namespace std;

int main(int argc, char *argv[]) {
    cout << "hawck-macrod v" MACROD_VERSION " forking ..." << endl;

    #if 0
    const char *home_cstring = getenv("HOME");
    if (home_cstring == nullptr) {
        cout << "Unable to find home directory" << endl;
        daemonize("/dev/null");
    } else {
        string HOME(home_cstring);
        daemonize(HOME + "/.local/share/hawck/macrod.log");
    }
    #endif

    try {
        MacroDaemon daemon;
        daemon.run();
    } catch (exception &e) {
        remove("/var/lib/hawck-input/kbd.sock");
        cout << e.what() << endl;
        exit(1);
    }
}
