/*************************************************************************
    > File Name: raptorq_bench.cc
    > Author: hsz
    > Brief:
    > Created Time: Fri 15 Nov 2024 06:01:26 PM CST
 ************************************************************************/

#include <assert.h>
#include <iostream>
#include <filesystem>
#include <map>

#include <raptorq/encoder.h>
#include <raptorq/decoder.h>

struct Block {
    int32_t file_id;
    int32_t file_size;

    std::vector<uint32_t>   piece_id_vec;
    std::vector<void *>     piece_data_vec;
};


int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("usage: %s /path/to/file\n", argv[0]);
        return 0;
    }

    const char *filePath = argv[1];
    if (!std::filesystem::is_regular_file(filePath)) {
        printf("Not a regular file: %s\n", filePath);
        return 0;
    }

    std::map<int32_t, Block> file_block_map;

    int32_t file_id = 0;
    int32_t ppb = 10;
    int32_t repair_piece = 2;
    int32_t piece_size = 1280;

    raptorq::Encoder::SP encoder = std::make_shared<raptorq::Encoder>();
    raptorq::Decoder::SP decoder = std::make_shared<raptorq::Decoder>();

    std::string file_content;
    file_content.resize((ppb + repair_piece) * piece_size);
    FILE *fp = fopen(filePath, "r");
    while (!feof(fp)) {
        auto read_size = fread(file_content.data(), 1, file_content.size(), fp);
        Block block;
        block.file_id = file_id++;
        block.file_size = read_size;
        printf("read_size = %zu\n", read_size);

        encoder->reset(ppb, piece_size, repair_piece, file_content.data());
        encoder->precompute(1);
        assert(encoder->encode(block.piece_id_vec, block.piece_data_vec));

        file_block_map[block.file_id] = block;
    }
    fclose(fp);
    std::string file_name = std::string(filePath) + ".cp";
    fp = fopen(file_name.c_str(), "w+");
    for (auto &it : file_block_map) {
        decoder->reset(ppb, piece_size);
        for (int32_t i = 0; i < it.second.piece_id_vec.size(); ++i) {
            if (!decoder->feedPiece(it.second.piece_id_vec[i], it.second.piece_data_vec[i])) {
                break;
            }
        }

        decoder->precompute(1);
        assert(decoder->decode(file_content));
        for (auto &block_it : it.second.piece_data_vec) {
            free(block_it);
            block_it = nullptr;
        }
        printf("file_content = %zu\n", file_content.size());

        assert(file_content.size() >= it.second.file_size);
        fwrite(file_content.c_str(), it.second.file_size, 1, fp);
    }

    fclose(fp);
    return 0;
}
