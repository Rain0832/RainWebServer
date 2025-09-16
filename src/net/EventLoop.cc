#include <unistd.h>

#include <sys/eventfd.h>

#include <errno.h>
#include <fcntl.h>
#include <memory>

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"

// In case of one thread create multiple EventLoop
thread_local EventLoop *t_loopInThisThread = nullptr;

// Default Poller IO multiplexing timeout
const int kPollTimeMs = 10000; // 10000ms = 10s

// Create wakeupfd to notify wakeup subReactor to handle new channel
int createEventfd()
{
    // function signature:
    //  int eventfd(unsigned int initval, int flags);
    //      initval: Timer value
    //      flags:
    //          EFD_NONBLOCK, set socket as non-blocking.
    //          EFD_CLOEXEC, execute fork, parent fd close, child fd keep
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL << "eventfd error: " << errno;
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), callingPendingFunctors_(false), threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)), wakeupFd_(createEventfd()), wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG << "EventLoop created" << this << "in thread" << threadId_;
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop" << t_loopInThisThread << "exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }

    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleWakeupRead, this)); // Set wakeup event type and callback function
    wakeupChannel_->enableReading();                                                // Every EventLoop has to listen wakeupChannel_ EPOLL event
}
EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll(); // Remove all event from channel
    wakeupChannel_->remove();     // Delete Channel from channel
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO << "EventLoop start looping";

    while (!quit_)
    {
        activeChannels_.clear(); ///< Clear last time active channels

        /// Get active channels(event happened) from Poller
        pollRetureTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        for (Channel *channel : activeChannels_)
        {
            /// Poller listen channel that has event, and report to EventLoop to notify channel to handle
            channel->handleEvent(pollRetureTime_);
        }
        /**
         * Call back operation(thread num >= 2, mainloop/mainReactor):
         * accept return connfd => pack connfd to Channel => TcpServer::newConnection allocate TcpConnection to subloop
         * mainloop call queueInLoop to add callback to subloop(callback need subloop execute, but subloop still in poller_->poll)
         */
        doPendingFunctors();
    }
    LOG_INFO << "EventLoopstop looping";
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;

    if (!isInLoopThread())
    {
        // Wake up the mainloop let it quit from poller_->poll()
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        cb();
    }
    else ///< Not in current loop, wake up EventLoop thread to execute callback
    {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    /// if the current loop is not in the thread of itself or if it is executing callback functions,
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup(); ///< Wake up loop thread
    }
}

void EventLoop::handleWakeupRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::handleRead() reads" << n << "bytes instead of 8";
    }
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        LOG_ERROR << "EventLoop::wakeup() writes" << n << "bytes instead of 8";
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
    /// Got todo Callbacks from other threads
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        /// Lock: in case of other threads calling queueInLoop() to add callback function to pendingFunctors_
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); /// Put the pending functors to the local vector
    }

    for (const Functor &functor : functors)
    {
        /// Call corresponding callback function
        functor();
    }

    callingPendingFunctors_ = false;
}
