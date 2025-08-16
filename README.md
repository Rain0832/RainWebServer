# RainWebServer

## Introduction
This is a high-performance WEB server implemented by C++.
It is based on the design ideas of:
- [RainMuduo](https://github.com/Rain0832/RainModuo.git) The muduo library with most core codes
- Multi-Threaded Multi-Reactor Network Model

It is further enhanced with the：
- [RainMemoPool](https://github.com/Rain0832/RainMemoPool.git): a three-level memory pool module
- [RainLog](https://github.com/Rain0832/RainLog.git): a double-buffer asynchronous log system
- [RainCache](https://github.com/Rain0832/RainCache.git): a LfuCache cache module

## Development Environment
* WSL Version: 2.4.12.0
* Windows Version: 10.0.26100.4946
* Linux version 5.15.167.4-microsoft-standard-WSL2 (root@f9c826d3017f) 
* gcc (GCC) 11.2.0, GNU ld (GNU Binutils) 2.37 #1 SMP Tue Nov 5 00:21:55 UTC 2024
* Destribution: Ubuntu 24.04.2 LTS
* cmake version 4.0.2

## Directory Structure
```shell
RainWebServer
├── CMakeLists.txt      # Project main CMakeLists.txt file
├── Doxyfile            # Project Doxygen configuration file
├── LICENSE             # Project license file
├── README.md           # Project introduction file
├── docs                # Project documentation directory
├── img                 # Project image directory (be used in README.md)
├── include             # Project header files directory
│   ├── log             # RainLog modul header files directory
│   ├── memory          # RainMemoPool modul header files directory
│   ├── net             # Network modul header files directory
│   └── util            # Utility header files directory
├── lib                 # Project library directory
└── src                 # Project source files directory
    ├── log             # RainLog modul source files directory
    ├── main.cc         # Main program source file
    ├── memory          # RainMemoPool modul source files directory
    ├── net             # Network modul source files directory
    └── util            # Utility source files directory
```

## Preparation
Install basic tools:

```bash
sudo apt-get update
sudo apt-get install -y wget cmake build-essential unzip git
```

## Build Instruction
1. Clone the project
```bash
   git clone https://github.com/Rain0832/RainWebServer.git
   cd RainWebServer
```

2. Create build directory and compile
```bash
   mkdir build &&
   cd build &&
   cmake .. &&
   make -j ${nproc}
```

3. Enter the bin directory
```bash
cd bin
```

4. Run the executable program
```bash
./main 
```

**NOTE**: You need to run `nc 127.0.0.1 8080` in another terminal to start the client to link the web server started by the main executable program.

## Run Result
After running the executable program `main` in the `bin` directory, the following result will be displayed:

The log file will be saved in the `logs` directory under the `bin` file. Each time the program is run, a new log file is generated, which records the running status and error information of the program.

- Server running result

![img](./img/1.png)

- Client running result
 
![img](./img/2.png)

**NOTE**: The test result is still based on the echo server, focusing on the implementation of the architecture.

### Log File Analysis
![img](./img/3.png)

1. File Descriptor Statistics
```bash
2025/01/24 17:40:240290 INFO  fd total count:1 - EPollPoller.cc:32
```

- EPoll current managing file descriptor count: 1.

2. Event Handling
```bash
2025/01/24 17:40:454794 INFO  %d events happend1 - EPollPoller.cc:40
2025/01/24 17:40:454852 INFO  channel handleEvent revents:1 - Channel.cc:73
```

- An event occurs (events happend1), which may be the closing of the client socket.
- revents:1 indicates that the triggered event type is EPOLLIN, which means that the opposite end has closed the connection or sent data.

3. Connection closed handling
```bash
2025/01/24 17:40:454890 INFO  TcpConnection::handleClose fd=13state=2 - TcpConnection.cc:241
2025/01/24 17:40:454907 INFO  func =>fd13events=0index=1 - EPollPoller.cc:66
2025/01/24 17:40:454929 INFO  Connection DOWN :127.0.0.1:47376 - main.cc:44
```

- TcpConnection::handleClose: The server handles the closure of the connection, where fd=13 is the file descriptor for the client connection.
- Connection DOWN: The connection with the client 127.0.0.1:47376 is down.
- events=0: Indicates that the file descriptor is no longer listening for any events.

4. Remove connection from server
```bash 
2025/01/24 17:40:455009 INFO  TcpServer::removeConnectionInLoop [EchoServer] - connection %sEchoServer-127.0.0.1:8080#1 - TcpServer.cc:114
2025/01/24 17:40:455138 INFO  removeChannel fd=13 - EPollPoller.cc:102
```
- TcpServer::removeConnectionInLoop: The server removes the connection from its internal management, where 127.0.0.1:47376 is the client connection.
- removeChannel: The file descriptor fd=13 is removed from the EPoll event listening list.

5. Resource release
```bash 
2025/01/24 17:40:455155 INFO  TcpConnection::dtor[EchoServer-127.0.0.1:8080#1]at fd=13state=0 - TcpConnection.cc:58
```
- Call TcpConnection::dtor: The server releases the connection's related resources, where fd=13 is the file descriptor for the client connection.
- state=0: Indicates that the connection has been completely closed, and the file descriptor fd=13 has been destroyed.


## Functionality Module

### Network Module
- **Event Loop Polling Module && Event Dispatching**: `EventLoop.*`, `Channel.*`, `Poller.*`, `EPollPoller.*` Responsible for event loop polling and event dispatching. `EventLoop` responsible for event loop management, `Poller`Responsible for event loop polling, `Channel`Responsible for event dispatching,`EPollPoller` implements the epoll-based event loop model.
- **Thread and Event Loop Binding**：`Thread.*`, `EventLoopThread.*`, `EventLoopThreadPool.*` Responsible for binding threads and event loops, achieving the `one loop per thread` model.
- **Network Connection Module**: `TcpServer.*`, `TcpConnection.*`, `Acceptor.*`, `Socket.*` Implement the `mainloop` response to network connections, and distribute them to `subloop`.
- **Buffer Module**: `Buffer.*` Provide automatic expansion buffer, ensuring data is received in order.

### Logger Module
- The logger module is responsible for recording important information during the running of the server, which helps developers for debugging and performance analysis. The log file is saved in the `bin/logs/` directory.

### Memory Module
- The memory management module is responsible for dynamic memory allocation and release, ensuring the stability and performance of the server under high load.

### LFU Cache Module
- The LfuCache module is used to determine which content to delete when the cache capacity is insufficient. The core idea of LFU is to remove the cache item with the lowest usage frequency.

### Utility Classes Module
- C++ server development common tools class.

## Contribution
Welcome to any form of contribution! Please submit issues, suggestions or code requests.
