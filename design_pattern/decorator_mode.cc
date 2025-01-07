/*************************************************************************
    > File Name: decorator_mode.cc
    > Author: hsz
    > Brief: 装饰器模式
    > Created Time: 2024年03月24日 星期日 14时45分30秒
 ************************************************************************/

/**
 * 装饰器模式（Decorator Pattern）
 * 允许向一个现有对象添加新的功能，同时又不改变其结构。这种类型的设计模式属于结构型模式，它是作为现有的类的一个包装。
 * 意图
 *      动态的给一个对象添加额外的职责
 * 优点
 *      装饰类和被装饰类可以独立发展，不会互相耦合
 * 缺点
 *      多层装饰比较复杂
 */

#include <iostream>

// 基础接口类
class Component
{
public:
    virtual void operation() = 0;
};

// 具体组件类
class ConcreteComponent : public Component
{
public:
    void operation() override {
        std::cout << "ConcreteComponent operation" << std::endl;
    }
};

// 装饰器基类
class Decorator : public Component
{
protected:
    Component* component;

public:
    Decorator(Component* comp) : component(comp) {}

    void operation() override {
        if (component != nullptr) {
            component->operation();
        }
    }
};

// 具体装饰器类
class ConcreteDecorator : public Decorator {
public:
    ConcreteDecorator(Component* comp) : Decorator(comp) {}

    void addBehavior() {
        std::cout << "Added Behavior" << std::endl;
    }

    void operation() override {
        Decorator::operation();
        addBehavior();
    }
};

int main() {
    Component* component = new ConcreteComponent();
    component->operation();

    std::cout << "=====================add behavior" << std::endl;

    Component* decoratedComponent = new ConcreteDecorator(component);
    decoratedComponent->operation();

    delete component;
    delete decoratedComponent;

    return 0;
}

