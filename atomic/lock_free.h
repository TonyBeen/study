/*************************************************************************
    > File Name: lock_free.h
    > Author: hsz
    > Brief:
    > Created Time: Mon 11 Mar 2024 05:24:45 PM CST
 ************************************************************************/

#ifndef __LOCK_FREE_H__
#define __LOCK_FREE_H__

#include <stdint.h>
#include <assert.h>
#include <atomic>
#include <memory>

// 实现一个无所队列

template<typename T>
struct lock_free_item
{
    std::atomic<lock_free_item<T> *> next;

    lock_free_item() :
        next(nullptr)
    {
    }

    void connect(lock_free_queue *prev, lock_free_queue *next)
    {
        assert(prev != nullptr);

    }

    T item;
};

template<typename T, uint32_t cap = 128>
class lock_free_queue
{
public:
    typedef std::unique_ptr<lock_free_item<T>> Ptr;

    lock_free_queue() :
        m_header(nullptr),
        m_tail(nullptr)
    {
        m_header.store(new lock_free_item<T>(), std::memory_order_release);
        m_tail.store(new lock_free_item<T>(), std::memory_order_release);

        auto ptr = m_header.load(std::memory_order_acquire);
        
    }

    ~lock_free_queue()
    {

    }

    bool try_push(const T &object)
    {
        lock_free_item<T> *pItem = new lock_free_item<T>();
        pItem->item = object;

        do
        {
            auto pTail = m_tail.load(std::memory_order_acquire);
            pTail->next.store(pItem, std::memory_order_release);
        } while (m_tail.compare_exchange_strong(pTail, pItem, std::memory_order_release, std::memory_order_relaxed));
    }

private:
    std::atomic<lock_free_item<T>*>    m_header;
    std::atomic<lock_free_item<T>*>    m_tail;
};

#endif // __LOCK_FREE_H__
