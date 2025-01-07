/*************************************************************************
    > File Name: strategy_mode.cc
    > Author: hsz
    > Brief: 行为型模式 - 策略模式
    > Created Time: 2024年03月24日 星期日 15时21分59秒
 ************************************************************************/

#include <iostream>

// 策略接口类
class Strategy {
public:
    virtual void doOperation() = 0;
};

// 具体策略类 A
class ConcreteStrategyA : public Strategy {
public:
    void doOperation() override {
        std::cout << "Using ConcreteStrategyA" << std::endl;
    }
};

// 具体策略类 B
class ConcreteStrategyB : public Strategy {
public:
    void doOperation() override {
        std::cout << "Using ConcreteStrategyB" << std::endl;
    }
};

// 上下文类
class Context {
private:
    Strategy* strategy;

public:
    Context(Strategy* strategy) : strategy(strategy) {}

    void setStrategy(Strategy* newStrategy) {
        strategy = newStrategy;
    }

    void executeStrategy() {
        strategy->doOperation();
    }
};

int main() {
    ConcreteStrategyA strategyA;
    ConcreteStrategyB strategyB;

    // 使用具体策略 A
    Context context(&strategyA);
    context.executeStrategy();

    // 切换到具体策略 B
    context.setStrategy(&strategyB);
    context.executeStrategy();

    return 0;
}

