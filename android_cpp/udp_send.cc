/*************************************************************************
    > File Name: udp_send.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年03月21日 星期五 11时16分34秒
 ************************************************************************/

#include <iostream>
#include <cstring>
#include <set>
#include <regex>
#include <iostream>
#include <fstream>

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include <sched.h>
#include <pthread.h>

#define SERVER_PORT 12345
#define BUFFER_SIZE 65536

std::set<std::string> g_nicSet;

uint32_t g_upload_speed = 50;
uint32_t g_interval = 100;
struct sockaddr_in g_servaddr;

bool thread_flag[10];

struct thread_data
{
    int32_t sockfd;
    int32_t flag_index;
};

void func(uint64_t &_out_bytes, uint64_t &_in_bytes, uint32_t &_last_get_band_time, uint64_t &output_band)
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

            // printf("\t%s\n", l.c_str());
            if (g_nicSet.size() > 0 && g_nicSet.find(nicName) == g_nicSet.end()) {
                continue;
            }

            auto din = strtod((const char *) match[2].str().data(), nullptr);
            auto dout = strtod((const char *) match[3].str().data(), nullptr);
            // printf("\t%s, din: %f, dout: %f\n", nicName.c_str(), din, dout);
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
            output_band = out_band >> 20;

            printf("output_band:%llu Mbps, outband: %llu bps\n", output_band, out_band);
        }
        _last_get_band_time = time;
        _out_bytes = out_bytes;
        _in_bytes = in_bytes;
    }
}

void *update_func(void *arg)
{
    thread_data *data = (thread_data *)arg;
    // pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    // pthread_detach(pthread_self());

    while (!thread_flag[data->flag_index]) {
        std::string message;
        message.resize(1300);

        // 发送数据
        auto nsend = sendto(data->sockfd, message.c_str(), message.size(), 0, (const struct sockaddr *)&g_servaddr, sizeof(g_servaddr));
        if (nsend < 0) {
            perror("sendto error");
            abort();
        }
        // pthread_testcancel();
        usleep(g_interval);
    }

    return nullptr;
}

void* thread_func(void* arg)
{
    int32_t sockfd = (intptr_t)arg;
    uint64_t in_bytes = 0;
    uint64_t out_bytes = 0;
    uint32_t last_get_band_time = 0;
    uint64_t output_band = 0;
    std::vector<pthread_t> tid_vec;
    std::vector<thread_data *> thread_data_vec;
    while (true) {
        func(out_bytes, in_bytes, last_get_band_time, output_band);
        if (tid_vec.size() < 3) {
            if (output_band < g_upload_speed) {
                pthread_t tid;
                thread_data data = {sockfd, (int32_t)tid_vec.size()};
                thread_data_vec.push_back(new thread_data(data));
                thread_flag[data.flag_index] = false;
                pthread_create(&tid, nullptr, update_func, thread_data_vec.back());
                tid_vec.push_back(tid);
            } else if (output_band > 2 * g_upload_speed) {
                // pthread_cancel(tid_vec.back());
                thread_data *data = thread_data_vec.back();
                thread_flag[data->flag_index] = true;
                pthread_join(tid_vec.back(), nullptr);
                free(data);
                thread_data_vec.pop_back();
                tid_vec.pop_back();
            }
        }
        usleep(1000 * 1000);
    }

    return nullptr;
}

int main(int argc, char **argv)
{
    int opt;
    const char *server_address = "127.0.0.1";
    int32_t port = SERVER_PORT;
    const char *nic = "wlan0";
    int32_t cpu_index = 0;
    while ((opt = getopt(argc, argv, "s:p:i:c:m:ht:")) != -1) {
        switch (opt) {
        case 's':
            server_address = optarg;
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'i':
            nic = optarg;
            break;
        case 'c':
            cpu_index = atoi(optarg);
            break;
        case 'm':
            g_upload_speed = atoi(optarg);
            break;
        case 'h':
            fprintf(stderr, "Usage: %s -s server_address -p port\n", argv[0]);
            return 0;
        case 't':
            g_interval = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -s server_address -p port\n", argv[0]);
            return 0;
        }
    }

    // 解析服务器地址
    struct hostent* host = gethostbyname(server_address);
    if (host == nullptr) {
        std::cerr << "Failed to resolve hostname: " << server_address << std::endl;
        return 1;
    }

    int32_t sockfd;
    char buffer[BUFFER_SIZE];
    memset(&g_servaddr, 0, sizeof(g_servaddr));
    g_servaddr.sin_family = AF_INET;
    g_servaddr.sin_port = htons(port);
    memcpy(&g_servaddr.sin_addr, host->h_addr, host->h_length);

    // 创建UDP套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }

    // 绑定网卡
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, nic, strlen(nic)) < 0) {
        perror("setsockopt error");
    }

    int32_t num_cpus = sysconf(_SC_NPROCESSORS_CONF);
    if (cpu_index >= num_cpus) {
        std::cerr << "Invalid CPU index: " << cpu_index << std::endl;
        close(sockfd);
        return 1;
    }

    g_nicSet.emplace(nic);

    // 绑定cpu
    // cpu_set_t cpuset;
    // CPU_ZERO(&cpuset);
    // CPU_SET(0, &cpuset);
    // sched_setaffinity(pthread_self(), sizeof(cpu_set_t), &cpuset);

    for (int i = 0; i < 10; i++) {
        thread_flag[i] = false;
    }

    thread_func((void *)sockfd);

    close(sockfd);
    return 0;
}
