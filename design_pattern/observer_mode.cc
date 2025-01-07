/*************************************************************************
    > File Name: observer_mode.cc
    > Author: hsz
    > Brief: 行为型模式 - 观察者模式
    > Created Time: 2024年03月24日 星期日 14时05分42秒
 ************************************************************************/

#include <vector>
#include <string>
#include <type_traits>

template<typename Func>
struct FunctionPointer;

// 类的成员函数
template<class Obj, typename Ret, typename... Args>
struct FunctionPointer<Ret (Obj::*) (Args...)>
{
    typedef Obj Object;
    typedef Ret ReturnType;
    typedef Ret (Obj::*Function) (Args...);

    static void call(Function f, Obj *o, Args... args) {
        (o->*f)(std::forward<Args>(args)...);
    }
};

// 类的成员函数 const
template<class Obj, typename Ret, typename... Args>
struct FunctionPointer<Ret (Obj::*) (Args...) const>
{
    typedef Obj Object;
    typedef Ret ReturnType;
    typedef Ret (Obj::*Function) (Args...) const;

    static void call(Function f, Obj *o, Args... args) {
        (o->*f)(std::forward<Args>(args)...);
    }
};

// 普通函数或静态成员函数
template<typename Ret, typename... Args>
struct FunctionPointer<Ret (*) (Args...)>
{
    typedef Ret ReturnType;
    typedef Ret (*Function) (Args...);

    static void call(Function f, Args... args) {
        f(std::forward<Args>(args)...);
    }
};


template<typename... Args>
class SlotBase
{
public:
    virtual ~SlotBase() = default;
    void exec(Args... args)
    {
        slotFunction(std::forward<Args>(args)...);
    }

protected:
    virtual void slotFunction(Args... args) = 0;
};

template<typename TRecver, typename SlotFunc, typename... Args>
class Slot : public SlotBase<Args...>
{
public:
    typedef FunctionPointer<SlotFunc> FuncType;

    Slot(TRecver *recver, SlotFunc func)
    {
        mRecver = recver;
        mFunc = func;
    }
    virtual ~Slot() {}

protected:
    virtual void slotFunction(Args... args) override
    {
        FuncType::call(mFunc, mRecver, std::forward<Args>(args)...);
    }

private:
    TRecver *mRecver;
    SlotFunc mFunc;
};

template<typename... Args>
class Signaler
{
public:
    Signaler() = default;
    ~Signaler() = default;

    template<typename TRecver, typename SlotFunc>
    void addSlot(TRecver *recver, SlotFunc func)
    {
        mSlotVec.push_back(new Slot<TRecver, SlotFunc, Args...>(recver, func));
    }

    void operator()(Args... args)
    {
        for (SlotBase<Args...> *it : mSlotVec)
        {
            it->exec(args...);
        }
    }

private:
    std::vector<SlotBase<Args...> *> mSlotVec;
};

class Recv1
{
public:
    void func(int num, std::string str)
    {
        printf("Recv1::func(%d, %s)\n", num, str.c_str());
    }
};

class Recv2
{
public:
    void func(int num, std::string str)
    {
        printf("Recv2::func(%d, %s)\n", num, str.c_str());
    }
};

class Send
{
public:
    Signaler<int, std::string> valueChanged;

public:
    void triggerSignal(int num, std::string str)
    {
        valueChanged(num, str);
    }
};

#define connect(sender, signal, recver, slot) \
    (sender)->signal.addSlot(recver, slot);


int main(int argc, char **argv)
{
    Send send;
    Recv1 r1;
    Recv2 r2;

    connect(&send, valueChanged, &r1, &Recv1::func);
    connect(&send, valueChanged, &r2, &Recv2::func);

    send.triggerSignal(10, std::string("Hello"));
    return 0;
}
