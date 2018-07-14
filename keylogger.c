#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/uinput.h>
#include <linux/input.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

static const char *const evval[] = {
    "UP",
    "DOWN",
    "REPEAT"
};

static const char *event_str[EV_CNT];

typedef struct _UDevice {
    int fd;
    struct uinput_setup usetup;
} UDevice;

void initEventStrs()
{
    event_str[EV_SYN      ] = "SYN"       ;
    event_str[EV_KEY      ] = "KEY"       ;
    event_str[EV_REL      ] = "REL"       ;
    event_str[EV_ABS      ] = "ABS"       ;
    event_str[EV_MSC      ] = "MSC"       ;
    event_str[EV_SW       ] = "SW"        ;
    event_str[EV_LED      ] = "LED"       ;
    event_str[EV_SND      ] = "SND"       ;
    event_str[EV_REP      ] = "REP"       ;
    event_str[EV_FF       ] = "FF"        ;
    event_str[EV_PWR      ] = "PWR"       ;
    event_str[EV_FF_STATUS] = "FF_STATUS" ;
    event_str[EV_MAX      ] = "MAX"       ;
}

UDevice *newUDevice()
{
    UDevice *dev = malloc(sizeof(UDevice));
    if (dev == NULL) {
        fprintf(stderr, "Unable to allocate memory");
        abort();
    }

    dev->fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);

    if (dev->fd < 0) {
        fprintf(stderr, "Unable to open /dev/uinput: %s\n", strerror(errno));
        return NULL;
    }

    /*
     * The ioctls below will enable the device that is about to be
     * created, to pass key events, in this case the space key.
     */
    ioctl(dev->fd, UI_SET_EVBIT, EV_KEY);
    for (int key = KEY_ESC; key <= KEY_MICMUTE; key++) {
        ioctl(dev->fd, UI_SET_KEYBIT, key);
    }

    memset(&dev->usetup, 0, sizeof(dev->usetup));
    dev->usetup.id.bustype = BUS_USB;
    dev->usetup.id.vendor = 0x1234; /* sample vendor */
    dev->usetup.id.product = 0x5678; /* sample product */
    strcpy(dev->usetup.name, "EvD");

    ioctl(dev->fd, UI_DEV_SETUP, &dev->usetup);
    ioctl(dev->fd, UI_DEV_CREATE);

    // Give the system time to notice us!
    sleep(1);

    return dev;
}

void destroyUDevice(UDevice *udev)
{
    ioctl(udev->fd, UI_DEV_DESTROY);
    close(udev->fd);
    free(udev);
}

void deviceEmit(UDevice *dev, int type, int code, int val)
{
    struct input_event ie;

    ie.type = type;
    ie.code = code;
    ie.value = val;
    /* timestamp values below are ignored */
    ie.time.tv_sec = 0;
    ie.time.tv_usec = 0;

    write(dev->fd, &ie, sizeof(ie));
}

int main(int argc, char *argv[])
{
    initEventStrs();

    if (argc < 3) {
        fprintf(stderr, "Usage: ev <input device> <output file>\n");
        return EXIT_FAILURE;
    }

    const char *dev = argv[1];
    const char *out_path = argv[2];
    struct input_event ev;
    ssize_t n;
    int fd;

    fd = open(dev, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Cannot open %s: %s.\n", dev, strerror(errno));
        return EXIT_FAILURE;
    }

    FILE *out = fopen(out_path, "a");
    if (out == NULL) {
        fprintf(stderr, "Cannot open %s: %s\n", out_path, strerror(errno));
        return EXIT_FAILURE;
    }

    UDevice *udev = newUDevice();
    if (udev == NULL) {
        fprintf(stderr, "Unable to create new udevice\n");
        return EXIT_FAILURE;
    }

    int grab = 1;
    ioctl(fd, EVIOCGRAB, &grab);

    fprintf(stderr, "Started.\n");

    char *line = NULL;
    size_t line_len = 0;
    bool repeat = true;

    for (;;) {
        if ((n = read(fd, &ev, sizeof(ev))) != sizeof(ev)) {
            fprintf(stderr, "ERROR: %s\n", strerror(errno));
            continue;
        }

        fprintf(out, "%s: %s %d\n", event_str[ev.type], (ev.value <= 2) ? evval[ev.value] : "?", (int)ev.code);
        fflush(out);

        write(udev->fd, &ev, sizeof(ev));
    }

    ioctl(fd, EVIOCGRAB, NULL);
    close(fd);

    return EXIT_SUCCESS;
}

