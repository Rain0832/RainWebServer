#include "EventLoop.h"
#include "EventLoopThread.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name), mutex_(), cond_(), callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // Start bottom level Thread object thread_

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]()
                   { return loop_ != nullptr; });
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc()
{
    // Create a independent EventLoop object for this thread
    // One EventLoop object per thread
    EventLoop loop;

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop(); // Execute EventLoop::loop(), set up bottom level Poller poll()
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}