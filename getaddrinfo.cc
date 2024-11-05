/*************************************************************************
    > File Name: getaddrinfo.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年08月27日 星期二 11时53分04秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <utils/elapsed_time.h>

int main()
{
    struct addrinfo *result = nullptr;
    struct addrinfo hints{};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    eular::ElapsedTime elapsedTime(eular::ElapsedTimeType::MICROSECOND);
    elapsedTime.start();
    int status = getaddrinfo("mail.example.com", NULL, &hints, &result);
    elapsedTime.stop();
    printf("%lu ns\n", elapsedTime.elapsedTime());
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    for(auto res = result; res && res->ai_addr; res = res->ai_next) {
        struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
        char ipstr[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &addr->sin_addr, ipstr, sizeof(ipstr));
        printf("IP Address: %s\n", ipstr);
    }

    if (result) {
        freeaddrinfo(result);
    }
    
    return 0;
}
