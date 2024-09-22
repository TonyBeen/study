/*************************************************************************
    > File Name: test_msgpack.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年09月22日 星期日 14时27分47秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <msgpack.h>

int main() {
    // 创建一个 MessagePack 的缓冲区
    msgpack_sbuffer sbuf;
    msgpack_sbuffer_init(&sbuf);

    // 创建一个 MessagePack 的对象
    msgpack_packer pk;
    msgpack_packer_init(&pk, &sbuf, msgpack_sbuffer_write);

    // 向打包器添加数据
    msgpack_pack_map(&pk, 3); // 创建一个 map，包含 3 个键值对

    // 添加第一个键值对
    msgpack_pack_str(&pk, strlen("name"));
    msgpack_pack_str_body(&pk, "name", strlen("name"));

    msgpack_pack_array(&pk, 3);

    msgpack_pack_double(&pk, 3.14);
    msgpack_pack_str(&pk, strlen("Alice"));
    msgpack_pack_str_body(&pk, "Alice", strlen("Alice"));
    msgpack_pack_true(&pk);

    // 添加第二个键值对
    msgpack_pack_str(&pk, strlen("age"));
    msgpack_pack_str_body(&pk, "age", strlen("age"));
    msgpack_pack_int(&pk, 30);

    // 添加第三个键值对
    msgpack_pack_str(&pk, strlen("country"));
    msgpack_pack_str_body(&pk, "country", strlen("country"));
    msgpack_pack_str(&pk, strlen("Wonderland"));
    msgpack_pack_str_body(&pk, "Wonderland", strlen("Wonderland"));

    // 输出序列化的数据大小
    printf("Serialized size: %zu\n", sbuf.size);

    // 反序列化
    msgpack_unpacked result;
    msgpack_unpacked_init(&result);

    size_t off = 0;
    if (msgpack_unpack_next(&result, sbuf.data, sbuf.size, &off)) {
        // 访问数据
        msgpack_object obj = result.data;

        // 遍历 map
        for (size_t i = 0; i < obj.via.map.size; ++i) {
            msgpack_object_kv kv = obj.via.map.ptr[i];

            // 打印键
            if (kv.key.type == MSGPACK_OBJECT_STR) {
                printf("Key: %.*s\n", (int)kv.key.via.str.size, kv.key.via.str.ptr);
            }

            // 打印值
            if (kv.val.type == MSGPACK_OBJECT_STR) {
                printf("\tValue: %.*s\n", (int)kv.val.via.str.size, kv.val.via.str.ptr);
            } else if (kv.val.type == MSGPACK_OBJECT_POSITIVE_INTEGER) {
                printf("\tValue: %lu\n", kv.val.via.u64);
            } else if (kv.val.type == MSGPACK_OBJECT_ARRAY) {
                printf("\tValue: Array length: %u\n", kv.val.via.array.size);
                for (size_t i = 0; i < kv.val.via.array.size; ++i) {
                    msgpack_object element = kv.val.via.array.ptr[i];
                    printf("\t\t");
                    msgpack_object_print(stdout, element);
                    fflush(stdout);
                    printf("\n");

                    // switch (element.type) {
                    // case MSGPACK_OBJECT_NIL:
                    //     break;
                    // case MSGPACK_OBJECT_BOOLEAN:
                    //     break;
                    // case MSGPACK_OBJECT_POSITIVE_INTEGER:
                    //     printf("\t\tArray[i] %lu: %lld\n", i, element.via.u64);
                    //     break;
                    // case MSGPACK_OBJECT_NEGATIVE_INTEGER:
                    //     printf("\t\tArray[i] %lu: %lld\n", i, element.via.i64);
                    //     break;
                    // case MSGPACK_OBJECT_FLOAT32:
                    //     break;
                    // case MSGPACK_OBJECT_FLOAT64:
                    //     break;
                    // case MSGPACK_OBJECT_STR:
                    //     break;
                    // case MSGPACK_OBJECT_ARRAY:
                    //     break;
                    // case MSGPACK_OBJECT_MAP:
                    //     break;
                    // case MSGPACK_OBJECT_BIN:
                    //     break;
                    // case MSGPACK_OBJECT_EXT:
                    //     break;
                    // default:
                    //     break;
                    // }
                }
            }
        }
    } else {
        printf("Failed to unpack\n");
    }

    // 释放资源
    msgpack_unpacked_destroy(&result);
    msgpack_sbuffer_destroy(&sbuf);

    return 0;
}
