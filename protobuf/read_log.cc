#include "log.pb.h"
#include <fstream>
#include <chrono>
#include <unistd.h>

void listmsg(const log::LogContext &msg)
{
    std::cout << "0x" << msg.level() << std::endl;
    std::cout << msg.millisecond() << std::endl;
    std::cout << msg.pid() << std::endl;
    std::cout << msg.tid() << std::endl;
    std::cout << msg.tag() << std::endl;
    std::cout << msg.msg() << std::endl;
    std::cout << msg.enablecolor() << std::endl;
}
int main(void)
{
    log::LogContext context;
    std::fstream input("./log", std::ios::in | std::ios::binary);

    // ParseFromArray
    if (!context.ParseFromIstream(&input)) { // 反序列化
        std::cerr << "Failed to prase address book" << std::endl;
        return -1;
    }

    listmsg(context);
    return 0;
}
