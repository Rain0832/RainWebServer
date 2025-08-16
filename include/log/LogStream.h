#pragma once
#include <string.h>

#include <string>

#include "FixedBuffer.h"
#include "Noncopyable.h"

class GeneralTemplate : Noncopyable
{
public:
    GeneralTemplate()
        : data_(nullptr), len_(0)
    {
    }

    explicit GeneralTemplate(const char *data, int len)
        : data_(data), len_(len)
    {
    }

    const char *data_;
    int len_;
};

// LogStream use to manage the output stream, overload the output stream operator <<, write various types of values into the internal buffer
class LogStream : Noncopyable
{
public:
    // Define Buffer type using fixed-size buffer
    using Buffer = FixedBuffer<kSmallBufferSize>;

    // Append the specified length of character data to the buffer
    void append(const char *buffer, int len)
    {
        buffer_.append(buffer, len); // Call Buffer's append method
    }

    // Return the constant reference of current buffer
    const Buffer &buffer() const
    {
        return buffer_; // Return current buffer
    }

    // Reset the buffer, set the current pointer to the beginning of the buffer
    void reset_buffer()
    {
        buffer_.reset(); // Call Buffer's reset method
    }

    // Overload the output stream operator << for bool value
    LogStream &operator<<(bool);

    LogStream &operator<<(short);
    LogStream &operator<<(unsigned short);
    LogStream &operator<<(int);
    LogStream &operator<<(unsigned int);
    LogStream &operator<<(long);
    LogStream &operator<<(unsigned long);
    LogStream &operator<<(long long);
    LogStream &operator<<(unsigned long long);

    LogStream &operator<<(float);
    LogStream &operator<<(double);

    LogStream &operator<<(char);
    LogStream &operator<<(const char *);
    LogStream &operator<<(const unsigned char *);
    LogStream &operator<<(const std::string &);
    LogStream &operator<<(const GeneralTemplate &g);

private:
    // Define the maximum number size constant
    static constexpr int kMaxNumberSize = 32;

    // Specialized for integer types
    template <typename T>
    void formatInteger(T num);

    // Internal buffer object
    Buffer buffer_;
};
