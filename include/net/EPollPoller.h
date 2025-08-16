#pragma once

#include <sys/epoll.h>
#include <vector>

#include "Poller.h"
#include "Timestamp.h"

/**
 * Usage of epoll:
 * 1. epoll_create
 * 2. epoll_ctl (add, mod, del)
 * 3. epoll_wait
 **/

class Channel;

class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // Rewrite the abstract method of Poller
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;

    // Update channel from poller
    void updateChannel(Channel *channel) override;
    // Remove channel from poller
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16;

    // Fill active channels
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // Update the channel, by calling epoll_ctl
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    // Save the epoll_create() return fd
    int epollfd_;
    // Save the epoll_wait() return events fd list
    EventList events_;
};