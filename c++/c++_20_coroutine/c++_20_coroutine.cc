/*************************************************************************
    > File Name: c++_20_coroutine.cc
    > Author: hsz
    > Brief: g++ c++_20_coroutine.cc -std=c++20 -fcoroutines -g
    > Created Time: 2024年06月22日 星期六 08时11分24秒
 ************************************************************************/

#include <iostream>
#include <coroutine>

using namespace std;

struct resumable_thing
{
    struct promise_type
    {
        int _value = 0;

        resumable_thing get_return_object()
        {
            return resumable_thing(coroutine_handle<promise_type>::from_promise(*this));
        }

        auto initial_suspend()
        {
            return suspend_never{};
        }

        auto final_suspend() noexcept
        {
            return suspend_never{};
        }

        // 声明此函数无法调用co_return
        // void return_void() {}

        void unhandled_exception()
        {
        }

        // 声明此函数可调用co_return
        auto return_value(int v) { _value = v; }
    };

    coroutine_handle<promise_type> _coroutine = nullptr;
    resumable_thing() = default;
    resumable_thing(resumable_thing const &) = delete;
    resumable_thing &operator=(resumable_thing const &) = delete;

    // resumable_thing(resumable_thing &&other)
    //     : _coroutine(other._coroutine)
    // {
    //     other._coroutine = nullptr;
    // }
    // resumable_thing &operator=(resumable_thing &&other)
    // {
    //     if (&other != this)
    //     {
    //         _coroutine = other._coroutine;
    //         other._coroutine = nullptr;
    //     }
    //     return *this;
    // }

    explicit resumable_thing(coroutine_handle<promise_type> coroutine) : _coroutine(coroutine)
    {
    }

    ~resumable_thing()
    {
        // 加上这段代码会崩溃, 但通过valgrind测试, 未发现内存泄露, 说明已经释放了
        // 通过https://cppinsights.io/ 可以看到C++20协程实现手段, 现已将其拷贝到c++_20_coroutine_unwind.cc
        // 在182行增加打印发现其被调用了, 但是未产生崩溃, 暂时不太理解为何
        // if (_coroutine)
        // {
        //     _coroutine.destroy();
        // }
    }

    void resume() { _coroutine.resume(); }
};

resumable_thing counter()
{
    cout << "counter: called\n";
    for (int i = 0; i < 5 ; ++i)
    {
        co_await std::suspend_always{};
        cout << "counter:: resumed (#" << i << ")\n";
    }

    cout << "counter: end\n";
    co_return 11;
}

int main()
{
    cout << "main:    calling counter\n";
    resumable_thing the_counter = counter();

    cout << "main:    resuming counter\n";

    the_counter.resume();
    the_counter.resume();
    the_counter.resume();
    the_counter.resume();
    the_counter.resume();

    cout << "main:    done\n";

    return 0;
}
