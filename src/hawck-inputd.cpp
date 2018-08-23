#include "KBDDaemon.hpp"
#include "Daemon.hpp"
#include <hawck_config.h>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: hawck-inputd <input device>\n");
        return EXIT_FAILURE;
    }

    const char *dev = argv[1];

    cout << "hawck-inputd v" INPUTD_VERSION " forking ..." << endl;
    //daemonize("/var/log/hawck-input/log");
    daemonize("/tmp/hawck-inputd.log");

    try {
        KBDDaemon daemon(dev);
        daemon.run();
    } catch (exception &e) {
        cout << "Error: " << e.what() << endl;
    }
}
