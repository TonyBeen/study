/*************************************************************************
    > File Name: test_std_call_once.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年08月13日 星期二 10时46分28秒
 ************************************************************************/

#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

void stressCPU()
{
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(100);

    double result = 0.0;
    while (std::chrono::high_resolution_clock::now() < end) {
        for (int i = 0; i < 10000000; ++i) {
            result += std::sin(static_cast<double>(i * M_PI / 180.0));
        }
        for (int i = 0; i < 10000000; ++i) {
            result += std::cos(static_cast<double>(i * M_PI / 180.0));
        }
    }

    std::cout << "Stress test finished. Result: " << result << std::endl;
}

int main()
{
    std::cout << "Starting CPU stress test..." << std::endl;
    
    std::thread th(stressCPU);
    stressCPU();

    th.join();
    std::cout << "CPU stress test completed." << std::endl;
    return 0;
}
