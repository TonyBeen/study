/*************************************************************************
    > File Name: test_redis_hash.cc
    > Author: hsz
    > Brief:
    > Created Time: Thu 20 Jan 2022 04:40:20 PM CST
 ************************************************************************/

#include <hiredis/hiredis.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <iostream>

using namespace std;

static redisContext *redis;

void hash_init()
{
    redisReply *reply = (redisReply *)redisCommand(redis, "hmset fortest name eular sex male age 18");
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        printf("%s() hmset error. [%s] [%s]\n", __func__, reply->str, redis->errstr);
        return;
    }

    assert(strcasecmp(reply->str, "OK") == 0);

    freeReplyObject(reply);
}

void hash_del()
{
    redisReply *reply = (redisReply *)redisCommand(redis, "hdel fortest name sex age");   // 删除键fortest, 删除fortest所有域会使键一起删除
    if (reply) {
        freeReplyObject(reply);
    }
}

void test_hmgetall()
{
    hash_init();

    redisReply *reply = (redisReply *)redisCommand(redis, "hgetall fortest");
    assert(reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s() hmset error. %s %s\n", __func__, reply->str, redis->errstr);
        return;
    }

    printf("reply type: %d, element: %zu, str: %s\n", reply->type, reply->elements, reply->str);

    redisReply *filed, *value;
    for (int i = 0; i < reply->elements; i += 2) {
        filed = reply->element[i];
        value = reply->element[i + 1];
        if (value && filed) {
            printf("[%d] \"%s\" -- \"%s\"\n", i, filed->str, value->str);
        }
    }

    freeReplyObject(reply);
    hash_del();
}

void test_hdel()
{
    hash_init();

    redisReply *reply = (redisReply *)redisCommand(redis, "hdel fortest");   // 域test不存在
    assert(reply);
    if (reply->type == REDIS_REPLY_ERROR) {
        printf("%s() hdel error. %s %s\n", __func__, reply->str, redis->errstr);
        return;
    }

    printf("type: %d, ret = %lld\n", reply->type, reply->integer);
    assert(reply->integer == 1);

    // 不存在的hash键也会返回REDIS_REPLY_ARRAY，但elements为0
    // redisCommand(redis, "hdel fortest name sex age");

    hash_del();
}

void several_key_init()
{
    for (int i = 0; i < 3; ++i) {
        redisReply *reply = (redisReply *)redisCommand(redis, "set test_mget%d cycle-%d", i, i);
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            printf("%s() set error. [%s] [%s]\n", __func__, reply->str, redis->errstr);
            return;
        }
        printf("%s() reply type: %d, %s\n", __func__, reply->type, reply->str);
        freeReplyObject(reply);
    }
}

void several_key_del()
{
    for (int i = 0; i < 3; ++i) {
        redisReply *reply = (redisReply *)redisCommand(redis, "del test_mget%d", i);
        if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
            printf("%s() set error. [%s] [%s]\n", __func__, reply->str, redis->errstr);
            return;
        }
        printf("%s() reply type: %d, %lld\n", __func__, reply->type, reply->integer);
        freeReplyObject(reply);
    }
}

void test_mget()
{
    several_key_init();
    redisReply *reply = (redisReply *)redisCommand(redis, "mget test_mget1 test_mget2");
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        printf("%s() set error. [%s] [%s]\n", __func__, reply->str, redis->errstr);
        return;
    }
    printf("reply type: %d\n", reply->type);
    redisReply *curr;
    for (int i = 0; i < reply->elements; ++i) {
        curr = reply->element[i];
        if (curr != nullptr) {
            printf("[%d] %s\n", i, curr->str);
        }
    }
    several_key_del();
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("input redis password\n");
        return 0;
    }
    redis = redisConnect("127.0.0.1", 6379);
    assert(redis != nullptr && redis->err == 0);
    redisReply *reply = (redisReply *)redisCommand(redis, "auth %s", argv[1]);
    if (reply == nullptr || reply->type != REDIS_REPLY_STATUS) {
        printf("auth error.");
        if (reply) {
            printf("[%s]\n", reply->str);
            freeReplyObject(reply);
        } else {
            printf("\n");
        }
        return 0;
    }
    printf("redis connected! [%d, %s]\n", reply->type, reply->str);

    test_hmgetall();
    test_hdel();
    test_mget();

    redisFree(redis);
    return 0;
}
