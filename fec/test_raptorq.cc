/*************************************************************************
    > File Name: test_raptorq.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年09月04日 星期三 14时13分33秒
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <algorithm>

#include <RaptorQ/cRaptorQ.h>

void readFile(const std::string& filename, std::string& data) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return;
    }

    data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

int main()
{
    std::string filename = "2m.dat"; // 输入文件名 2m.dat
    std::string file_data;

    // 读取文件内容
    readFile(filename, file_data);

    file_data.resize(16 * 1280);
    file_data[0] = '\x2';
    printf("read '%s' over, size = %zu\n", filename.c_str(), file_data.size());

    uint16_t subsymbol = 512;
    uint16_t symbol_size = 2048;
    size_t   max_memory = 2048;

    /**
     * 一个文件经喷泉码编码生成多个block
     *
     * 一个blcok由多个symbol组成. block之间含有的symbol个数相差不超过1(测试结果)
     * 
     * 一个symbol的大小由 symbol_size控制
     *
     * subsymbol 和 max_memory 以及 源数据大小 影响block中每个symbol个数
     * 
     * 大概计算公式: 
     * block个数 = std::ceil(data_size / (max_memory * symbol_size / subsymbol))
     * 
     * symbol个数 = std::ceil(data_size / symbol_size);
     * 
     * block中含有symbol个数 = symbol个数 / block个数
     * 
     * symbol平均分到每个block中, 假设有17个symbol, 4个block, 不会是 [5, 5, 4, 3]这样分配
     * 而是 [5, 4, 4, 4]
     *
     * oti_scheme 和 oti_common 只与data_size subsymbol symbol_size max_memory有关, 与数据无关
     */
    RaptorQ_ptr *pEncoder = nullptr;
    pEncoder = RaptorQ_Enc(ENC_8, (void *)file_data.data(), file_data.size(), subsymbol, symbol_size, max_memory);
    if (pEncoder == NULL) {
        fprintf(stderr, "Coud not initialize encoder.\n");
        return 0;
    }

    RaptorQ_precompute(pEncoder, 1, false);

    printf("symbol_size = %u\n", RaptorQ_symbol_size(pEncoder)); // 结果为 symbol_size

    auto blocks = RaptorQ_blocks(pEncoder);
    std::map<uint32_t, std::vector<uint8_t>> symbol_map;
    for (uint8_t block = 0; block < blocks; ++block) {
        uint32_t sym_for_blk = RaptorQ_symbols(pEncoder, block);
        printf("Block: %i, sym_for_blk = %u\n", block, sym_for_blk);

        uint16_t repair_symbol = 1;
        uint32_t data_size = symbol_size / sizeof(uint8_t); // 因为类型选的是 ENC_8

        // 编码符号
        for (uint32_t source = 0; source < sym_for_blk; ++source) {
            uint32_t id = RaptorQ_id(source, block);
            printf("symbol Id: %u\n", id);
            auto &symbol_vec = symbol_map[id];
            symbol_vec.reserve(data_size);

            uint8_t *data_buffer = symbol_vec.data();
            uint64_t written = RaptorQ_encode(pEncoder, (void **)&data_buffer, data_size, source, block);
            if (written != data_size) {
                fprintf(stderr, "Error in getting source symbol\n");
                RaptorQ_free(&pEncoder);
                return -1;
            }
            // data_buffer 现在指向 data_buffer + written位置
        }

        // 获取修复符号
        uint32_t max_repair_symbol = RaptorQ_max_repair (pEncoder, block);
        printf("最大修复符号个数: %u, 默认生成 %u 个修复符号\n", max_repair_symbol, repair_symbol);
        for (uint32_t sym_rep = sym_for_blk; repair_symbol && sym_rep < max_repair_symbol; ++sym_rep, --repair_symbol) {
            uint32_t id = RaptorQ_id(sym_rep, block);
            printf("symbol Id: %u\n", id);
            auto &symbol_vec = symbol_map[id];
            symbol_vec.reserve(data_size);

            uint8_t *data_buffer = symbol_vec.data();
            uint64_t written = RaptorQ_encode (pEncoder, (void **)&data_buffer, data_size, sym_rep, block);
            if (written != data_size) {
                fprintf(stderr, "Error in getting source symbol\n");
                RaptorQ_free(&pEncoder);
                return -1;
            }
        }
    }

    // 编码完毕 释放资源 获取解码所需数据
    uint32_t oti_scheme = RaptorQ_OTI_Scheme(pEncoder);
    uint64_t oti_common = RaptorQ_OTI_Common(pEncoder);
    printf("oti_scheme = %u, oti_common = %lu\n", oti_scheme, oti_common);
    RaptorQ_free(&pEncoder);

    printf("\n--------------------------------------------------------------------------------\n\n");

    // 编码选择哪个类型, 解码也需一致
    RaptorQ_ptr *pDecode = RaptorQ_Dec(DEC_8, oti_common, oti_scheme);
    if (pDecode == NULL) {
        fprintf(stderr, "Could not initialize decoder!\n");
        return -1;
    }

    blocks = RaptorQ_blocks(pDecode);
    printf("blocks = %u\n", blocks);

    for (auto it = symbol_map.begin(); it != symbol_map.end(); ++it) {
        // printf("Id = %u\n", it->first);
        uint8_t *symbol_data = it->second.data();
        uint32_t symbol_data_size = RaptorQ_symbol_size(pDecode) / sizeof(uint8_t);

        if (!RaptorQ_add_symbol_id(pDecode, (void **)&symbol_data, symbol_data_size, it->first)) {
            // 当数据足够恢复的时候再添加数据会报错, 此时继续即可
            fprintf(stderr, "Error: couldn't add the symbol to the decoder\n");
            continue;
        }
    }

    auto decode_size = RaptorQ_bytes(pDecode) / sizeof(uint8_t);
    // 如果是其他类型需要计算是否能整除

    std::string decode_file_data;
    decode_file_data.resize(decode_size);
    void *decode_data = (void *)decode_file_data.data();
    uint64_t written = RaptorQ_decode (pDecode, (void **)&decode_data, decode_size);
    if ((written != decode_size) || (decode_size != file_data.size())) {
        fprintf(stderr, "Couldn't decode: %zu - %lu\n", file_data.size(), written);
    } else {
        printf("Decoded: %lu\n", written);
    }

    if (decode_file_data != file_data) {
        printf("Inconsistent data!\n");
    }

    RaptorQ_free(&pDecode);
    return 0;
}