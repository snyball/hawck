#pragma once

#include <string>
#include <exception>

extern "C" {
    #include <string.h>
    #include <execinfo.h>
    #include <stdio.h>
}

#include <iostream>

class SystemError : public std::exception {
private:
    std::string expl;

    inline void printBacktrace() {
        constexpr int elems = 100;
        void *array[elems];
        size_t size;
        size = backtrace(array, elems);
        backtrace_symbols_fd(array, size, stderr->_fileno);
    }

public:
    explicit SystemError(const std::string &expl) : expl(expl) {}

    SystemError(const std::string &expl, int errnum) : expl(expl) {
        char errbuf[256];
        strerror_r(errnum, errbuf, sizeof(errbuf));
        this->expl += errbuf;
    }

    virtual const char *what() const noexcept {
        return expl.c_str();
    }
};
