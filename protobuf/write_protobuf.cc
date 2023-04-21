/*************************************************************************
    > File Name: test_protobuf.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 13 Jun 2022 03:52:25 PM CST
 ************************************************************************/

#include "msg.pb.h"
#include <fstream>
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
    eular::ProtoBufTest test;
    test.set_id(0x1024);
    test.set_str("Hello");

    fstream output("./log", ios::out | ios::trunc | ios::binary);

    if (!test.SerializeToOstream(&output)) {    // 序列化
        std::cerr << "Failed to write msg." << std::endl;
        return -1;
    }
    return 0;
}
