/*************************************************************************
    > File Name: write_zip.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年06月30日 星期一 14时54分13秒
 ************************************************************************/

#include <string>

#include <zip.h>

#include <utils/CLI11.hpp>

int main(int argc, char **argv)
{
    printf("libzip version: " LIBZIP_VERSION "\n");

    std::string file_path;
    std::string out_path;
    std::string password;
    CLI::App app{"write_zip"};
    app.add_option("-f,--file", file_path, "file to be zipped")->required();
    app.add_option("-o,--out", out_path, "output file path")->default_val("output.zip");
    app.add_option("-p,--password", password, "zip password")->default_val("");
    CLI11_PARSE(app, argc, argv);

    int32_t zip_error;
    zip_t *zip = zip_open(out_path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &zip_error);
    if (zip == nullptr) {
        zip_error_t ziperror;
        zip_error_init_with_code(&ziperror, zip_error);
        fprintf(stderr, "Failed to open zip file %s: %s\n", out_path.c_str(), zip_error_strerror(&ziperror));
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

    zip_source_t *source = zip_source_file(zip, file_path.c_str(), 0, 0);
    if (source == nullptr) {
        fprintf(stderr, "Failed to create zip source from file %s: %s\n", file_path.c_str(), zip_strerror(zip));
        zip_discard(zip);
        return 1;
    }

    zip_int64_t idx = zip_file_add(zip, file_path.c_str(), source, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8);
    if (idx < 0) {
        fprintf(stderr, "Failed to add file %s to zip: %s\n", file_path.c_str(), zip_strerror(zip));
        zip_source_free(source);
        zip_discard(zip);
        return 1;
    }

    if (!password.empty()) {
        if (zip_file_set_encryption(zip, idx, ZIP_EM_AES_256, password.c_str()) != 0) {
            std::cerr << "Failed to set encryption: " << zip_strerror(zip) << std::endl;
            zip_close(zip);
            return 1;
        }
    }

    if (zip_close(zip) < 0) {
        fprintf(stderr, "Failed to close zip file %s: %s\n", out_path.c_str(), zip_strerror(zip));
        zip_discard(zip);
        return 1;
    }
    return 0;
}
