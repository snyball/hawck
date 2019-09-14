/* =====================================================================================
 * CSV library
 *
 * Copyright (C) 2018 Jonas Møller (no) <jonas.moeller2@protonmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * =====================================================================================
 */

#include "CSV.hpp"
#include <algorithm>
#include <tuple>

using namespace std;

CSV::CSV(string path) {
    ifstream istream(path, ios::in);
    loadFromStream(istream);
}

CSV::CSV(ifstream &istream) {
    loadFromStream(istream);
}

using Segment = tuple<int, int>;
static void segmentOn(string line, char sep, vector<Segment> &vec) {
    static const char quot = '"';
    int beg = 0,
        i = 0;
    bool in_quot = false,
         quit_quot = false;
    for (char c : line) {
        if (in_quot) {
            if (c == quot) {
                quit_quot = !quit_quot;
                i++;
                continue;
            } else if (quit_quot) {
                quit_quot = in_quot = false;
            }
        }

        if (c == quot) {
            in_quot = true;
        } else if (c == sep && !in_quot) {
            vec.push_back(make_tuple(beg, i));
            beg = i+1;
        }
        i++;
    }
    vec.push_back(make_tuple(beg, i));
}

/**
 * Turn escaped quotes inside quoted cells into single quotes.
 */
static string readCell(const string &cell) {
    if (cell.length() == 0 || cell[0] != '"') {
        return cell;
    }
    stringstream ss;
    bool skip = true;
    for (size_t i = 0; i < cell.length()-1; i++) {
        const char c = cell[i];
        if (skip) {
            skip = false;
            continue;
        }
        if (c == '"') {
            skip = true;
        }
        ss << c;
    }
    return ss.str();
}

void CSV::loadFromStream(ifstream &istream) {
    string line;

    // Get header first:
    getline(istream, line);
    vector<Segment> segment_idxs;
    auto pushRow = [&](string line) -> size_t {
        segmentOn(line, ',', segment_idxs);
        for (auto [beg, end] : segment_idxs)
            matrix.push_back(readCell(line.substr(beg, end-beg)));
        size_t sz = segment_idxs.size();
        segment_idxs.clear();
        return sz;
    };

    pushRow(line);

    // Set width
    w = matrix.size();
    h++;

    string empty = "";
    while (getline(istream, line)) {
        int segments = min((int) w, (int) pushRow(line));

        // Add padding
        for (int i = segments; i < w; i++)
            matrix.push_back(empty);
        h++;
    }
}

vector<const string *> *CSV::getColCells(int col_n) {
    auto vec = new vector<const string *>;
    for (int i = w+col_n; i < w*h; i += w) {
        vec->push_back(&matrix[i]);
    }
    return vec;
}

vector<const string *> *CSV::getRowCells(int row_n) {
    auto vec = new vector<const string *>;
    for (int i = row_n * w; i < row_n*(w+1); i++) {
        vec->push_back(&matrix[i]);
    }
    return vec;
}
