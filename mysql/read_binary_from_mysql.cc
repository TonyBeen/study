/*************************************************************************
    > File Name: read_binary_from_mysql.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 13 Dec 2021 11:11:17 PM CST
 ************************************************************************/

#include <mysql/mysql.h>
#include <utils/string8.h>
#include <log/log.h>
#include <iostream>

using namespace std;
using namespace Jarvis;

#define LOG_TAG "test_read_image"

MYSQL *mysql = nullptr;
void connect_mysql()
{
    mysql = mysql_init(mysql);
    MYSQL *ret = mysql_real_connect(mysql, "127.0.0.1", "mysql", "627096590", "userdb", 3306, nullptr, 0);
    if (ret == nullptr) {
        LOGE("mysql_real_connect error. %d,%s", mysql_errno(mysql), mysql_error(mysql));
        exit(0);
    }

    LOGI("%s() mysql init ok", __func__);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        LOGI("usage: %s filename", argv[0]);
        return 0;
    }
    connect_mysql();
    String8 name;
    MYSQL_RES *res;
    MYSQL_ROW row;
    uint64_t *rowLen;
    uint32_t numRows;

    const char *fileName = argv[1];
    char sql[256] = {0};
    int sqlLen = sprintf(sql, "select image_data from image limit 1");
    int ret = mysql_real_query(mysql, sql, sqlLen);
    if (ret) {
        LOGE("mysql_real_query error. %d,%s", mysql_errno(mysql), mysql_error(mysql));
        goto error;
    } else {
        LOGI("read OK");
    }
    res = mysql_store_result(mysql);
    if (res == nullptr) {
        LOGE("mysql_store_result error. %d,%s", mysql_errno(mysql), mysql_error(mysql));
        goto error;
    }

    numRows = mysql_num_rows(res);  // 多少行数据
    for (int i = 0; i < numRows; ++i) {
        row = mysql_fetch_row(res);
        rowLen = mysql_fetch_lengths(res);

        uint64_t len = rowLen[0];
        uint64_t offset = 0;
        LOGI("data len = %llu", len);
        int fd = open(fileName, O_RDWR | O_CREAT, 0664);
        if (fd < 0) {
            LOGW("open errno. [%d,%s]", errno, strerror(errno));
            goto error;
        }
        while (1) {
            int ret = write(fd, row[0] + offset, 1024);
            if (ret == 0) {
                break;
            }
            if (ret < 0) {
                LOGW("write errno. [%d,%s]", errno, strerror(errno));
                goto error;
            }
            offset += ret;
        }
    }



error:
    if (res) {
        mysql_free_result(res);
    }
    mysql_close(mysql);
    return 0;
}
