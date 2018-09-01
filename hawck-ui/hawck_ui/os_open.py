import os

class OSOpen:
    def __init__(self, path, mode):
        self.path = path
        self.mode = mode
        self.fd = -1

    def __enter__(self):
        self.fd = os.open(self.path, self.mode)
        return self

    def __exit__(self, type, value, traceback):
        if self.fd != -1:
            os.close(self.fd)

    def read(self, sz: int):
        return os.read(self.fd, sz)

    def write(self, buf: bytes):
        return os.write(self.fd, buf)

    def close(self):
        try:
            os.close(fd)
        finally:
            self.fd = -1
