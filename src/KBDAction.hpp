#pragma once

extern "C" {
    #include <linux/uinput.h>
    #include <linux/input.h>
    #include <stdint.h>
}

struct KBDAction {
    uint8_t done : 1;
    uint8_t kbd : 7;
    struct input_event ev;
};
