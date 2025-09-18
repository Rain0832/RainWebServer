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

    /**
     * @brief Set new connection callback function
     */
    void setNewConnectionCallback(const NewConnectionCallback &cb) { NewConnectionCallback_ = cb; }

    /**
     * @brief Judge whether the Acceptor is listening
     */
    bool listening() const { return listening_; }

    /**
     * @brief Start listening local port
     */
    void listen();

private:
    /**
     * @brief Handle new connection event
     */
    void handleRead();

    EventLoop *loop_;                             ///< Acceptor use user-defined baseLoop, also mainLoop
    Socket acceptSocket_;                         ///< Accept new connection socket
    Channel acceptChannel_;                       ///< Use to listen connection
    NewConnectionCallback NewConnectionCallback_; ///< New connection callback
    bool listening_;                              ///< is it listening now?
};