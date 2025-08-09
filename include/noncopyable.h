#pragma once

// the inheritance classof noncopyable will make the derived class object
// can be constructed and destructed normally,
// but cannot perform copy construction and assignment construction
class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete; // or void operator=(const noncopyable &) = delete; to forbid the chaining assignment

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};