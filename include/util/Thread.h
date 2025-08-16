#pragma once

#include <unistd.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "Noncopyable.h"

class Thread : Noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;       // Bind when thread created
    ThreadFunc func_; // Thread callback function
    std::string name_;
    static std::atomic_int numCreated_;
};