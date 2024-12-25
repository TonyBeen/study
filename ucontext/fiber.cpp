/*************************************************************************
    > File Name: fiber.cpp
    > Author: hsz
    > Mail:
    > Created Time: Wed 22 Sep 2021 04:17:18 PM CST
 ************************************************************************/

#include "fiber.h"
#include <log/log.h>
#include <atomic>
#include <exception>

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "fiber"
#else
#define LOG_TAG "fiber"
#endif

namespace eular{

static std::atomic<uint64_t> gFiberId(0);       // 协程ID
static std::atomic<uint64_t> gFiberCount(0);    // 当前协程总数

static thread_local Fiber *gCurrFiber = nullptr;            // 当前正在执行的协程
static thread_local Fiber::sp gThreadMainFiber = nullptr;   // 一个线程的主协程

// TODO: 配置yaml文件，获取栈大小
uint64_t getStackSize()
{
    static uint64_t size = 1024 * 1024;
    // size = Config::Lookup<uint64_t>("fiber.stack_size", 1024 * 1024);
    return size;
}

class MallocAllocator
{
public:
    static void *alloc(uint64_t size)
    {
        return malloc(size);
    }
    static void dealloc(void *ptr, uint64_t size)
    {
        LOG_ASSERT(ptr, "dealloc a null pointer");
        free(ptr);
    }
};

using Allocator = MallocAllocator;

Fiber::Fiber() :
    mFiberId(++gFiberId)
{
    ++gFiberCount;
    mState = EXEC;
    if (getcontext(&mCtx)) {
        LOG_ASSERT(false, "getcontext error, %d %s", errno, strerror(errno));
    }
    SetThis(this);
    LOGD("Fiber::Fiber() main fiber started. id = %d, total = %d", mFiberId, gFiberCount.load());
}

Fiber::Fiber(std::function<void()> cb, uint64_t stackSize) :
    mFiberId(++gFiberId),
    mCb(cb),
    mState(READY)
{
    ++gFiberCount;

    mStackSize = stackSize ? stackSize : getStackSize();
    mStack = Allocator::alloc(mStackSize);
    if (!mStack) {
        LOGD("Fiber id = %lu, stack pointer is null", mFiberId);
    }
    if (getcontext(&mCtx)) {
        LOG_ASSERT(false, "Fiber::Fiber(std::function<void()>, uint64_t) getcontext error, %d %s", errno, strerror(errno));
    }
    mCtx.uc_stack.ss_sp = mStack;
    mCtx.uc_stack.ss_size = mStackSize;
    mCtx.uc_link = nullptr;
    makecontext(&mCtx, &FiberEntry, 0);

    LOGD("Fiber::Fiber(std::function<void()>, uint64_t) id = %lu, total = %d start",
        mFiberId, gFiberCount.load());
}

Fiber::~Fiber()
{
    --gFiberCount;
    LOGD("Fiber::~Fiber() id = %lu, total = %d", mFiberId, gFiberCount.load());
    if (mStack) {
        LOG_ASSERT(mState == TERM || mState == EXCEPT,
            "file %s, line %d", __FILE__, __LINE__);
        Allocator::dealloc(mStack, mStackSize);
    } else {    // main fiber
        LOG_ASSERT(!mCb, "");
        if (gCurrFiber == this) {
            SetThis(nullptr);
        }
    }
}

// 调用位置在主协程中。
void Fiber::Reset(std::function<void()> cb)
{
    LOG_ASSERT(mStack, "main fiber can't reset"); // 排除main fiber
    // 暂停态，执行态，ready态无法reset
    LOG_ASSERT(mState == TERM || mState == EXCEPT, "Reset unauthorized operation");
    mCb = cb;
    if (getcontext(&mCtx)) {
        LOG_ASSERT(false, "File %s, Line %s. getcontex error.", __FILE__, __LINE__);
    }
    mCtx.uc_stack.ss_sp = mStack;
    mCtx.uc_stack.ss_size = mStackSize;
    mCtx.uc_link = nullptr;
    makecontext(&mCtx, &FiberEntry, 0);
    mState = READY;
}

/**
 * 同一个线程应使SwapIn的调用次数比Yeild调用次数多一，如果多二则会在FiberEntry结尾处退出线程
 * 如果不多一则会使智能指针的引用计数大于1，导致释放不干净，原因是FiberEntry的回调未执行完毕，
 * 即最后一次Yeild操作保存了堆栈，使得在栈上的智能指针无法释放
 */
void Fiber::SwapIn()
{
    SetThis(this);
    LOG_ASSERT(mState != EXEC, "");
    mState = EXEC;
    if (swapcontext(&gThreadMainFiber->mCtx, &mCtx)) {
        LOG_ASSERT(false, "SwapIn() id = %d, errno = %d, %s", mFiberId, errno, strerror(errno));
    }
}

void Fiber::SwapOut()
{
    SetThis(gThreadMainFiber.get());
    if (swapcontext(&mCtx, &gThreadMainFiber->mCtx)) {
        LOG_ASSERT(false, "SwapIn() id = %d, errno = %d, %s", mFiberId, errno, strerror(errno));
    }
}

void Fiber::SetThis(Fiber *f)
{
    gCurrFiber = f;
}

Fiber::sp Fiber::GetThis()
{
    if (gCurrFiber) {
        return gCurrFiber->shared_from_this();
    }
    Fiber::sp fiber(new Fiber());
    LOG_ASSERT(fiber.get() == gCurrFiber, "");
    gThreadMainFiber = fiber;
    LOG_ASSERT(gThreadMainFiber, "");
    return gCurrFiber->shared_from_this();
}

void Fiber::Resume()
{
    SwapIn();
}

void Fiber::Yeild2Hold()
{
    Fiber::sp ptr = GetThis();
    LOG_ASSERT(ptr != nullptr, "");
    LOG_ASSERT(ptr->mState == EXEC, "current state = %d", ptr->mState);
    ptr->mState = HOLD;
    ptr->SwapOut();
}

void Fiber::Yeild2Ready()
{
    Fiber::sp ptr = GetThis();
    LOG_ASSERT(ptr != nullptr, "");
    LOG_ASSERT(ptr->mState == EXEC, "");
    ptr->mState = READY;
    ptr->SwapOut();
}

void Fiber::FiberEntry()
{
    Fiber::sp curr = GetThis();
    LOG_ASSERT(curr != nullptr, "");
    try {
        curr->mCb();
        curr->mCb = nullptr;
        curr->mState = TERM;
    } catch (const std::exception& e) {
        curr->mState = EXCEPT;
        LOGE("Fiber except: %s; id = %lu", e.what(), curr->mFiberId);
    } catch (...) {
        curr->mState = EXCEPT;
        LOGE("Fiber except. id = %lu", curr->mFiberId);
    }

    Fiber *ptr = curr.get();
    curr.reset();
    ptr->SwapOut();

    LOG_ASSERT(false, "never reach here");
}

}