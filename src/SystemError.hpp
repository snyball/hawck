#pragma once

#include <string>
#include <exception>
#include <mutex>

extern "C" {
    #undef _GNU_SOURCE
    #include <string.h>
    #define _GNU_SOURCE
    #include <execinfo.h>
    #include <stdio.h>
}

#include <iostream>

static std::mutex strerror_mtx;

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
        // FIXME: This always results in strerror_r:UNKNOWN, while
        //        the strerror function works just fine.
        #if 0
        char errbuf[512];
        memset(errbuf, 0, sizeof(errbuf));
        if (strerror_r(errnum, errbuf, sizeof(errbuf)) != 0) {
            switch (errno) {
                case ERANGE: strncpy(errbuf, "[strerror_r:ERANGE]", sizeof(errbuf)); break;
                case EINVAL: strncpy(errbuf, "[strerror_r:EINVAL]", sizeof(errbuf)); break;
                default:     strncpy(errbuf, "[strerror_r:UNKNOWN]", sizeof(errbuf)); break;
            }
        }
        this->expl += errbuf;
        #else
        // Workaround using static mutex
        std::lock_guard<std::mutex> lock(strerror_mtx);
        this->expl += strerror(errnum);
        #endif
    }

    virtual const char *what() const noexcept {
        return expl.c_str();
    }
};
