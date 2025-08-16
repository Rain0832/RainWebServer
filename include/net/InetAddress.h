#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// Capsulation of socket address type
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr)
    {
    }

    // Convert binary IP address to string format
    std::string toIp() const;
    // Convert binary IP address and port to string format
    std::string toIpPort() const;
    // Get port number
    uint16_t toPort() const;

    const sockaddr_in *getSockAddr() const { return &addr_; }
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

private:
    sockaddr_in addr_;
};