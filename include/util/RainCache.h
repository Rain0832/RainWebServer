#pragma once

namespace RainCache
{
  template <typename Key, typename Value>
  class RainCache
  {
    // 缓存基类
  public:
    // 析构函数
    virtual ~RainCache() {};

    // 缓存接口 添加
    virtual void put(Key key, Value value) = 0;

    // 缓存接口 查询 传出参数
    // 传出成功返回 true
    virtual bool get(Key key, Value &value) = 0;

    // 缓存接口 查询 返回值
    virtual Value get(Key key) = 0;
  };
} // namespace RainCache