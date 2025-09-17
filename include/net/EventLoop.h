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

/**
 * @brief The heart of the server: Contain 2 modules: Channel and Poller(epoll)
 */
class EventLoop : Noncopyable
{
public:
    /// Function object data structure
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    /**
     * @brief Set up the loop
     * @details 1. Set looping_ to true, quit_ to false
     *          2. Clear the activeChannels_ container
     *          3. Call poller_->poll() to get activeChannels_
     *          4. Update pollRetureTime_ to current time point
     *          5. Call doPendingFunctors() to execute pending callback functions
     */
    void loop();

    /**
     * @brief Quit the loop
     * @details 1. If called in the self thread: thread has finished executing the loop() function, the poller_->poll has exited.
     *          2. If called in the other thread: needs to wake up the epoll_wait of the thread of the EventLoop
     *          3. For example, when subloop(worker) calls mainloop(IO)'s quit(), needs to wake up the mainloop(IO)'s poller_->poll to execute the loop() function.
     * @note In normal cases,
     *       the mainloop(IO) is responsible for requesting connections and the callback function is written to the subloop(worker) by the producer-consumer model.
     */
    void quit();

    /**
     * @brief Get the current time point of the loop
     * @details The time point is the time point when the last poll() function returns.
     */
    Timestamp pollReturnTime() const { return pollRetureTime_; }

    /**
     * @brief Run a callback function in the loop thread
     * @details If the current thread is the loop thread, the callback function is executed directly.
     *          Else, the callback function is put into the pending callback functions container and the loop thread is woken up.
     */
    void runInLoop(Functor cb);

    /**
     * @brief Queue a callback function to be executed in the loop thread
     * @details 1. Put callback function into the pending callback functions container
     *          2. If the current thread is the loop thread, call doPendingFunctors() to execute the callback function.
     */
    void queueInLoop(Functor cb);

    /**
     * @brief Wake up the loop thread
     * @details Write a byte to the wakeupFd_ to wake up the loop thread blocked in epoll_wait
     */
    void wakeup();

    /**
     * @brief Update the channel in the poller
     */
    void updateChannel(Channel *channel);

    /**
     * @brief Remove the channel from the poller
     */
    void removeChannel(Channel *channel);

    /**
     * @brief Check if the channel is in the activeChannels_ container
     */
    bool hasChannel(Channel *channel);

    /**
     * @brief Check if the current thread is the loop thread
     * @details threadId_ is CurrentThread::tid() when EventLoop is created
     */
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    /**
     * @brief Schedule a callback function to be executed at a specific time point
     */
    void runAt(Timestamp timestamp, Functor &&cb)
    {
        timerQueue_->addTimer(std::move(cb), timestamp, 0.0);
    }

    /**
     * @brief Schedule a callback function to be executed after a specific time interval
     */
    void runAfter(double waitTime, Functor &&cb)
    {
        Timestamp time(addTime(Timestamp::now(), waitTime));
        runAt(time, std::move(cb));
    }

    /**
     * @brief Schedule a callback function to be executed every specific time interval
     */
    void runEvery(double interval, Functor &&cb)
    {
        Timestamp timestamp(addTime(Timestamp::now(), interval));
        timerQueue_->addTimer(std::move(cb), timestamp, interval);
    }

private:
    /**
     * @brief Handle the wakeup event
     */
    void handleWakeupRead();

    /**
     * @brief Call pending callback functions
     */
    void doPendingFunctors();

    using ChannelList = std::vector<Channel *>;

    std::atomic_bool looping_;
    std::atomic_bool quit_;

    /// Record current EventLoop created by which thread
    const pid_t threadId_;

    /// Poller return Event Channels time point
    Timestamp pollRetureTime_;
    std::unique_ptr<Poller> poller_;
    std::unique_ptr<TimerQueue> timerQueue_;

    /**
     * When mainLoop get new Channel,
     * use poll() to select a subLoop,
     * use wakeupFd_ to wake up subLoop
     */
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    /// Return all active channels(events happened)
    ChannelList activeChannels_;

    /// Is current loop executing callback functions?
    std::atomic_bool callingPendingFunctors_;

    /// Save pending callback functions
    std::vector<Functor> pendingFunctors_;

    /// Lock protect the vector container
    std::mutex mutex_;
};