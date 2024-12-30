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
    friend class Coroutine;
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
        if (this != std::addressof(other)) {
            std::swap(m_p, other.m_p);
        }

        return *this;
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
    using TaskCB = std::function<void()>;

    enum CoState {
        HOLD,       // 暂停状态
        EXEC,       // 执行状态
        TERM,       // 结束状态
        READY,      // 可执行态
        EXCEPT      // 异常状态
    };

    Coroutine(TaskCB cb, const CoSharedStack &stack);
    ~Coroutine();

    static void             SetThis(Coroutine *co); // 设置当前正在执行的协程
    static Coroutine::SP    GetThis();              // 获取当前正在执行的协程
           void             Resume();               // 唤醒协程
    static void             Yeild2Hold();           // 将当前正在执行的协程让出执行权给主协程，并设置状态为HOLD

private:
    Coroutine();
    static void CoroutineEntry();
    void swapIn();              // 切换到前台, 获取执行权限
    void swapOut();             // 切换到后台, 让出执行权限

private:
    std::shared_ptr<CoroutinePrivate> m_p;
};
} // namespace eular

#endif // __COROURINE_H__
