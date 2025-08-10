#pragma once

#include <algorithm>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @class ConsistentHash
 * @brief Implements the consistent hash algorithm class
 *
 * ConsistentHash is a distributed hash algorithm that
 * aims to minimize the redistribution of keys when nodes are added or removed.
 * Usually used in distributed caching systems and sharding of distributed databases.
 */
class ConsistentHash
{
public:
    /**
     * @brief Constructor.
     * @param numReplicas Virtual nodes number per physical node, add virtual nodes can improve load balancing effect
     * @param hashFunc Optional custom hash function, default is std::hash
     */
    ConsistentHash(size_t numReplicas, std::function<size_t(const std::string &)> hashFunc = std::hash<std::string>())
        : numReplicas_(numReplicas), hashFunction_(hashFunc) {}

    /**
     * @brief Add a node to the hash ring
     * Every node is replicated as several virtual nodes, every virtual node is calculated by `node + index`(unique hash value)
     * Theses hash values are stored in the hash ring and sorted for efficient lookup.
     *
     * @param node To add node name(like server address)
     */
    void addNode(const std::string &node)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        for (size_t i = 0; i < numReplicas_; ++i)
        {
            // Caculate unique hash value for each virtual node
            size_t hash = hashFunction_(node + "_0" + std::to_string(i));
            circle_[hash] = node;          // Hash value map to node
            sortedHashes_.push_back(hash); // Add to sorted hash list
        }
        // Sort hash values
        std::sort(sortedHashes_.begin(), sortedHashes_.end());
    }

    /**
     * @brief Remove a node from the hash ring
     * Remove all virtual nodes and their corresponding hash values of the node.
     *
     * @param node To remove node name
     */
    void removeNode(const std::string &node)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        for (size_t i = 0; i < numReplicas_; ++i)
        {
            // Calculate hash value for virtual node
            size_t hash = hashFunction_(node + std::to_string(i));
            circle_.erase(hash); // Remove from hash ring
            auto it = std::find(sortedHashes_.begin(), sortedHashes_.end(), hash);
            if (it != sortedHashes_.end())
            {
                sortedHashes_.erase(it); // Remove from sorted hash list
            }
        }
    }

    /**
     * @brief Find the node responsible for handling the given key
     * Find the first node in the hash ring with a hash value greater than or equal to the key hash value.
     * If not found (exceeds the maximum value of the hash ring), wrap around to the first node.
     *
     * @param key To find key(like data identifier)
     * @return Node name responsible for handling the key
     * @throws std::runtime_error If the hash ring is empty(no nodes)
     */
    size_t getNode(const std::string &key)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (circle_.empty())
        {
            throw std::runtime_error("No nodes in consistent hash");
        }
        size_t hash = hashFunction_(key); // Calculate key hash value
        // Find the first position in the sorted hash list with a hash value greater than or equal to the key hash value
        auto it = std::upper_bound(sortedHashes_.begin(), sortedHashes_.end(), hash);
        if (it == sortedHashes_.end())
        {
            // If exceeds the maximum value of the hash ring, wrap around to the first node
            it = sortedHashes_.begin();
        }
        return *it; // Return corresponding hash value
    }

private:
    size_t numReplicas_;                                      // Number of virtual nodes per physical node
    std::function<size_t(const std::string &)> hashFunction_; // Custom or default hash function
    std::unordered_map<size_t, std::string> circle_;          // Hash value to node name mapping
    std::vector<size_t> sortedHashes_;                        // Sorted hash value list for efficient lookup
    std::mutex mtx_;                                          // Mutex to protect hash ring, ensuring thread safety
};
