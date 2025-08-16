#pragma once

#include "Noncopyable.h"

class InetAddress;

// Conpsulate the socket fd and provide some useful functions
class Socket : Noncopyable
{
public:
    explicit Socket(int sockfd)
        : sockfd_(sockfd)
    {
    }
    ~Socket();

    int fd() const { return sockfd_; }

    // Bind socket to local address
    void bindAddress(const InetAddress &localaddr);

    // Start listening for incoming connections
    void listen();

    // Accept a client connection, return new socket fd
    int accept(InetAddress *peeraddr);

    // Can not write, but can read
    void shutdownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};
