#pragma once

#include <cmath>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "RainCache.h"

namespace RainCache
{
  // 前向声明
  template <typename Key, typename Value>
  class RainLfu;

  // 频率链表类
  template <typename Key, typename Value>
  class FreqList
  {
  private:
    struct Node
    {
      int freq; // 访问频次
      Key key;
      Value value;
      std::weak_ptr<Node> pre; // 上一结点改为weak_ptr打破循环引用
      std::shared_ptr<Node> next;

      // 无参数构造函数
      Node()
          : freq(1),
            next(nullptr)
      {
      }

      // 有参构造函数
      explicit Node(Key key, Value value)
          : freq(1),
            key(key),
            value(value),
            next(nullptr)
      {
      }
    };

    using NodePtr = std::shared_ptr<Node>;
    int freq_;     // 访问频率
    NodePtr head_; // 虚拟头结点
    NodePtr tail_; // 虚拟尾结点

  public:
    // 频率列表构造函数
    explicit FreqList(int n)
        : freq_(n)
    {
      head_ = std::make_shared<Node>();
      tail_ = std::make_shared<Node>();
      head_->next = tail_;
      tail_->pre = head_;
    }

    // 列表是否为空
    bool isEmpty() const
    {
      return head_->next == tail_;
    }

    // 添加节点
    void addNode(NodePtr node)
    {
      if (!node || !head_ || !tail_)
        return;

      node->pre = tail_->pre;
      node->next = tail_;
      tail_->pre.lock()->next = node; // 使用lock()获取shared_ptr
      tail_->pre = node;
    }

    // 删除节点
    void removeNode(NodePtr node)
    {
      if (!node || !head_ || !tail_)
        return;
      if (node->pre.expired() || !node->next)
        return;

      auto pre = node->pre.lock(); // 使用lock()获取shared_ptr
      pre->next = node->next;
      node->next->pre = pre;
      node->next = nullptr; // 确保显式置空next指针，彻底断开节点与链表的连接
    }

    // 取头部节点
    NodePtr getFirstNode() const
    {
      return head_->next;
    }

    friend class RainLfu<Key, Value>;
  };

  // Lfu 类，继承抽象基类
  template <typename Key, typename Value>
  class RainLfu : public RainCache<Key, Value>
  {

  public:
    using Node = typename FreqList<Key, Value>::Node;
    using NodePtr = std::shared_ptr<Node>;
    using NodeMap = std::unordered_map<Key, NodePtr>;

    explicit RainLfu(int capacity, int maxAverageNum = 1000000)
        : capacity_(capacity),
          minFreq_(INT8_MAX),
          maxAverageNum_(maxAverageNum),
          curAverageNum_(0),
          curTotalNum_(0)
    {
    }

    ~RainLfu() override = default;

    // 存入缓存
    void put(Key key, Value value) override
    {
      if (capacity_ == 0)
        return;

      std::lock_guard<std::mutex> lock(mutex_);
      auto it = nodeMap_.find(key);
      if (it != nodeMap_.end())
      {
        // 重置其value值
        it->second->value = value;
        // 找到了直接调整就好了，不用再去get中再找一遍，但其实影响不大
        getInternal(it->second, value);
        return;
      }

      putInternal(key, value);
    }

    // get 接口，传出参数
    bool get(Key key, Value &value) override
    {
      std::lock_guard<std::mutex> lock(mutex_);
      auto it = nodeMap_.find(key);
      if (it != nodeMap_.end())
      {
        getInternal(it->second, value);
        return true;
      }

      return false;
    }

    // get 接口，返回值
    Value get(Key key) override
    {
      Value value;
      get(key, value);
      return value;
    }

    // 清空缓存,回收资源
    void purge()
    {
      nodeMap_.clear();
      freqToFreqList_.clear();
    }

  private:
    // 添加缓存
    void putInternal(Key key, Value value)
    {
      // 如果不在缓存中，则需要判断缓存是否已满
      if (nodeMap_.size() == capacity_)
      {
        // 缓存已满，删除最不常访问的结点，更新当前平均访问频次和总访问频次
        kickOut();
      }

      // 创建新结点，将新结点添加进入，更新最小访问频次
      NodePtr node = std::make_shared<Node>(key, value);
      nodeMap_[key] = node;
      addToFreqList(node);
      addFreqNum();
      minFreq_ = std::min(minFreq_, 1);
    }

    // 获取缓存
    void getInternal(NodePtr node, Value &value)
    {
      // 找到之后需要将其从低访问频次的链表中删除，并且添加到+1的访问频次链表中，
      // 访问频次+1, 然后把value值返回
      value = node->value;
      // 从原有访问频次的链表中删除节点
      removeFromFreqList(node);
      node->freq++;
      addToFreqList(node);
      // 如果当前node的访问频次如果等于minFreq+1，并且其前驱链表为空，则说明
      // freqToFreqList_[node->freq - 1]链表因node的迁移已经空了，需要更新最小访问频次
      if (node->freq - 1 == minFreq_ && freqToFreqList_[node->freq - 1]->isEmpty())
        minFreq_++;

      // 总访问频次和当前平均访问频次都随之增加
      addFreqNum();
    }

    // 移除缓存中的过期数据
    void kickOut()
    {
      NodePtr node = freqToFreqList_[minFreq_]->getFirstNode();
      removeFromFreqList(node);
      nodeMap_.erase(node->key);
      decreaseFreqNum(node->freq);
    }

    // 从频率列表中移除节点
    void removeFromFreqList(NodePtr node)
    {
      // 检查结点是否为空
      if (!node)
        return;

      auto freq = node->freq;
      freqToFreqList_[freq]->removeNode(node);
    }

    // 添加到频率列表
    void addToFreqList(NodePtr node)
    {
      // 检查结点是否为空
      if (!node)
        return;

      // 添加进入相应的频次链表前需要判断该频次链表是否存在
      auto freq = node->freq;
      if (freqToFreqList_.find(node->freq) == freqToFreqList_.end())
      {
        // 不存在则创建
        freqToFreqList_[node->freq] = new FreqList<Key, Value>(node->freq);
      }

      freqToFreqList_[freq]->addNode(node);
    }

    // 增加平均访问等频率
    void addFreqNum()
    {
      curTotalNum_++;
      if (nodeMap_.empty())
        curAverageNum_ = 0;
      else
        curAverageNum_ = curTotalNum_ / nodeMap_.size();

      if (curAverageNum_ > maxAverageNum_)
      {
        handleOverMaxAverageNum();
      }
    }

    // 减少平均访问等频率
    void decreaseFreqNum(int num)
    {
      curTotalNum_ -= num;
      if (nodeMap_.empty())
        curAverageNum_ = 0;
      else
        curAverageNum_ = curTotalNum_ / nodeMap_.size();
    }

    // 处理当前平均访问频率超过上限的情况
    void handleOverMaxAverageNum()
    {
      if (nodeMap_.empty())
        return;

      // 当前平均访问频次已经超过了最大平均访问频次，所有结点的访问频次- (maxAverageNum_ / 2)
      for (auto it = nodeMap_.begin(); it != nodeMap_.end(); ++it)
      {
        // 检查结点是否为空
        if (!it->second)
          continue;

        NodePtr node = it->second;

        // 先从当前频率列表中移除
        removeFromFreqList(node);

        // 减少频率
        node->freq -= maxAverageNum_ / 2;
        if (node->freq < 1)
          node->freq = 1;

        // 添加到新的频率列表
        addToFreqList(node);
      }

      // 更新最小频率
      updateMinFreq();
    }

    // 更新最小频率
    void updateMinFreq()
    {
      minFreq_ = INT8_MAX;
      for (const auto &pair : freqToFreqList_)
      {
        if (pair.second && !pair.second->isEmpty())
          minFreq_ = std::min(minFreq_, pair.first);
      }
      if (minFreq_ == INT8_MAX)
        minFreq_ = 1;
    }

  private:
    int capacity_;                                                   // 缓存容量
    int minFreq_;                                                    // 最小访问频次(用于找到最小访问频次结点)
    int maxAverageNum_;                                              // 最大平均访问频次
    int curAverageNum_;                                              // 当前平均访问频次
    int curTotalNum_;                                                // 当前访问所有缓存次数总数
    std::mutex mutex_;                                               // 互斥锁
    NodeMap nodeMap_;                                                // key 到 缓存节点的映射
    std::unordered_map<int, FreqList<Key, Value> *> freqToFreqList_; // 访问频次到该频次链表的映射
  };

  // Lfu 哈希分片
  template <typename Key, typename Value>
  class RainLfuHash
  {

  public:
    // 构造函数
    explicit RainLfuHash(size_t capacity, int sliceNum, int maxAverageNum = 10)
        : sliceNum_(sliceNum > 0 ? sliceNum : std::thread::hardware_concurrency()),
          capacity_(capacity)
    {
      size_t sliceSize = std::ceil(capacity_ / static_cast<double>(sliceNum_)); // 每个lfu分片的容量
      for (int i = 0; i < sliceNum_; ++i)
      {
        lfuSliceCaches_.emplace_back(new RainLfu<Key, Value>(sliceSize, maxAverageNum));
      }
    }

    // 存入缓存
    void put(Key key, Value value)
    {
      // 根据key找出对应的lfu分片
      size_t sliceIndex = Hash(key) % sliceNum_;
      lfuSliceCaches_[sliceIndex]->put(key, value);
    }

    // 取出接口，传出参数
    bool get(Key key, Value &value)
    {
      // 根据key找出对应的lfu分片
      size_t sliceIndex = Hash(key) % sliceNum_;
      return lfuSliceCaches_[sliceIndex]->get(key, value);
    }

    // 取出接口 返回值
    Value get(Key key)
    {
      Value value;
      get(key, value);
      return value;
    }

    // 清除缓存
    void purge()
    {
      for (auto &lfuSliceCache : lfuSliceCaches_)
      {
        lfuSliceCache->purge();
      }
    }

  private:
    // 将 key 计算成对应哈希值
    size_t Hash(Key key)
    {
      std::hash<Key> hashFunc;
      return hashFunc(key);
    }

  private:
    size_t capacity_;                                                  // 缓存总容量
    int sliceNum_;                                                     // 缓存分片数量
    std::vector<std::unique_ptr<RainLfu<Key, Value>>> lfuSliceCaches_; // 缓存lfu分片容器
  };
}