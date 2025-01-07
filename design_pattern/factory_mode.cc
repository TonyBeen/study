/*************************************************************************
    > File Name: factory_mode.cc
    > Author: hsz
    > Brief: 创建型模式 - 工厂方法模式
    > Created Time: 2024年03月24日 星期日 15时09分34秒
 ************************************************************************/

#include <iostream>
#include <memory>

// 产品接口类
class Product {
public:
    virtual void use() = 0;
};

// 具体产品类 A
class ConcreteProductA : public Product {
public:
    void use() override {
        std::cout << "Using ConcreteProductA" << std::endl;
    }
};

// 具体产品类 B
class ConcreteProductB : public Product {
public:
    void use() override {
        std::cout << "Using ConcreteProductB" << std::endl;
    }
};

// 工厂接口类
class Factory {
public:
    virtual std::shared_ptr<Product> createProduct() = 0;
};

// 具体工厂类 A
class ConcreteFactoryA : public Factory {
public:
    std::shared_ptr<Product> createProduct() override {
        return std::make_shared<ConcreteProductA>();
    }
};

// 具体工厂类 B
class ConcreteFactoryB : public Factory {
public:
    std::shared_ptr<Product> createProduct() override {
        return std::make_shared<ConcreteProductB>();
    }
};

int main() {
    // 使用工厂 A 创建产品 A
    std::shared_ptr<Factory> factoryA = std::make_shared<ConcreteFactoryA>();
    std::shared_ptr<Product> productA = factoryA->createProduct();
    productA->use();

    // 使用工厂 B 创建产品 B
    std::shared_ptr<Factory> factoryB = std::make_shared<ConcreteFactoryB>();
    std::shared_ptr<Product> productB = factoryB->createProduct();
    productB->use();

    return 0;
}
