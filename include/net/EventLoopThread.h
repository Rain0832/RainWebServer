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

    /**
     * @brief Start the thread and run the event loop.
     * @details This function will create a new thread and run the event loop in it.
     *          The event loop will be returned to the caller.
     */
    EventLoop *startLoop();

private:
    /**
     * @brief The function that will be run in the new thread.
     */
    void threadFunc();

    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};