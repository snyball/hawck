#pragma once

#include <string>
#include <exception>

class SystemError : public std::exception {
private:
    std::string expl;
public:
    SystemError(std::string expl) : expl(expl) {}

    virtual const char *what() const noexcept {
        return expl.c_str();
    }
};
