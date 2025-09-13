#pragma once

#include <stddef.h>

#include <algorithm>
#include <string>
#include <vector>

class Buffer
{
public:
    /// A prependable space, reserve 8 bytes before buffer inital size data
    static const size_t kCheapPrepend = 8;
    /// The initial buffer size
    static const size_t kInitialSize = 1024;

    /**
     * @brief Construct a new Buffer object
     * @details Here we initialize buffer_ with kCheapPrepend + initalSize bytes,
     * by reserving kCheapPrepend bytes before the initial size data,
     * so that we can prepend some data before the initial size data if necessary.
     * (we can wrap the data with like TCP/UDP/HTTP header)
     *
     */
    explicit Buffer(size_t initalSize = kInitialSize)
        : buffer_(kCheapPrepend + initalSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend)
    {
    }

    /**
     * @brief Return the number of bytes that can be read from the buffer(size)
     * @details It can caculate by the writeIndex_ and readIndex_,
     * writeIndex means the position first byte where we can write data,
     * readIndex means the position first byte where we can read data.
     */
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }

    /**
     * @brief Return the number of bytes that can be written to the buffer(size)
     * @details It can caculate by the buffer size and writeIndex_,
     * buffer size means the total size of buffer,
     * writeIndex means the position first byte where we can write data.
     */
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }

    /**
     * @brief Return the number of bytes that can be prepend to the buffer(size)
     * @details It just return the readerIndex_ value,
     * the position before readerIndex_ inthe buffer can be used to prepend data.
     */
    size_t prependableBytes() const { return readerIndex_; }

    /**
     * @brief Return buffer readable data start address(pointer)
     */
    const char *peek() const { return begin() + readerIndex_; }

    /**
     * @brief Retrieve len bytes readbale data from buffer
     * @details It will judge the is the readable data is enough to retrieve.
     */
    void retrieve(size_t len)
    {
        if (len < readableBytes())
        {
            /// Actually just move the readerIndex_
            readerIndex_ += len;
        }
        else ///< len >= readableBytes()
        {
            retrieveAll();
        }
    }

    /**
     * @brief Retrieve all readable data from buffer
     * @details Here we reset buffer readerIndex_ and writerIndex_ to kCheapPre
     */
    void retrieveAll()
    {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    /**
     * @brief Retrieve all readable data from buffer and return as string type data
     */
    std::string retrieveAllAsString() { return retrieveAsString(readableBytes()); }

    /**
     * @brief Retrieve len bytes readable data from buffer and return as string type data
     * @details First we copy len bytes data from buffer to string,
     * use peek() to get the start address,combine with len to construct a string
     * Then we move the readerIndex_ to len backward
     */
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    /**
     * @brief Prepend len bytes data to buffer
     * @details If the left space of buffer is not enough to prepend len bytes data,
     * we will expand the buffer to make space.
     */
    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
    }

    /**
     * @brief Append len bytes data to buffer
     */
    void append(const char *data, size_t len)
    {
        ensureWritableBytes(len);
        /// Need start and end address of data, and start address of destination
        std::copy(data, data + len, beginWrite());
        writerIndex_ += len;
    }

    /**
     * @brief Return a writable address(pointer) of buffer
     */
    char *beginWrite() { return begin() + writerIndex_; }

    /**
     * @brief Return a const writable address(pointer) of buffer
     */
    const char *beginWrite() const { return begin() + writerIndex_; }

    /**
     * @brief Read data from fd to buffer
     * @details First we define a 64KB stack extra space to store temporary data,
     * Second define iovec vec to store buffer_ writable space and extrabuf
     * Third use readv to read data from fd to buffer_ or extrabuf,
     * Last use saveErrno to store the errno value if readv failed.
     */
    ssize_t readFd(int fd, int *saveErrno);

    /**
     * @brief Send data from buffer to fd
     * @details Call ::write to send data from buffer_ to fd,
     */
    ssize_t writeFd(int fd, int *saveErrno);

private:
    /**
     * @brief Return the start address of buffer
     */
    char *begin() { return &*buffer_.begin(); }

    /**
     * @brief Return the const start address of buffer
     */
    const char *begin() const { return &*buffer_.begin(); }

    /**
     * @brief Expand the buffer to make space for len bytes data
     * @details If the left space of buffer is not enough to prepend len bytes data,
     * we will expand the buffer to make space.
     * Else we will move the readable data to the front of buffer,
     * and reset the readerIndex_ and writerIndex_ to kCheapPrepend.
     */
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

    std::vector<char> buffer_; ///< The buffer data
    size_t readerIndex_;       ///< The position first byte where we can read data
    size_t writerIndex_;       ///< The position first byte where we can write data
};
