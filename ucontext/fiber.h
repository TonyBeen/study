/*************************************************************************
    > File Name: fiber.h
    > Author: hsz
    > Mail:
    > Created Time: Wed 22 Sep 2021 04:17:18 PM CST
 ************************************************************************/

#ifndef __FIBER_H__
#define __FIBER_H__

#include <stdio.h>
#include <ucontext.h>
#include <functional>
#include <memory>

namespace eular {
/**
 * @brief 非对称协程, 无法在协程回调创建协程, 需要将运行权限交回主协程才能执行其他协程
 */
class Fiber : public std::enable_shared_from_this<Fiber>
{
public:
    typedef std::shared_ptr<Fiber> sp;
    enum FiberState {
        HOLD,       // 暂停状态
        EXEC,       // 执行状态
        TERM,       // 结束状态
        READY,      // 可执行态
        EXCEPT      // 异常状态
    };
    Fiber(std::function<void()> cb, uint64_t stackSize = 0);
    ~Fiber();

    uint64_t            FiberId() const { return mFiberId; }
    FiberState          State() const { return mState; }

           void         Reset(std::function<void()> cb);
    static void         SetThis(Fiber *f);  // 设置当前正在执行的协程
    static Fiber::sp    GetThis();          // 获取当前正在执行的协程
           void         Resume();           // 唤醒协程
    static void         Yeild2Hold();       // 将当前正在执行的协程让出执行权给主协程，并设置状态为HOLD

private:
    static void         Yeild2Ready();      // 将当前正在执行的协程让出执行权给主协程，并设置状态为READY
    Fiber();                    // 线程的第一个协程调用
    static void FiberEntry();   // 协程入口函数
    void SwapIn();              // 切换到前台, 获取执行权限
    void SwapOut();             // 切换到后台, 让出执行权限

private:
    ucontext_t      mCtx;
    FiberState      mState;
    uint64_t        mFiberId;
    uint64_t        mStackSize;
    void *          mStack;
    std::function<void()> mCb;
};

} // namespace eular

#endif // __FIBER_H__