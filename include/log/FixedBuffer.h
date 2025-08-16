#pragma once

#include <string.h>

#include <string>

#include "Noncopyable.h"

class AsyncLogging;
constexpr int kSmallBufferSize = 4000;
constexpr int kLargeBufferSize = 4000 * 1000;

// Use in manage log data storage
// Provide a fixed-size buffer to append data to the buffer and provide related operations
template <int buffer_size>
class FixedBuffer : Noncopyable
{
public:
    // Initialize the current pointer to the beginning of the buffer
    FixedBuffer()
        : cur_(data_), size_(0)
    {
    }

    // Append data of specified length to the buffer
    // If there is enough available space in the buffer,
    // copy the data to the current pointer position and update the current pointer
    void append(const char *buf, size_t len)
    {
        if (avail() > len)
        {
            memcpy(cur_, buf, len); // Copy data to the current pointer position
            add(len);
        }
    }

    // Return the starting address of the buffer
    const char *data() const { return data_; }

    // Return the length of the valid data in the buffer
    int length() const { return size_; }

    // Return the current pointer position
    char *current() { return cur_; }

    // Return the available space size in the buffer
    size_t avail() const { return static_cast<size_t>(buffer_size - size_); }

    // Update the current pointer, increase the specified length
    void add(size_t len)
    {
        cur_ += len;
        size_ += len;
    }

    // Reset the current pointer to the beginning of the buffer
    void reset()
    {
        cur_ = data_;
        size_ = 0;
    }

    // Clear the data in the buffer
    void bzero() { ::bzero(data_, sizeof(data_)); }

    // Convert the data in the buffer to std::string type and return
    std::string toString() const { return std::string(data_, length()); }

private:
    char data_[buffer_size]; // Define a fixed-size buffer
    char *cur_;              // Current pointer, pointing to the next available position in the buffer
    int size_;               // Buffer size
};
