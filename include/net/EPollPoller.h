#pragma once

#include <sys/epoll.h>
#include <vector>

#include "Poller.h"
#include "Timestamp.h"

class Channel;

/**
 * Usage of epoll:
 * 1. epoll_create
 * 2. epoll_ctl (add, mod, del)
 * 3. epoll_wait
 **/
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    /**
     * @brief Poll IO events, return the active channels
     * @details 1. Call epoll_wait() to wait IO events
     *          2. Handle the return values: numEvents
     *          3. Use LOG_DEBUG and LOG_ERROR to record the debug and error information
     *          4. Call fillActiveChannels() put active channels into activeChannels
     *          5. Return the current timestamp
     */
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;

    /**
     * @brief Update the channel
     * @details 1. Record channel fd, events, and index
     *          2. Handle new channel(add to map) or delete channel
     *          3. Handle registered channel(mod)
     */
    void updateChannel(Channel *channel) override;

    /**
     * @brief Remove the channel
     * @details 1. Erase the channel from the map
     *          2. Call update() EPOLL_CTL_DEL signal to delete the channel
     */
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;

    /**
     * @brief Fill active channels
     * @details 1. Set numEvents to the channel
     *          2. Add the channel to activeChannels
     * @note Use const keyword to avoid modifying the member variables
     */
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;

    /**
     * @brief Update epoll_event(add/mod/del)
     * @details 1. A more base function to update, actually we use a fd to update the epoll_event
     *          2. Can add/mod/del a channel
     */
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    /// Save the epoll_create() return fd
    int epollfd_;

    /// Save the epoll_wait() return events fd list
    EventList events_;
};