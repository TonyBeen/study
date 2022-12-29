/*************************************************************************
    > File Name: write_protocol.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 20 Jun 2022 11:23:45 AM CST
 ************************************************************************/

#include "protocol.pb.h"
#include <iostream>
#include <fstream>

using namespace std;

int main(int argc, char **argv)
{
    P2SResponse res;
    res.set_statuscode(0);
    res.set_msg("ok");
    PeerInfo *info = res.add_info();
    info->set_peer_uuid("uuid");
    info->set_peer_name("name");

    fstream output("./log", ios::out | ios::trunc | ios::binary);
    if (res.SerializePartialToOstream(&output) == false) {
        std::cerr << "Failed to write msg." << std::endl;
        return -1;
    }

    return 0;
}
