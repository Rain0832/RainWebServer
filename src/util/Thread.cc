#include <semaphore.h>

#include "CurrentThread.h"
#include "Thread.h"

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach();
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0); // false: Not shared, 0: Initial value
    // Start new thread
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
                                                           {
                                                               tid_ = CurrentThread::tid(); // Get thread id
                                                               sem_post(&sem);
                                                               func_(); // Start new thread, and run the callback function
                                                           }));

    // Wait for thread to start and get its tid
    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}
