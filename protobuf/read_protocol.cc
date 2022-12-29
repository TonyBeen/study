/*************************************************************************
    > File Name: read_protocol.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 20 Jun 2022 11:23:53 AM CST
 ************************************************************************/

#include "protocol.pb.h"
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char **argv)
{
    fstream input("./log", ios::in | ios::binary);
    P2SResponse res;
    if (!res.ParseFromIstream(&input)) {
        std::cerr << "Failed to prase address book" << std::endl;
        return -1;
    }

    std::cout << res.statuscode() << std::endl;
    std::cout << res.msg() << std::endl;

    for (int i = 0; i < res.info_size(); ++i) {
        std::cout << res.mutable_info(i)->peer_uuid() << std::endl;
        std::cout << res.mutable_info(i)->peer_name() << std::endl;
    }

    return 0;
}
