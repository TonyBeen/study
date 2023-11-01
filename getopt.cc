/*************************************************************************
    > File Name: getopt.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 10 Jan 2022 08:27:36 AM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <string>
#include <iostream>
using namespace std;

int main(int argc, char **argv)
{
    int ch = 0;
    while ((ch = getopt(argc, argv, "hc:p:")) > 0) {
        switch (ch) {
        case 'h':
            printf("h\n");
            break;
        case 'c':
            printf("c: %s\n", optarg);
            break;
        case 'p':
            printf("p: %s\n", optarg);
            break;
        case '?':
            printf("invalid param\n");
            break;
        default:
            printf("default\n");
            break;
        }
    }
    
    return 0;
}
