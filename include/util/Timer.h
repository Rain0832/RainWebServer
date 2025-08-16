#ifndef TIMER_H
#define TIMER_H

#include <functional>

#include "Timestamp.h"
#include "Noncopyable.h"

class Timer : Noncopyable
{
public:
    using TimerCallback = std::function<void()>;

    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(move(cb)),
          expiration_(when),
          interval_(interval),
          repeat_(interval > 0.0)
    {
    }

    void run() const
    {
        callback_();
    }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }

    void restart(Timestamp now);

private:
    const TimerCallback callback_; // Timer call back function
    Timestamp expiration_;         // The next expiration time
    const double interval_;        // Time out interval, if one-time timer, is 0
    const bool repeat_;            // Whether it is repeated (false means one-time timer)
};

#endif // TIMER_H