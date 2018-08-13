#include "MacroDaemon.hpp"
#include "Daemon.hpp"
#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
    #if 1
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
        cout << e.what() << endl;
        exit(1);
    }
}
