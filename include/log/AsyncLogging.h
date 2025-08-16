#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

#include "FixedBuffer.h"
#include "LogFile.h"
#include "LogStream.h"
#include "Thread.h"
#include "noncopyable.h"

class AsyncLogging
{
public:
    AsyncLogging(const std::string &basename, off_t rollSize, int flushInterval = 3);
    ~AsyncLogging()
    {
        if (running_)
        {
            stop();
        }
    }

    // Front-End Interface
    void append(const char *logline, int len);
    void start()
    {
        running_ = true;
        thread_.start();
    }
    void stop()
    {
        running_ = false;
        cond_.notify_one();
    }

private:
    using LargeBuffer = FixedBuffer<kLargeBufferSize>;
    using BufferVector = std::vector<std::unique_ptr<LargeBuffer>>;
    // BufferVector::value_type -> std::vector<std::unique_ptr<Buffer>> -> std::unique_ptr<Buffer>
    using BufferPtr = BufferVector::value_type;

    void threadFunc();

    // Flush for LogFile
    const int flushInterval_;
    std::atomic<bool> running_;
    const std::string basename_;
    const off_t rollSize_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;

    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;
    BufferVector buffers_;
};