/*************************************************************************
    > File Name: get_dev.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年03月21日 星期五 11时34分11秒
 ************************************************************************/

/*************************************************************************
    > File Name: get_dev.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年11月13日 星期三 11时59分26秒
 ************************************************************************/

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <regex>

#include <stdatomic.h>

#include <unistd.h>

std::set<std::string> g_nicSet;

void func(uint64_t &_out_bytes, uint64_t &_in_bytes, uint32_t &_last_get_band_time)
{
    std::string band_file("/proc/net/dev");

    std::ifstream input_file(band_file);
    if (!input_file.is_open()) {
        printf("[APP] get band file fail\n");
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    while  (getline(input_file, line)) {
        lines.push_back(line);
    }

    std::regex band_regex("(.*):\\s*(\\d+)\\s*\\d+\\s*\\d+\\s*\\d+\\s*\\d+\\s*\\d+\\s*\\d+\\s*\\d+\\s*(\\d+).*");
    std::smatch match;
    uint64_t out_band = 0;
    uint64_t in_band = 0;
    double in_bytes = 0;
    double out_bytes = 0;

    for (auto l: lines) {
        if (std::regex_match(l, match, band_regex) && match.size() == 4) {
            if (strstr(match[1].str().data(), "lo")) {
                continue;
            }

            auto nicName = match[1].str();
            nicName.erase(std::remove(nicName.begin(), nicName.end(), ' '), nicName.end());

            //printf("\t%s\n", l.c_str());
            if (g_nicSet.size() > 0 && g_nicSet.find(nicName) == g_nicSet.end()) {
                continue;
            }

            auto din = strtod((const char *) match[2].str().data(), nullptr);
            auto dout = strtod((const char *) match[3].str().data(), nullptr);
            in_bytes += din;
            out_bytes += dout;
        }
    }

    auto time = std::time(nullptr);
    {
        uint64_t _get_orig_rate = 0;
        uint64_t _get_orig_band = 0;
        if (_last_get_band_time > 0 && _in_bytes > 0 && _out_bytes > 0) {
            in_band = ((in_bytes - _in_bytes) * 8) / ((time - _last_get_band_time));
            out_band = ((out_bytes - _out_bytes) * 8) / ((time - _last_get_band_time));
            if (out_band > 0) {
                _get_orig_rate = (100 * in_band) / out_band;
            }
            _get_orig_band = in_band >> 20;
            uint64_t output_band = out_band >> 20;

            printf("----> _get_orig_band:%lu, output_band:%lu, get_orig_rate:%lu, inband:%lu, outband:%lu\n", _get_orig_band, output_band, _get_orig_rate, in_band, out_band);
        }
        _last_get_band_time = time;
        _out_bytes = out_bytes;
        _in_bytes = in_bytes;
    }
}

int main(int argc, char **argv)
{
    g_nicSet = {"macvlan4vr0", "macvlan4vr1", "macvlan4vr2", "macvlan4vr3", "macvlan4vr4", "macvlan4vr5", "macvlan4vr6"};
    uint64_t _out_bytes = 0;
    uint64_t _in_bytes = 0;
    uint32_t _times = 0;
    for (int32_t i = 0; i < 60; ++i) {
        func(_out_bytes, _in_bytes, _times);
        // printf(" ===> _out_bytes = %lu, _in_bytes = %zu, _times = %u\n", _out_bytes, _in_bytes, _times);
        printf("\n");
        sleep(3);
    }

    atomic_int atomic_counter;

    return 0;
}
