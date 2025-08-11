#pragma once

#include <stddef.h>

#include <algorithm>
#include <string>
#include <vector>

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initalSize = kInitialSize)
        : buffer_(kCheapPrepend + initalSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend)
    {
    }

    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }

    // Return buffer readable data start address
    const char *peek() const { return begin() + readerIndex_; }
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            // Application read part of the buffer (len)
            // left readerIndex_ += len to writerIndex_ (not read)
            readerIndex_ += len;
        }
        else // len == readableBytes()
        {
            retrieveAll();
        }
    }
    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    // Set on Message function report buffer data to string type data return
    std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        // Already read all readable data from buffer
        // Here we reset buffer readerIndex_ and writerIndex_ to kCheapPrepend
        retrieve(len);
        return result;
    }

    // buffer_.size - writerIndex_
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len); // Expand buffer
        }
    }

    // Set [data, data+len] memory data to writable buffer
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }
    char *beginWrite() { return begin() + writerIndex_; }
    const char *beginWrite() const { return begin() + writerIndex_; }

    // Read data from fd to buffer
    ssize_t readFd(int fd, int *saveErrno);

    // Send data from buffer to fd
    ssize_t writeFd(int fd, int *saveErrno);

private:
    char *begin() { return &*buffer_.begin(); }
    const char *begin() const { return &*buffer_.begin(); }

    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                      begin() + writerIndex_,
                      begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};
