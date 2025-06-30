/*************************************************************************
    > File Name: read_zip.cc
    > Author: hsz
    > Brief: g++ read_zip.cc -std=c++11 -lstdc++fs -lzip -o read_zip
    > Created Time: 2025年06月30日 星期一 14时54分07秒
 ************************************************************************/

#include <string>
#include <experimental/filesystem>
#include <vector>
#include <fstream>

#include <zip.h>

#include <utils/CLI11.hpp>

namespace fs = std::experimental::filesystem;

bool write_file_bytes(const std::string &out_path, const std::vector<uint8_t> &data)
{
    try {
        fs::create_directories(fs::path(out_path).parent_path());
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }

    std::ofstream file(out_path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char *>(data.data()), data.size());
    return file.good();
}

int main(int argc, char **argv)
{
    printf("libzip version: " LIBZIP_VERSION "\n");

    std::string zip_path;
    std::string password;
    std::string output_dir;
    CLI::App app{"write_zip"};
    app.add_option("-z,--zip", zip_path, "output file path")->default_val("output.zip")->check(CLI::ExistingFile);
    app.add_option("-o,--output", output_dir, "Output directory")->default_val("");
    app.add_option("-p,--password", password, "zip password")->default_val("");
    CLI11_PARSE(app, argc, argv);

    int32_t zip_error;
    zip_t *zip = zip_open(zip_path.c_str(), ZIP_RDONLY, &zip_error);
    if (zip == nullptr) {
        zip_error_t ziperror;
        zip_error_init_with_code(&ziperror, zip_error);
        fprintf(stderr, "Failed to open zip file %s: %s\n", zip_path.c_str(), zip_error_strerror(&ziperror));
        zip_error_fini(&ziperror);
        return 1;
    }

    if (!password.empty()) {
        if (zip_set_default_password(zip, password.c_str()) < 0) {
            fprintf(stderr, "Failed to set zip password: %s\n", zip_strerror(zip));
            zip_discard(zip);
            return 1;
        }
    }

    zip_int64_t num_entries = zip_get_num_entries(zip, 0);
    if (num_entries == 0) {
        std::cerr << "No files in zip zip." << std::endl;
        zip_close(zip);
        return 1;
    }

    for (zip_uint64_t idx = 0; idx < num_entries; ++idx) {
        zip_stat_t zs;
        if (zip_stat_index(zip, idx, 0, &zs) != 0) {
            std::cerr << "Failed to stat entry #" << idx << " in zip." << std::endl;
            continue;
        }
        std::string entry_name = zs.name;
        fs::path out_path = fs::path(output_dir) / entry_name;
        // 是目录则创建目录并跳过
        if (entry_name.back() == '/') {
            fs::create_directories(out_path);
            continue;
        }

        zip_file_t *zf = nullptr;
        zf = zip_fopen_index(zip, idx, 0); // NOTE zip_fopen_index_encrypted 支持使用不同密码解压
        if (!zf) {
            std::cerr << "Failed to open file '" << entry_name << "' in zip (wrong password or corrupt zip?): " << zip_strerror(zip) << std::endl;
            continue;
        }

        std::vector<uint8_t> buffer(zs.size);
        zip_int64_t nread = zip_fread(zf, buffer.data(), zs.size);
        if (nread < 0 || static_cast<size_t>(nread) != zs.size) {
            std::cerr << "Failed to read file '" << entry_name << "' from zip." << std::endl;
            zip_fclose(zf);
            continue;
        }
        zip_fclose(zf);

        if (!write_file_bytes(out_path.string(), buffer)) {
            std::cerr << "Failed to write output file: " << out_path << std::endl;
            continue;
        }
        std::cout << "Extracted: " << out_path << std::endl;
    }

    zip_close(zip);
    std::cout << "All files extracted." << std::endl;
    return 0;
}
