#define _DEFAULT_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>

pid_t MACROD_PID;
pid_t INPUTD_PID;

uid_t uidof(const char *user)
{
    uid_t ret = -1;
    struct passwd pwd;
    struct passwd *result;

    int bufsz = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsz == -1)
        bufsz = 16384;

    char *buf = malloc(bufsz);
    if (buf == NULL) {
        perror("malloc");
        abort();
    }

    int s = getpwnam_r(user, &pwd, buf, bufsz, &result);
    if (result == NULL) {
        if (s == 0) {
            fprintf(stderr, "No such user: %s", user);
        } else {
            errno = s;
            perror("getpwnam_r");
        }
    } else {
        ret = pwd.pw_uid;
    }

    free(buf);

    return ret;
}

pid_t runuser(const char *destdir,
              const char *username,
              const char *exe_path,
              char *const *argv)
{
    pid_t pid;
    char uid = uidof(username);
    switch ((pid = fork())) {
        case -1:
            fprintf(stderr, "Unable to fork(): %s", strerror(errno));
            return -1;

        case 0:
            if (destdir != NULL)
                chroot(destdir);
            setuid(uid);
            execvp(exe_path, argv);
            exit(EXIT_FAILURE);

        default:
            return pid;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
        fprintf(stderr, "Usage: hawck-chroot <destdir> <kbd device> <output file>");

    char *destdir = argv[1];
    char *kbd_dev = argv[2];
    char *out = argv[3];

    char *const inputd_argv[] = {"--kbd-device", "--no-hotplug", kbd_dev, "--dummy-output",
                                 out};
    char *const macrod_argv[] = {};

    pid_t macrod_pid = runuser(destdir, "hawck-test-user", "hawck-macrod", macrod_argv);
    pid_t inputd_pid = runuser(destdir, "hawck-input", "hawck-inputd", inputd_argv);

    if (macrod_pid == -1 || inputd_pid == -1)
        goto cleanup;

    sigset_t sigset;
    int sig;
    sigfillset(&sigset);
    sigwait(&sigset, &sig);

cleanup:
    if (macrod_pid != -1)
        kill(macrod_pid, SIGKILL);
    if (inputd_pid != -1)
        kill(inputd_pid, SIGKILL);

    return 0;
}
