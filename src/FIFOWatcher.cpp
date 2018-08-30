extern "C" {
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
}

#include <thread>

#include "FIFOWatcher.hpp"
#include "UNIXSocket.hpp"
#include "utils.hpp"

using namespace std;

FIFOWatcher::FIFOWatcher(const std::string& fifo_path)
    : path(fifo_path)
{}

FIFOWatcher::~FIFOWatcher() {
    if (fd != -1)
        ::close(fd);
}

void FIFOWatcher::reset() {
    if (fd != -1)
        close(fd);

    if ((fd = ::open(path.c_str(), O_RDONLY)) == -1) {
        throw SystemError("Unable to open fifo: ", errno);
    }
}

void FIFOWatcher::watch() {
    uint32_t sz;

    reset();

    for (;;) {
        try {
            recvAll(fd, &sz);
            if (sz >= max_recv || sz == 0) {
                reset();
                continue;
            }
            auto buf = unique_ptr<char[]>(new char[sz + 1]);
            buf.get()[sz] = 0;
            recvAll(fd, buf.get(), sz);
            handleMessage(buf.get(), sz);
        } catch (const SocketError& e) {
            reset();
        }
    }
}

void FIFOWatcher::handleMessage(const char *buf, size_t) {
    cout << "Got message: " << buf << endl;
}

void FIFOWatcher::begin() {
    cout << "####### BEGIN #######" << endl;
    thread t0([this]() {
                  watch();
              });
    t0.detach();
    cout << "###### DETACHED #####" << endl;
}

