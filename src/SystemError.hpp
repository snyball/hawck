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
        void *stack[elems];
        size_t size = backtrace(stack, elems);
        backtrace_symbols_fd(stack + 1, size - 1, fileno(stderr));
    }

public:
    explicit SystemError(const std::string &expl) : expl(expl) {}

    SystemError(const std::string &expl, int errnum) : expl(expl) {
        char errbuf[8192];
        memset(errbuf, 0, sizeof(errbuf));
        if (strerror_r(errnum, errbuf, sizeof(errbuf)) != 0) {
            switch (errno) {
                case ERANGE: strncpy(errbuf, "[strerror_r:ERANGE]", sizeof(errbuf)); break;
                case EINVAL: strncpy(errbuf, "[strerror_r:EINVAL]", sizeof(errbuf)); break;
                default:     strncpy(errbuf, "[strerror_r:UNKNOWN]", sizeof(errbuf)); break;
            }
        }
        printBacktrace();
        this->expl += errbuf;
    }

    virtual const char *what() const noexcept {
        return expl.c_str();
    }
};
