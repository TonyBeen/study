/*************************************************************************
    > File Name: coroutine.h
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月27日 星期五 11时13分45秒
 ************************************************************************/

#ifndef __COROURINE_H__
#define __COROURINE_H__

#include <memory>
#include <functional>

#include <utils/utils.h>

namespace eular {
struct CoSharedStackPrivate;
class CoSharedStack
{
public:
    CoSharedStack() = default;
    CoSharedStack(uint32_t size);
    ~CoSharedStack() = default;

    CoSharedStack(const CoSharedStack &other) : m_p(other.m_p) { }
    CoSharedStack &operator=(const CoSharedStack &other)
    {
        if (this != std::addressof(other)) {
            m_p = other.m_p;
        }

        return *this;
    }
    CoSharedStack(CoSharedStack &&other)
    {
        std::swap(m_p, other.m_p);
    }
    CoSharedStack &operator=(CoSharedStack &&other)
    {
        std::swap(m_p, other.m_p);
    }

private:
    std::shared_ptr<CoSharedStackPrivate> m_p;
};

struct CoroutinePrivate;
class Coroutine : public std::enable_shared_from_this<Coroutine>
{
    DISALLOW_COPY_AND_ASSIGN(Coroutine);
public:
    using SP = std::shared_ptr<Coroutine>;
    using WP = std::weak_ptr<Coroutine>;

    enum FiberState {
        HOLD,       // 暂停状态
        EXEC,       // 执行状态
        TERM,       // 结束状态
        READY,      // 可执行态
        EXCEPT      // 异常状态
    };

    Coroutine();
    ~Coroutine();

private:
    std::shared_ptr<CoroutinePrivate> m_p;
};
} // namespace eular

#endif // __COROURINE_H__
