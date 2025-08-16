#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ConsistenHash.h"
#include "Noncopyable.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : Noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // If work in multi-threads, baseLoop_ (mainLoop) will distribute Channel to subLoop by poll
    EventLoop *getNextLoop(const std::string &key);

    std::vector<EventLoop *> getAllLoops(); // 获取所有的EventLoop

    // Check if this thread pool has started
    bool started() const { return started_; }

    // Get name of this thread pool
    const std::string name() const { return name_; }

private:
    EventLoop *baseLoop_;                                   // User use muduo create loop if thread number == 1, use user-created loop. else create multi-EventLoop
    std::string name_;                                      // TreadPool name, defined by user, EventLoopThread name depend on thread pool name
    bool started_;                                          // Is thread started
    int numThreads_;                                        // Thread number in thread pool
    int next_;                                              // New connection come, select EventLoop index
    std::vector<std::unique_ptr<EventLoopThread>> threads_; // IO thread list
    std::vector<EventLoop *> loops_;                        // Thread pool's EventLoop list, point to EventLoopThread function create EventLoop object
    ConsistentHash hash_;                                   // Consistent Hash object
};