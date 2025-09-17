#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>

#include <string>

/**
 * @brief Capsulation of socket address type
 */
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit InetAddress(const sockaddr_in &addr)
        : addr_(addr)
    {
    }

    /**
     * @brief Convert binary IP address to string format
     */
    std::string toIp() const;

    /**
     * @brief Convert binary IP address and port to string format
     */
    std::string toIpPort() const;

    /**
     * @brief Convert port from network byte order to host byte order, and Get binary IP address
     */
    uint16_t toPort() const;

    /**
     * @brief Get binary IP address and port
     */
    const sockaddr_in *getSockAddr() const { return &addr_; }

    /**
     * @brief Set binary IP address and port
     */
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }

private:
    sockaddr_in addr_;
};