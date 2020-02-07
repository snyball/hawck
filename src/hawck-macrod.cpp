#include "MacroDaemon.hpp"
#include "Daemon.hpp"
#include "XDG.hpp"
#include <iostream>
#include <fstream>
#include "utils.hpp"
#if MESON_COMPILE
#include <hawck_config.h>
#else
#define MACROD_VERSION "unknown"
#endif

extern "C" {
    #include <getopt.h>
    #include <signal.h>
    #include <sys/types.h>
    #include <syslog.h>
}

using namespace std;

static int no_fork;

int main(int argc, char *argv[]) {
    string HELP =
        "Usage: hawck-macrod [--no-fork]\n"
        "\n"
        "Options:\n"
        "  --no-fork   Don't daemonize/fork.\n"
        "  -h, --help  Display this help information.\n"
        "  --version   Display version and exit.\n"
    ;
    XDG xdg("hawck");

    //daemonize("/var/log/hawck-input/log");
    static struct option long_options[] =
        {
            /* These options set a flag. */
            {"no-fork", no_argument,       &no_fork, 1},
            {"version", no_argument, 0, 0},
            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"help",         no_argument,       0, 'h'},
            {0, 0, 0, 0}
        };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    unordered_map<string, function<void(const string& opt)>> long_handlers = {
        {"version", [&](const string&) {
                        cout << "Hawck InputD v" MACROD_VERSION << endl;
                        exit(0);
                    }},
    };

    do {
        int c = getopt_long(argc, argv, "h",
                            long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
            case 0: {
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                string name(long_options[option_index].name);
                string arg(optarg ? optarg : "");
                if (long_handlers.find(name) != long_handlers.end())
                    long_handlers[name](arg);
                break;
            }

            case 'h':
                cout << HELP;
                exit(0);

            case '?':
                /* getopt_long already printed an error message. */
                break;

            default:
                cout << "DEFAULT" << endl;
                abort ();
        }
    } while (true);

    umask(0022);

    if (!no_fork) {
        cout << "hawck-macrod v" MACROD_VERSION " forking ..." << endl;
        xdg.mkpath(0700, XDG_DATA_HOME, "logs");
        daemonize(xdg.path(XDG_DATA_HOME, "logs", "macrod.log"));
    }

    // Kill any alread-running instance of macrod and write the new pid to the
    // pidfile.
    string pid_file = xdg.path(XDG_RUNTIME_DIR, "macrod.pid");
    killPretender(pid_file);

    MacroDaemon daemon;
    try {
        daemon.run();
    } catch (exception &e) {
        cout << e.what() << endl;
        syslog(LOG_CRIT, "MacroD: %s", e.what());
        clearPidFile(pid_file);
        throw;
    }
}
