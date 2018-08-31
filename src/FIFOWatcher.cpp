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
            auto [ret, ret_sz] = handleMessage(buf.get(), sz);
            if (ret != nullptr) {
                write(fd, &ret_sz, sizeof(ret_sz));
                write(fd, ret.get(), ret_sz);
            }
        } catch (const SocketError& e) {
            reset();
        }
    }
}

tuple<unique_ptr<char[]>, uint32_t> FIFOWatcher::handleMessage(const char *buf, size_t) {
    cout << "Got message: " << buf << endl;
    return make_tuple(nullptr, 0);
}

void FIFOWatcher::begin() {
    cout << "####### BEGIN #######" << endl;
    thread t0([this]() {
                  watch();
              });
    t0.detach();
    cout << "###### DETACHED #####" << endl;
}

