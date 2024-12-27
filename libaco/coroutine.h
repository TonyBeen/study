/*************************************************************************
    > File Name: coroutine.h
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月27日 星期五 11时13分45秒
 ************************************************************************/

#ifndef __COROURINE_H__
#define __COROURINE_H__

#include <memory>

#include <utils/utils.h>

namespace eular {
struct SharedStackPrivate;
class SharedStack
{
public:
    SharedStack() = default;
    SharedStack(uint32_t size);
    ~SharedStack() = default;

    SharedStack(const SharedStack &other) : m_p(other.m_p) { }
    SharedStack &operator=(const SharedStack &other)
    {
        if (this != std::addressof(other)) {
            m_p = other.m_p;
        }

        return *this;
    }
    SharedStack(SharedStack &&other)
    {
        std::swap(m_p, other.m_p);
    }
    SharedStack &operator=(SharedStack &&other)
    {
        std::swap(m_p, other.m_p);
    }

    void *pointer() const;
    uint32_t stackSize() const;

private:
    std::shared_ptr<SharedStackPrivate> m_p;
};

struct CoroutinePrivate;
class Coroutine
{
    DISALLOW_COPY_AND_ASSIGN(Coroutine);
public:
    Coroutine();
    ~Coroutine();

private:
    std::shared_ptr<CoroutinePrivate> m_p;
};
} // namespace eular

#endif // __COROURINE_H__
