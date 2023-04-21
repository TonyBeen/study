/*************************************************************************
    > File Name: read_protobuf.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 13 Jun 2022 05:12:42 PM CST
 ************************************************************************/

#include "msg.pb.h"
#include <iostream>
#include <fstream>
using namespace std;

void listmsg(const eular::ProtoBufTest & msg)
{
    std::cout << "0x" << std::hex  << msg.id() << std::endl;
    std::cout << msg.str() << std::endl;
}
int main(void)
{
    eular::ProtoBufTest msg1;
    fstream input("./log", ios::in | ios::binary);
    if(!msg1.ParseFromIstream(&input)) { // 反序列化
        std::cerr << "Failed to prase address book" << std::endl;
        return -1;
    }
    listmsg(msg1);
    return 0;
}

