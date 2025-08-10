#pragma once

#include <atomic>
#include <memory>
#include <string>

#include "Buffer.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "Timestamp.h"
#include "noncopyable.h"

class Channel;
class EventLoop;
class Socket;

// TcpServer => Acceptor => New user connection, get connfd by accept function
// => TcpConnection set callbacks => set to Channel => Poller => Channel callback
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                  const std::string &nameArg,
                  int sockfd,
                  const InetAddress &localAddr,
                  const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddress() const { return localAddr_; }
    const InetAddress &peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    void send(const std::string &buf);
    void sendFile(int fileDescriptor, off_t offset, size_t count);

    // Shutdown connection
    void shutdown();

    void setConnectionCallback(const ConnectionCallback &cb)
    {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb)
    {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    {
        writeCompleteCallback_ = cb;
    }
    void setCloseCallback(const CloseCallback &cb)
    {
        closeCallback_ = cb;
    }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
    {
        highWaterMarkCallback_ = cb;
        highWaterMark_ = highWaterMark;
    }

    // TcpConnection Established
    void connectEstablished();

    // TcpConnection Destroyed
    void connectDestroyed();

private:
    enum StateE
    {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *data, size_t len);
    void shutdownInLoop();
    void sendFileInLoop(int fileDescriptor, off_t offset, size_t count);
    EventLoop *loop_;        // Depends on TcpServer thread number, if multiReactor -> subloop, or if singleReactor -> baseloop
    const std::string name_; // TcpServer distribute connection name
    std::atomic_int state_;  // Connection state
    bool reading_;           // If connection is listening to read events

    // Socket Channel is simmilar to Acceptor
    // Acceptor => mainloop    TcpConnection => subloop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    // User register callback to TcpServer, TcpServer pass callback to TcpConnection, TcpConnection register callback to Channel
    ConnectionCallback connectionCallback_;       // New connection callback
    MessageCallback messageCallback_;             // Read-Write message callback
    WriteCompleteCallback writeCompleteCallback_; // Message sent complete callback
    HighWaterMarkCallback highWaterMarkCallback_; // Buffer data high water mark callback
    CloseCallback closeCallback_;                 // Close connection callback
    size_t highWaterMark_;                        // Buffer data high water mark

    // Data buffer
    Buffer inputBuffer_;  // Receive data buffer
    Buffer outputBuffer_; // Send data buffer, user send data to outputBuffer_
};
