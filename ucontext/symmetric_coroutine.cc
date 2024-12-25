/*************************************************************************
    > File Name: symmetric_coroutine.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月25日 星期三 19时54分03秒
 ************************************************************************/

#include <ucontext.h>
#include <iostream>
#include <vector>
#include <memory>

constexpr size_t STACK_SIZE = 64 * 1024; // 每个协程的栈大小

struct Coroutine {
    ucontext_t context;               // 协程上下文
    std::unique_ptr<char[]> stack;    // 协程栈
    bool finished = false;            // 标志是否完成
};

std::vector<std::shared_ptr<Coroutine>> coroutines; // 存储所有协程
int current_coroutine = -1;                         // 当前运行的协程索引
ucontext_t main_context;                            // 主上下文

// 协程入口函数
void coroutine_entry() {
    int idx = current_coroutine; // 获取当前协程索引
    if (idx >= 0 && idx < coroutines.size()) {
        std::cout << "Coroutine " << idx << " started\n";

        // 模拟协程任务
        for (int i = 0; i < 3; ++i) {
            std::cout << "Coroutine " << idx << " running step " << i + 1 << "\n";
            // 切换到下一个协程
            int next = (current_coroutine + 1) % coroutines.size();
            int32_t currnet_co = current_coroutine;
            current_coroutine = next; // 更新当前协程索引
            swapcontext(&coroutines[currnet_co]->context, &coroutines[next]->context);
        }

        // 标记协程完成
        coroutines[current_coroutine]->finished = true;
        std::cout << "Coroutine " << idx << " finished\n";
    }

    // 切回主上下文
    swapcontext(&coroutines[current_coroutine]->context, &main_context);
}

// 创建协程
void create_coroutine() {
    auto coroutine = std::make_shared<Coroutine>();
    coroutine->stack = std::make_unique<char[]>(STACK_SIZE);

    // 初始化协程上下文
    getcontext(&coroutine->context);
    coroutine->context.uc_stack.ss_sp = coroutine->stack.get();
    coroutine->context.uc_stack.ss_size = STACK_SIZE;
    coroutine->context.uc_link = &main_context; // 设置协程完成后的返回上下文
    makecontext(&coroutine->context, coroutine_entry, 0);

    coroutines.push_back(coroutine);
}

// 启动协程调度
void run_coroutines() {
    while (true) {
        bool all_finished = true;
        for (size_t i = 0; i < coroutines.size(); ++i) {
            if (!coroutines[i]->finished) {
                all_finished = false;
                current_coroutine = i;
                swapcontext(&main_context, &coroutines[i]->context);
            }
        }
        if (all_finished) break;
    }
}

int main() {
    // 创建三个协程
    create_coroutine();
    create_coroutine();
    create_coroutine();

    // 启动协程调度
    std::cout << "Starting coroutines\n";
    run_coroutines();
    std::cout << "All coroutines finished\n";

    return 0;
}
