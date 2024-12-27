/*************************************************************************
    > File Name: coroutine.cpp
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月27日 星期五 11时13分49秒
 ************************************************************************/

#include "coroutine.h"

#include <atomic>
#include <inttypes.h>

#include <utils/exception.h>
#include <log/log.h>

#include "aco.h"

#define LOG_TAG "coroutine"

namespace eular {

static std::atomic<uint64_t> g_coroutineId(0);      // 协程ID
static std::atomic<uint64_t> g_coroutineCount(0);   // 当前协程总数

static thread_local Coroutine*      g_currentCo = nullptr;  // 当前正在执行的协程
static thread_local Coroutine::SP   g_mainCo = nullptr;     // 一个线程的主协程

struct CoSharedStackPrivate
{
    aco_share_stack_t *shared_stack = nullptr;
};

struct CoroutinePrivate
{
    aco_t*              co_ctx;
    uint64_t            co_id;
    Coroutine::TaskCB   co_task;
    Coroutine::CoState  co_state;

    CoroutinePrivate() :
        co_ctx(nullptr),
        co_id(0),
        co_state(Coroutine::CoState::READY)
    {
        ++g_coroutineCount;
        co_id = ++g_coroutineId;
    }

    ~CoroutinePrivate()
    {
        --g_coroutineCount;
    }
};

CoSharedStack::CoSharedStack(uint32_t size)
{
    m_p = std::make_shared<CoSharedStackPrivate>();
    m_p->shared_stack = aco_share_stack_new(size);
}

Coroutine::Coroutine()
{
    m_p = std::make_shared<CoroutinePrivate>();
    m_p->co_ctx = aco_create(nullptr, nullptr, 0, nullptr, nullptr);
    m_p->co_state = CoState::READY;
    SetThis(this);
}

Coroutine::Coroutine(TaskCB cb, const CoSharedStack &stack)
{
    if (!stack.m_p->shared_stack) {
        throw Exception("invalid shared stack");
    }

    m_p = std::make_shared<CoroutinePrivate>();
    m_p->co_ctx = aco_create(g_mainCo->m_p->co_ctx, stack.m_p->shared_stack, 0, &Coroutine::CoroutineEntry, nullptr);
    m_p->co_task = cb;
}

Coroutine::~Coroutine()
{
}

void Coroutine::SetThis(Coroutine *co)
{
    g_currentCo = co;
}

Coroutine::SP Coroutine::GetThis()
{
    if (g_currentCo) {
        return g_currentCo->shared_from_this();
    }

    Coroutine::SP co = std::make_shared<Coroutine>();
    LOG_ASSERT(co.get() == g_currentCo, "");
    g_mainCo = co;
    LOG_ASSERT(g_mainCo, "");
    return g_currentCo->shared_from_this();
}

void Coroutine::Resume()
{
    swapIn();
}

void Coroutine::Yeild2Hold()
{
    Coroutine::SP ptr = GetThis();
    LOG_ASSERT(ptr != nullptr, "");
    LOG_ASSERT(ptr->m_p->co_state == CoState::EXEC, "current state = %d", ptr->m_p->co_state);
    ptr->m_p->co_state = CoState::HOLD;
    ptr->swapOut();
}

void Coroutine::CoroutineEntry()
{
    {
        Coroutine::SP current = Coroutine::GetThis();
        LOG_ASSERT(current != nullptr, "");
        try {
            current->m_p->co_task();
            current->m_p->co_task = nullptr;
            current->m_p->co_state = CoState::TERM;
        } catch (const std::exception& e) {
            current->m_p->co_state = CoState::EXCEPT;
            LOGE("Fiber except: %s; id = %" PRIu64, e.what(), current->m_p->co_id);
        } catch (...) {
            current->m_p->co_state = CoState::EXCEPT;
            LOGE("Fiber except. id = %" PRIu64, current->m_p->co_id);
        }

        Coroutine *co = current.get();
        current.reset();
        LOGD("id %" PRIu64 " swap out", co->m_p->co_id);
        co->swapOut();
    }

    LOG_ASSERT(false, "never reach here");
}

void Coroutine::swapIn()
{
    SetThis(this);
    m_p->co_state = CoState::EXEC;
    aco_resume(m_p->co_ctx);
}

void Coroutine::swapOut()
{
    SetThis(g_mainCo.get());
    aco_yield();
}

} // namespace eular
