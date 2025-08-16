#pragma once

#include <unordered_map>
#include <vector>

#include "Timestamp.h"
#include "Noncopyable.h"

class Channel;
class EventLoop;

// muduo Core
// Multi-Channel Event Dispatcher I/O Multiplexing Module
class Poller
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // Abstract interface for poller
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // Check if the channel is in the current poller
    bool hasChannel(Channel *channel) const;

    // EventLoop can get the default IO multiplexing implementation through this interface
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    // Key: socket file descriptor
    // Value: corresponding channel object
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    // Poller event loop
    EventLoop *ownerLoop_;
};