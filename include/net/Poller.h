#pragma once

#include <unordered_map>
#include <vector>

#include "Timestamp.h"
#include "Noncopyable.h"

class Channel;
class EventLoop;

//
/**
 * @brief Multi-Channel Event Dispatcher I/O Multiplexing Module
 * @note this is the muduo Core module
 */
class Poller
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    /**
     * @brief Poll event, wait a specified time, and add ready channels to the activeChannels list
     */
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;

    /**
     * @brief Add/Update a channel to the poller
     */
    virtual void updateChannel(Channel *channel) = 0;

    /**
     * @brief Remove a channel from the poller
     */
    virtual void removeChannel(Channel *channel) = 0;

    /**
     * @brief Check if the channel is in the current poller
     */
    bool hasChannel(Channel *channel) const;

    /**
     * @brief Create a default poller object based on the current system
     */
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    /// Map Key: socket file descriptor, Value: corresponding channel object
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    /// Poller event loop
    EventLoop *ownerLoop_;
};