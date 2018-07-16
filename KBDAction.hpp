#pragma once

extern "C" {
    #include <linux/uinput.h>
    #include <linux/input.h>
    #include <stdint.h>
}

struct KBDAction {
    uint8_t done : 1;
    struct input_event ev;
};
