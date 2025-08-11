#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

#include <Buffer.h>

ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    // Stack extra space, used to temporarily store data when reading from the socket,
    // when the buffer_ is temporarily not enough, the data is stored temporarily,
    // and then appended to the buffer_ when it becomes full.
    char extrabuf[65536] = {0}; // Stack memory space 65536/1024 = 64KB

    /*
    struct iovec {
        ptr_t iov_base; // iov_base buffer save readv or writev
        size_t iov_len;
    };
    */

    struct iovec vec[2];
    const size_t writable = writableBytes();

    // First buffer, point to writable space
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    // Second buffer, point to stack space
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
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