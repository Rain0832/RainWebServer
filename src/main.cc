#include <sys/stat.h>

#include <sstream>
#include <string>

#include "AsyncLogging.h"
#include "LFU.h"
#include "Logger.h"
#include "TcpServer.h"
#include "memoryPool.h"

// Logfile RollSize
static const off_t kRollSize = 1 * 1024 * 1024;

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const InetAddress &addr, const std::string &name) : server_(loop, addr, name), loop_(loop)
    {
        // Register the server to the event loop(Callback function)
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        // Set the number of subloop threads
        server_.setThreadNum(3);
    }
    void start()
    {
        server_.start();
    }

private:
    // New connection established
    // or
    // Existing connection broken
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO << "Connection UP :" << conn->peerAddress().toIpPort().c_str();
        }
        else
        {
            LOG_INFO << "Connection DOWN :" << conn->peerAddress().toIpPort().c_str();
        }
    }

    // Readable/Writeable event callback
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
    {
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
    }
    TcpServer server_;
    EventLoop *loop_;
};

AsyncLogging *g_asyncLog = NULL;
AsyncLogging *getAsyncLog()
{
    return g_asyncLog;
}

void asyncLog(const char *msg, int len)
{
    AsyncLogging *logging = getAsyncLog();
    if (logging)
    {
        logging->append(msg, len);
    }
}

int main(int argc, char *argv[])
{
    // 1. Set up log system, double buffering asynchronous write to disk
    const std::string LogDir = "logs";
    mkdir(LogDir.c_str(), 0755);
    std::ostringstream LogfilePath;
    LogfilePath << LogDir << "/" << ::basename(argv[0]);
    AsyncLogging log(LogfilePath.str(), kRollSize);
    g_asyncLog = &log;
    Logger::setOutput(asyncLog);
    log.start();

    // 2. Set up memory pool and LFU cache
    memoryPool::HashBucket::initMemoryPool();
    const int CAPACITY = 5;
    KamaCache::KLfuCache<int, std::string> lfu(CAPACITY);

    // 3. Set up the network module
    EventLoop loop;
    InetAddress addr(8080);
    EchoServer server(&loop, addr, "EchoServer");
    server.start();

    // 4. Main loop starts the event loop
    std::cout << "================================================Start Web Server================================================" << std::endl;
    loop.loop();
    std::cout << "================================================Stop Web Server=================================================" << std::endl;

    // 5. Stop log system
    log.stop();
}