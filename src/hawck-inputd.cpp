#include <string>
#include <fstream>

#include "KBDDaemon.hpp"
#include "Daemon.hpp"
#include "utils.hpp"

#if MESON_COMPILE
#include <hawck_config.h>
#else
#define INPUTD_VERSION "unknown"
#endif

extern "C" {
    #include <signal.h>
    #include <getopt.h>
    #include <syslog.h>
}

using namespace std;

static void handleSigPipe(int) {
    // cout << "KBDDaemon aborting due to SIGPIPE" << endl;
    // abort();
}

static int no_fork;

auto varToOption(string opt) {
    replace(opt.begin(), opt.end(), '_', '-');
    return "--" + opt;
}

#define VAR_TO_OPTION(_var) varToOption(#_var)

auto numOption(const string& name, int *num) {
    return [num, name](const string& opt) {
               try {
                   *num = stoi(opt);
               } catch (const exception &e) {
                   cout << name << ": Require an integer" << endl;
                   exit(0);
               }
           };
}

auto strOption(const string&, string *str) {
    return [str](const string& opt) {
               *str = opt;
           };
}

#define NUM_OPTION(_name) {VAR_TO_OPTION(_name), numOption(VAR_TO_OPTION(_name), &(_name))},
#define STR_OPTION(_name) {VAR_TO_OPTION(_name), strOption(VAR_TO_OPTION(_name), &(_name))}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, handleSigPipe);

    string HELP =
        "Usage:\n"
        "    hawck-inputd [--udev-event-delay <µs>]\n"
        "                 [--no-fork]\n"
        "                 [--socket-timeout]\n"
        "                 -k <device>\n"
        "    hawck-inputd [-hv]\n"
        "\n"
        "Examples:\n"
        "    hawck-inputd --kbd-device /dev/input/event13\n"
        "    hawck-inputd --udev-event-delay 2500 --no-fork -k{/dev/input/event13,/dev/input/event15}\n"
        "    hawck-inputd $(/usr/share/hawck/bin/get-kbd-args.sh)\n"
        "\n"
        "Options:\n"
        "    --no-fork           : Don't daemonize/fork.\n"
        "    -h,--help           : Display this help information.\n"
        "    -v,--version        : Display version and exit.\n"
        "    -k,--kbd-device     : Add a keyboard to listen to.\n"
        "    --udev-event-delay  : Delay between events sent on the udevice in µs.\n"
        "    --socket-timeout    : Time in milliseconds until timeout on sockets.\n"
    ;

    //daemonize("/var/log/hawck-input/log");
    static struct option long_options[] =
        {
            /* These options set a flag. */
            {"no-fork", no_argument,       &no_fork, 1},
            {"udev-event-delay", required_argument,       0, 0},
            {"socket-timeout", required_argument,       0, 0},
            {"version", no_argument, 0, 0},
            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"help",         no_argument,       0, 'h'},
            {"kbd-device",   required_argument, 0, 'k'},
            {"kbd-name",     required_argument, 0, 'n'},
            {0, 0, 0, 0}
        };
    /* getopt_long stores the option index here. */
    int option_index = 0;

    int udev_event_delay = 3800;
    int socket_timeout = 1024;
    vector<string> kbd_names;
    vector<string> kbd_devices;
    unordered_map<string, function<void(const string& opt)>> long_handlers = {
        {"version", [&](const string&) {
                        cout << "Hawck InputD v" INPUTD_VERSION << endl;
                        exit(0);
                    }},
        NUM_OPTION(udev_event_delay)
        NUM_OPTION(socket_timeout)
    };

    do {
        int c = getopt_long(argc, argv, "hk:n:",
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

            case 'k':
                kbd_devices.push_back(string(optarg));
                break;

            // TODO: Implement lookup of keyboard names
            case 'n':
                cout << "Option -n/--kbd-name: Not implemented." << endl;
                break;

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

    if (kbd_devices.size() == 0) {
        cout << "Unable to start Hawck InputD without any keyboard devices." << endl;
        exit(0);
    }

    remove("/var/lib/hawck-input/pid");

    cout << "Starting Hawck InputD v" INPUTD_VERSION " on:" << endl;
    for (const auto& dev : kbd_devices)
        cout << "  - <" << dev << ">" << endl;

    if (!no_fork) {
        cout << "forking ..." << endl;
        daemonize("/tmp/hawck-inputd.log");
    }

    // Write pid
    try {
        ofstream ostream("/var/lib/hawck-input/pid");
        ostream << getpid();
    } catch (const exception &e) {
        throw SystemError("Unable to write pid: ", errno);
    }

    try {
        cout << "Settin up daemon ..." << endl;
        KBDDaemon daemon;
        cout << "Adding devices ..." << endl;
        for (const auto& dev : kbd_devices)
            daemon.addDevice(dev);
        daemon.setEventDelay(udev_event_delay);
        daemon.setSocketTimeout(socket_timeout);
        syslog(LOG_INFO, "Running Hawck InputD ...");
        cout << "Running ..." << endl;
        daemon.run();
    } catch (const exception &e) {
        syslog(LOG_CRIT, "Abort due to exception: %s", e.what());
        cout << "Error: " << e.what() << endl;
        remove("/var/lib/hawck-input/pid");
        throw;
    }
}
