#pragma once

#include <vector>
#include <limits.h>
#include <assert.h>
#include "KBDAction.hpp"
#include "KeyValue.hpp"

#include <iostream>
struct KeyCombo {
    std::vector<int> key_seq;
    int activator;
    int num_seq;

    inline KeyCombo(const std::vector<int>& seq) noexcept {
        assert(seq.size() < INT_MAX);
        key_seq = seq;
        activator = key_seq[key_seq.size() - 1];
        key_seq.pop_back();
    }

    inline bool check(const KBDAction &action) noexcept {
        if (action.ev.value == KEY_VAL_REPEAT)
            return false;

        if (num_seq == ((int) key_seq.size()) && action.ev.code == activator &&
            action.ev.value == KEY_VAL_DOWN)
        {
            num_seq = 0;
            return true;
        }

        for (auto k : key_seq) {
            if (k == action.ev.code) {
                num_seq += (action.ev.value == KEY_VAL_DOWN) ? 1 : -1;
                break;
            }
        }

        if (num_seq < 0)
            num_seq = 0;

        return false;
    }
};

struct KeyComboToggle : public KeyCombo {
    bool active = false;
    inline KeyComboToggle(const std::vector<int>& seq) noexcept
        : KeyCombo(seq)  {}

    inline bool check(const KBDAction& action) noexcept {
        if (KeyCombo::check(action))
            active = !active;
        return active;
    }
};
