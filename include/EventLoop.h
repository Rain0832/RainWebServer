#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "CurrentThread.h"
#include "TimerQueue.h"
#include "Timestamp.h"
#include "Noncopyable.h"

class Channel;
class Poller;

// Contain 2 modules: Channel and Poller(epoll)
class EventLoop : Noncopyable
{
public:
    // Function object
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // Set up the loop
    void loop();
    // Stop the loop
    void quit();

    Timestamp pollReturnTime() const { return pollRetureTime_; }

    // Run callback function in current loop
    void runInLoop(Functor cb);

    // Put upper call back function cb into queue,
    // wake up the running loop's threadto execute cb
    void queueInLoop(Functor cb);

    // Write data to wakeupFd_(Read event) to wake up the loop thread
    void wakeup();

    // Encapsulation method for Poller
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    // Judge EventLoop object is in thread of itself
    // threadId_ is CurrentThread::tid() when EventLoop is created
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    // Scheduled task: Run at a specific time point
    void runAt(Timestamp timestamp, Functor &&cb)
    {
        timerQueue_->addTimer(std::move(cb), timestamp, 0.0);
    }

    // Scheduled task: Run after a specific time interval
    void runAfter(double waitTime, Functor &&cb)
    {
        Timestamp time(addTime(Timestamp::now(), waitTime));
        runAt(time, std::move(cb));
    }

    // Scheduled task: Run every specific time interval
    void runEvery(double interval, Functor &&cb)
    {
        Timestamp timestamp(addTime(Timestamp::now(), interval));
        timerQueue_->addTimer(std::move(cb), timestamp, interval);
    }

private:
    // Read wakeupFd_ to wake up blocked epoll_wait
    void handleWakeupRead();
    // Call pending callback functions
    void doPendingFunctors();

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_;
    // Quit from the loop
    std::atomic_bool quit_;

    // Record current EventLoop created by which thread
    const pid_t threadId_;

    // Poller return Event Channels time point
    Timestamp pollRetureTime_;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;

    // When mainLoop get new Channel, use poll() to select a subLoop
    // Use wakeupFd_ to wake up subLoop
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    // Return all active channels(events happened)
    ChannelList activeChannels_;

    // Is current loop executing callback functions?
    std::atomic_bool callingPendingFunctors_;
    // Save pending callback functions
    std::vector<Functor> pendingFunctors_;
    // Lock protect the vector container
    std::mutex mutex_;
};