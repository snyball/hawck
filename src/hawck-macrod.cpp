#include "MacroDaemon.hpp"
#include "Daemon.hpp"
#include <iostream>
#if MESON_COMPILE
#include <hawck_config.h>
#else
#define MACROD_VERSION "unknown"
#endif

extern "C" {
    #include <syslog.h>
    #include <getopt.h>
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

    cout << "hawck-macrod v" MACROD_VERSION " forking ..." << endl;

    if (!no_fork) {
        const char *home_cstring = getenv("HOME");
        if (home_cstring == nullptr) {
            cout << "Unable to find home directory" << endl;
            daemonize("/dev/null");
        } else {
            string HOME(home_cstring);
            daemonize(HOME + "/.local/share/hawck/macrod.log");
        }
    }

    MacroDaemon daemon;
    try {
        daemon.run();
    } catch (exception &e) {
        cout << e.what() << endl;
        syslog(LOG_CRIT, "MacroD: %s", e.what());
        throw;
    }
}
