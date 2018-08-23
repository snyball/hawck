#include <fstream>

#include "KBDDaemon.hpp"
#include "Daemon.hpp"
#include "utils.hpp"
#if MESON_COMPILE
#include <hawck_config.h>
#endif

extern "C" {
#include <signal.h>
}

using namespace std;

static void handleSigPipe(int) {
    cout << "KBDDaemon aborting due to SIGPIPE" << endl;
    abort();
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, handleSigPipe);

    if (argc < 2) {
        fprintf(stderr, "Usage: hawck-inputd <input device>\n");
        return EXIT_FAILURE;
    }

    const char *dev = argv[1];

    cout << "hawck-inputd v" INPUTD_VERSION " forking ..." << endl;
    //daemonize("/var/log/hawck-input/log");
    daemonize("/tmp/hawck-inputd.log");

    // Write pid
    try {
        ofstream ostream("/var/lib/hawck-input/pid");
        ostream << getpid();
        ostream.close();
    } catch (const exception &e) {
        throw SystemError("Unable to write pid: ", errno);
    }

    try {
        KBDDaemon daemon(dev);
        daemon.run();
    } catch (exception &e) {
        cout << "Error: " << e.what() << endl;
    }
}
