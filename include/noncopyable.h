#pragma once

// the inheritance class of Noncopyable will make the derived class object
// can be constructed and destructed normally,
// but cannot perform copy construction and assignment construction
class Noncopyable
{
public:
    Noncopyable(const Noncopyable &) = delete;
    Noncopyable &operator=(const Noncopyable &) = delete; // or void operator=(const noncopyable &) = delete; to forbid the chaining assignment

protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
};