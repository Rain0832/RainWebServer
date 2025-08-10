#pragma once

#include <functional>

#include "Channel.h"
#include "Noncopyable.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

class Acceptor : Noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;

    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    // Set new connection callback function
    void setNewConnectionCallback(const NewConnectionCallback &cb) { NewConnectionCallback_ = cb; }

    // Judge whether the Acceptor is listening
    bool listenning() const { return listenning_; }

    // Listen local port
    void listen();

private:
    void handleRead(); // Process new user connection event

    EventLoop *loop_;                             // Acceptor use user-defined baseLoop, also mainLoop
    Socket acceptSocket_;                         // Accept new connection socket
    Channel acceptChannel_;                       // Use to listen connection
    NewConnectionCallback NewConnectionCallback_; // New connection callback
    bool listenning_;                             // is listening now?
};