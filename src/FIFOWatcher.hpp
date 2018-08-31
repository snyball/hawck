#pragma once

#include <string>
#include <memory>
#include <functional>

class FIFOWatcher {
private:
    int fd = -1;
    std::string path;
    uint32_t max_recv = 16384;

    virtual void watch();

    virtual void reset();

public:
    explicit FIFOWatcher(const std::string& path);

    virtual ~FIFOWatcher();

    virtual std::tuple<std::unique_ptr<char[]>, uint32_t> handleMessage(const char *buf, size_t);

    /** Start a new thread to watch the fifo, call the callback when
     *  a packet is received. */
    virtual void begin();
};
