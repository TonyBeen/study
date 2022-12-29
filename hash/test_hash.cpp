/*************************************************************************
    > File Name: test_hash.cpp
    > Author: hsz
    > Brief:
    > Created Time: Mon 25 Jul 2022 03:27:07 PM CST
 ************************************************************************/

#include "hash.h"
#include <iostream>
#include <unordered_map>

using namespace std;

static uint32_t compare(const void *k1, const void *k2, size_t ks)
{
    return *((int *)k1) == *((int *)k2);
}

int main(int argc, char **argv)
{
    utp_hash_t *hash = utp_hash_create(79, sizeof(int), sizeof(int), 15, utp_hash_mem, compare);
    int key_1 = 100;
    int *value_1 = (int *)utp_hash_add(&hash, &key_1);
    *value_1 = key_1;

    int key_2 = 20000;
    int *value_2 = (int *)utp_hash_add(&hash, &key_2);
    *value_2 = key_2;

    utp_hash_iterator_t it;
    int *value = nullptr;
    while ((value = (int *)utp_hash_iterate(hash, &it))) {
        if (value) {
            printf("value %d, bucket = %u, elem = %u\n", *value, it.bucket, it.elem);
        }
    }

    return 0;
}
