#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>

namespace memoryPool
{
#define MEMORY_POOL_NUM 64
#define SLOT_BASE_SIZE 8
#define MAX_SLOT_SIZE 512

    // Each memory pool has a different slot size, which is not fixed (multiple of 8)
    struct Slot
    {
        Slot *next;
    };

    class MemoryPool
    {
    public:
        MemoryPool(size_t BlockSize = 4096);
        ~MemoryPool();

        void init(size_t);

        void *allocate();
        void deallocate(void *);

    private:
        void allocateNewBlock();
        size_t padPointer(char *p, size_t align);

    private:
        int BlockSize_;
        int SlotSize_;
        Slot *firstBlock_;            // Point to the first block of memory pool
        Slot *curSlot_;               // Point to the not used slot
        Slot *freeList_;              // Point to free slot
        Slot *lastSlot_;              // The last slot of the memory pool
        std::mutex mutexForFreeList_; // Save the free list to avoid concurrent access
        std::mutex mutexForBlock_;    // Save the block to avoid concurrent access
    };

    class HashBucket
    {
    public:
        static void initMemoryPool();
        static MemoryPool &getMemoryPool(int index);

        static void *useMemory(size_t size)
        {
            if (size <= 0)
                return nullptr;

            // Lager than 512 bytes, use new
            if (size > MAX_SLOT_SIZE)
                return operator new(size);

            // Upper bound of integer division
            return getMemoryPool(((size + 7) / SLOT_BASE_SIZE) - 1).allocate();
        }

        static void freeMemory(void *ptr, size_t size)
        {
            if (!ptr)
                return;
            if (size > MAX_SLOT_SIZE)
            {
                operator delete(ptr);
                return;
            }

            getMemoryPool(((size + 7) / SLOT_BASE_SIZE) - 1).deallocate(ptr);
        }

        template <typename T, typename... Args>
        friend T *newElement(Args &&...args);

        template <typename T>
        friend void deleteElement(T *p);
    };

    template <typename T, typename... Args>
    T *newElement(Args &&...args)
    {
        T *p = nullptr;
        // Select the appropriate memory pool based on the size of the element
        if ((p = reinterpret_cast<T *>(HashBucket::useMemory(sizeof(T)))) != nullptr)
            // Construct the object in the allocated memory
            new (p) T(std::forward<Args>(args)...);

        return p;
    }

    template <typename T>
    void deleteElement(T *p)
    {
        // Destruct the object and free the memory
        if (p)
        {
            p->~T();
            HashBucket::freeMemory(reinterpret_cast<void *>(p), sizeof(T));
        }
    }

} // namespace memoryPool