#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <unistd.h>

#include "Acceptor.h"
#include "InetAddress.h"
#include "Logger.h"

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL << "listen socket create err " << errno;
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop), acceptSocket_(createNonblocking()), acceptChannel_(loop, acceptSocket_.fd()), listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);

    // TcpServer::start() => Acceptor.listen() if new user connection, execute a callback (accept => connfd => pack into Channel => wake up subloop)
    // baseloop listens to an event => acceptChannel_(listenfd) => execute the callback function
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll(); // Remove interest in events from Poller
    acceptChannel_.remove();     // Call EventLoop->removeChannel => Poller->removeChannel to remove the corresponding part of the ChannelMap
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();         // listen
    acceptChannel_.enableReading(); // acceptChannel_ register to Poller !!! important !!!
}

// listenfd have event happened - new user connection
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (NewConnectionCallback_)
        {
            NewConnectionCallback_(connfd, peerAddr); // Poller find subLoop, wake up and dispatch new client Channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR << "accept Err";
        if (errno == EMFILE)
        {
            LOG_ERROR << "sockfd reached limit";
        }
    }
}