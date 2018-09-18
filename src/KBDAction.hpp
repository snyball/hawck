/** @file KBDAction.hpp
 *
 * @brief Layout of packets sent between InputD and MacroD
 *        to talk about input/output events.
 */

#pragma once

extern "C" {
    #include <linux/uinput.h>
    #include <linux/input.h>
    #include <stdint.h>
}

/** Actions as sent via UNIX socket from InputD to MacroD */
struct KBDAction {
    /** Whether this action signifies the end of a series of
     *  events. */
    uint8_t done : 1;
    /** The source keyboard for this event. */
    uint8_t kbd : 7;
    /** The event that was emitted, or should be emitted
     *  from the InputD UDevice. */
    struct input_event ev;
};
