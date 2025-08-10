#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <fcntl.h> // for open
#include <errno.h>
#include <string.h>
#include <unistd.h> // for close

#include <functional>
#include <string>

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include "TcpConnection.h"

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL << " mainLoop is null!";
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop)), name_(nameArg), state_(kConnecting), reading_(true), socket_(new Socket(sockfd)), channel_(new Channel(loop, sockfd)), localAddr_(localAddr), peerAddr_(peerAddr), highWaterMark_(64 * 1024 * 1024) // 64M
{
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    LOG_INFO << "TcpConnection::ctor:[" << name_.c_str() << "]at fd=" << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO << "TcpConnection::dtor[" << name_.c_str() << "]at fd=" << channel_->fd() << "state=" << (int)state_;
}

void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        // Single readctor, user call conn->send(), loop_ is current thread
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else
        {
            loop_->runInLoop(
                std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

// Sent data: Application write fast, but kernel send data slow
// Should write data to buffer, and set watermark callback
void TcpConnection::sendInLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    if (state_ == kDisconnected) // After call TcpConnection::shutdown(), can not write data
    {
        LOG_ERROR << "TCP disconnected, give up writing";
    }

    // Channel write first data or buffer has no data to send
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // Sent all data here, should not set epollout event for channel
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) // EWOULDBLOCK represent not blocked, and no data return(equal to EAGAIN)
            {
                LOG_ERROR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE RESET
                {
                    faultError = true;
                }
            }
        }
    }

    // Last time write not send all data, save remaining data to buffer and register EPOLLOUT event for channel,
    // Poller notify tcp sent buffer has space, and call sock->channel's writeCallback_ to send remaining data,
    // channel's writeCallback_ is TcpConnection's handleWrite,
    // and send all remaining data in outputBuffer_
    if (!faultError && remaining > 0)
    {
        // Data left to be sent in output buffer
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
            channel_->enableWriting(); // Must register channel write event, otherwise poller will not notify channel
        }
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    // Current outputBuffer_ all data has been sent
    if (!channel_->isWriting())
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // Register channel EPOLLIN read event to poller

    // New connection, execute callback
    connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // Remove all interested events from poller
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // Remove channel from poller
}

// Read is relative to the server,
// when the peer client has data arriving,
// the server detects EPOLLIN and triggers the callback on that fd handleRead to read the data sent by the peer.
void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0) // Data arrived
    {
        // Connected user has readable event, call user callback onMessage
        // shared_from_this() gets the smart pointer of TcpConnection
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0) // Client server Connection closed
    {
        handleClose();
    }
    else // With an error
    {
        errno = savedErrno;
        LOG_ERROR << "TcpConnection::handleRead";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n); // Read from buffer Readable area and move readindex
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    // TcpConnection object in the subloop, add callback to pendingFunctors_
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop(); // Delete TcpConnection in current loop
                }
            }
        }
        else
        {
            LOG_ERROR << "TcpConnection::handleWrite";
        }
    }
    else
    {
        LOG_ERROR << "TcpConnection fd=" << channel_->fd() << "is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO << "TcpConnection::handleClose fd=" << channel_->fd() << "state=" << (int)state_;
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // Connect the callback
    closeCallback_(connPtr);      // Execute close callback, execute TcpServer::removeConnection
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR << "TcpConnection::handleError name:" << name_.c_str() << "- SO_ERROR:%" << err;
}

void TcpConnection::sendFile(int fileDescriptor, off_t offset, size_t count)
{
    if (connected())
    {
        // Current thread is loop thread?
        if (loop_->isInLoopThread())
        {
            // If yes, execute sendFileInLoop directly
            sendFileInLoop(fileDescriptor, offset, count);
        }
        else
        {
            // If not, wake up the loop thread to execute sendFileInLoop
            loop_->runInLoop(std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), fileDescriptor, offset, count));
        }
    }
    else
    {
        LOG_ERROR << "TcpConnection::sendFile - not connected";
    }
}

void TcpConnection::sendFileInLoop(int fileDescriptor, off_t offset, size_t count)
{
    ssize_t bytesSent = 0;    // Sent how many bytes?
    size_t remaining = count; // Still need to send how many bytes?
    bool faultError = false;  // Error flag

    if (state_ == kDisconnecting)
    {
        // If connection is disconnecting, give up sending data
        LOG_ERROR << "disconnected, give up writing";
        return;
    }

    // Channel write first data or buffer has no data to send
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        bytesSent = sendfile(socket_->fd(), fileDescriptor, &offset, remaining);
        if (bytesSent >= 0)
        {
            remaining -= bytesSent;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // remaining == 0: data all sent, no need to set write event for channel
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            // bytesSent < 0
            if (errno != EWOULDBLOCK)
            {
                // If not blocked and no data return, it is an error( = EAGAIN)
                LOG_ERROR << "TcpConnection::sendFileInLoop";
            }
            if (errno == EPIPE || errno == ECONNRESET)
            {
                faultError = true;
            }
        }
    }

    // Process remaining data
    if (!faultError && remaining > 0)
    {
        // Sent remaining data in output buffer
        loop_->queueInLoop(std::bind(&TcpConnection::sendFileInLoop, shared_from_this(), fileDescriptor, offset, remaining));
    }
}