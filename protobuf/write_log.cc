/*************************************************************************
    > File Name: log.cc
    > Author: hsz
    > Brief:
    > Created Time: Thu 20 Apr 2023 07:00:39 PM CST
 ************************************************************************/

#include "log.pb.h"
#include <fstream>
#include <chrono>
#include <unistd.h>

int main()
{
    log::LogContext context;
    context.set_level(0);

    std::chrono::steady_clock::time_point tm = std::chrono::steady_clock::now();
    std::chrono::milliseconds mills = 
        std::chrono::duration_cast<std::chrono::milliseconds>(tm.time_since_epoch());
    
    context.set_millisecond(mills.count());

    context.set_pid(getpid());
    context.set_tid(syscall(186));
    context.set_tag("test");
    context.set_msg("hello world");
    context.set_enablecolor(false);

    std::fstream output("./log", std::ios::out | std::ios::trunc | std::ios::binary);

    // SerializeToArray
    if (!context.SerializeToOstream(&output)) {    // 序列化
        std::cerr << "Failed to write msg." << std::endl;
        return -1;
    }

    return 0;
}