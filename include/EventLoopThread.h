#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

#include "Noncopyable.h"
#include "Thread.h"

class EventLoop;

class EventLoopThread : Noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop *startLoop();

private:
    // This function is run in the new thread created by Thread class
    void threadFunc();

    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};