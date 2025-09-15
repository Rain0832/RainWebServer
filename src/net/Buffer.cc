#include <unistd.h>

#include <sys/uio.h>

#include <errno.h>

#include "Buffer.h"

ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0}; ///< Stack memory space 65536/1024 = 64KB

    /**
     * struct_iovec.h
     * struct iovec
     * {
     *      ptr_t iov_base; // iov_base buffer save readv or writev
     *      size_t iov_len;
     * };
     */
    struct iovec vec[2];
    const size_t writable = writableBytes();

    /// First buffer, point to writable space
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    /// Second buffer, point to stack space
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    /// when there is enough space in this buffer, don't read into extrabuf.
    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);

    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)
    {
        writerIndex_ += n;
    }
    else
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}