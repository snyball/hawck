#pragma once

#include <string>
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

    virtual void handleMessage(const char *msg, size_t sz);

    /** Start a new thread to watch the fifo, call the callback when
     *  a packet is received. */
    virtual void begin();
};
