#include "memoryPool.h"

namespace memoryPool
{
    MemoryPool::MemoryPool(size_t BlockSize)
        : BlockSize_(BlockSize)
    {
    }

    MemoryPool::~MemoryPool()
    {
        // Delete all the blocks
        Slot *cur = firstBlock_;
        while (cur)
        {
            Slot *next = cur->next;
            // Equal with: free(reinterpret_cast<void*>(firstBlock_));
            // Cast it to void*, because void type doesn't need destructor, just release memory
            operator delete(reinterpret_cast<void *>(cur));
            cur = next;
        }
    }

    void MemoryPool::init(size_t size)
    {
        assert(size > 0);
        SlotSize_ = size;
        firstBlock_ = nullptr;
        curSlot_ = nullptr;
        freeList_ = nullptr;
        lastSlot_ = nullptr;
    }

    void *MemoryPool::allocate()
    {
        // Use free list first
        if (freeList_ != nullptr)
        {
            {
                std::lock_guard<std::mutex> lock(mutexForFreeList_);
                if (freeList_ != nullptr)
                {
                    Slot *temp = freeList_;
                    freeList_ = freeList_->next;
                    return temp;
                }
            }
        }

        Slot *temp;
        {
            std::lock_guard<std::mutex> lock(mutexForBlock_);

            // Current memory block has no available slots, allocate a new block
            if (curSlot_ >= lastSlot_)
            {
                allocateNewBlock();
            }

            temp = curSlot_;

            // Can not use curSlot_ += SlotSize_ because curSlot_ is a Slot* type, so we need to divide it by SlotSize_
            curSlot_ += SlotSize_ / sizeof(Slot);
        }

        return temp;
    }

    void MemoryPool::deallocate(void *ptr)
    {
        if (ptr)
        {
            // Collect memory, insert it into the free list by head insertion
            std::lock_guard<std::mutex> lock(mutexForFreeList_);
            reinterpret_cast<Slot *>(ptr)->next = freeList_;
            freeList_ = reinterpret_cast<Slot *>(ptr);
        }
    }

    void MemoryPool::allocateNewBlock()
    {
        // std::cout << "Apply for new memory block, BlockSize: " << BlockSize_ << "，SlotSize: " << SlotSize_ << std::endl;
        //  Insert a new memory block by head insertion
        void *newBlock = operator new(BlockSize_);
        reinterpret_cast<Slot *>(newBlock)->next = firstBlock_;
        firstBlock_ = reinterpret_cast<Slot *>(newBlock);

        char *body = reinterpret_cast<char *>(newBlock) + sizeof(Slot *);
        size_t paddingSize = padPointer(body, SlotSize_); // Compute padding size
        curSlot_ = reinterpret_cast<Slot *>(body + paddingSize);

        // If beyound the last slot, has no available slots, need to allocate a new block from system
        lastSlot_ = reinterpret_cast<Slot *>(reinterpret_cast<size_t>(newBlock) + BlockSize_ - SlotSize_ + 1);

        freeList_ = nullptr;
    }

    // Let pointer pad to the next multiple of slot size
    size_t MemoryPool::padPointer(char *p, size_t align)
    {
        // align is slot size
        return (align - reinterpret_cast<size_t>(p)) % align;
    }

    void HashBucket::initMemoryPool()
    {
        for (int i = 0; i < MEMORY_POOL_NUM; i++)
        {
            getMemoryPool(i).init((i + 1) * SLOT_BASE_SIZE);
        }
    }

    // Design Pattern: Singleton mode
    MemoryPool &HashBucket::getMemoryPool(int index)
    {
        static MemoryPool memoryPool[MEMORY_POOL_NUM];
        return memoryPool[index];
    }

} // namespace memoryPool
