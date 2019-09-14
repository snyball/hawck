#pragma once

#include <chrono>
#include <string>
#include <memory>
#include <functional>

class FIFOWatcher {
    using Milliseconds = std::chrono::milliseconds;
private:
    int fd = -1;
    int ofd = -1;
    std::string path;
    std::string opath;
    uint32_t max_recv = 16384;

    virtual void watch();

    virtual void reset();

public:
    explicit FIFOWatcher(const std::string& fifo_path, const std::string& fifo_in_path);

    virtual ~FIFOWatcher();

    virtual std::string handleMessage(const char *buf, size_t);

    virtual void reply(std::string buf, Milliseconds timeout);

    /** Start a new thread to watch the fifo, call the callback when
     *  a packet is received. */
    virtual void start();
};
