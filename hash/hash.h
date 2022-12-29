/*************************************************************************
    > File Name: hash.h
    > Author: hsz
    > Brief:
    > Created Time: Mon 25 Jul 2022 02:36:08 PM CST
 ************************************************************************/

#ifndef __HASH_H__
#define __HASH_H__

#include <string.h> // memset
#include <stdlib.h> // malloc
#include <stdint.h>
#include <stdio.h>

typedef uint32_t utp_link_t;

typedef uint32_t (*utp_hash_compute_t)(const void *keyp, size_t keysize);
typedef uint32_t (*utp_hash_equal_t)(const void *key_a, const void *key_b, size_t keysize);

struct utp_hash_t
{
    utp_link_t N;
    uint8_t K;
    uint8_t E;
    size_t count;
    utp_hash_compute_t hash_compute;
    utp_hash_equal_t hash_equal;
    utp_link_t allocated;
    utp_link_t used;
    utp_link_t free;
    utp_link_t inits[0];
};

struct utp_hash_iterator_t
{
    utp_link_t bucket;
    utp_link_t elem;

    utp_hash_iterator_t() : bucket(0xffffffff), elem(0xffffffff) {}
};

uint32_t utp_hash_mem(const void *keyp, size_t keysize);
uint32_t utp_hash_comp(const void *key_a, const void *key_b, size_t keysize);

utp_hash_t *utp_hash_create(int N, int key_size, int total_size, int initial, utp_hash_compute_t hashfun = utp_hash_mem, utp_hash_equal_t eqfun = NULL);
void *utp_hash_lookup(utp_hash_t *hash, const void *key);
void *utp_hash_add(utp_hash_t **hashp, const void *key);
void *utp_hash_del(utp_hash_t *hash, const void *key);

void *utp_hash_iterate(utp_hash_t *hash, utp_hash_iterator_t *iter);
void utp_hash_free_mem(utp_hash_t *hash);


#endif // __HASH_H__