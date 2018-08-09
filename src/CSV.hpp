/* =====================================================================================
 * CSV library
 *
 * Copyright (C) 2018 Jonas Møller (no) <jonasmo441@gmail.com>
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

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>

/**
 * CSV library.
 */
class CSV {
private:
    std::vector<std::string> matrix;
    ssize_t w = 0, h = 0;

    /**
     * Format a csv cell, this involves putting cells inside quotes
     * and escaping quotes.
     */
    inline std::string fmtCell(std::string &s) {
        for (char c : s) {
            if (c == ',' || c == '"') {
                std::stringstream ss;
                ss << "\"";
                for (char c : s) {
                    if (c == '"') {
                        ss << "\"\"";
                    } else {
                        ss << c;
                    }
                }
                ss << "\"";
                return ss.str();
            }
        }
        return s;
    }

    /**
     * Write a CSV object to a stream.
     */
    template <class T>
    void _write(T &stream) {
        for (ssize_t y = 0; y < h; y++) {
            ssize_t base = y*w;
            for (ssize_t x = 0; x < w; x++) {
                stream << fmtCell(matrix[base + x]) << (x+1 == w ? "" : ",");
            }
            stream << std::endl;
        }
    }

    void loadFromStream(std::ifstream &istream);

public:
    class CSVError : public std::exception {
    private:
        std::string expl;
    public:
        CSVError(std::string expl) : expl(expl) {}

        virtual const char *what() const noexcept {
            return expl.c_str();
        }
    };

    /**
     * Read a csv file from a file path.
     */
    CSV(std::string path);

    /**
     * Read a csv file from an input stream.
     */
    CSV(std::ifstream &istream);

    /**
     * Get the number of rows in the csv object.
     */
    inline int nRows() { return h; }

    /**
     * Get the number of columns in the csv object.
     */
    inline int nCols() { return w; }

    /**
     * Get all cells at column `col_n`
     *
     * Pointers to strings are valid as long as the CSV object
     * lives.
     */
    std::vector<const std::string *> *getColCells(int col_n);

    /**
     * Get all cells in the column with header `name`.
     *
     * Pointers to strings are valid as long as the CSV object
     * lives.
     * 
     * @return vector of cells, or null if the name is not found.
     */
    inline std::vector<const std::string *> *getColCells(std::string name) {
        int idx = getColIndex(name);
        if (idx < 0)
            throw CSVError("No such name");
        return getColCells(idx);
    }

    /**
     * Get the column index given a column name.
     *
     * @return column index, or -1 if no such name is found.
     */
    inline int getColIndex(std::string name) {
        for (int i = 0; i < w; i++)
            if (matrix[i] == name)
                return i;
        return -1;
    }

    /**
     * Get a row of cells.
     */
    std::vector<const std::string *> *getRowCells(int row_n);

    /** See _write */
    inline void write(std::stringstream &s) { _write(s); }

    /** See _write */
    inline void write(std::ostream &s) { _write(s); }
};
