/*************************************************************************
    > File Name: insert_image_to_mysql.cc
    > Author: hsz
    > Brief:
    > Created Time: Mon 13 Dec 2021 09:52:49 PM CST
 ************************************************************************/

#include <mysql/mysql.h>
#include <utils/string8.h>
#include <log/log.h>
#include <iostream>

using namespace std;
using namespace eular;

#define LOG_TAG "test_insert_image"

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
        LOGI("usage: %s file/path", argv[0]);
        return 0;
    }

    connect_mysql();
    const char *imageName = argv[1];
    int fd = open(imageName, O_RDONLY);
    if (fd < 0) {
        LOGE("open error. [%d,%s]", errno, strerror(errno));
        return 0;
    }

    uint32_t imageSize = lseek(fd, 0, SEEK_END);
    char *buf = new char[imageSize];
    LOG_ASSERT(buf, "");
    uint32_t offset = 0;
    lseek(fd, 0, SEEK_SET);
    while (1) {
        int ret = read(fd, buf + offset, 1024);
        if (ret == 0) {
            break;
        }
        if (ret < 0) {
            LOGW("read errno. [%d,%s]", errno, strerror(errno));
            break;
        }
        offset += ret;
    }

    char *realBuf = new char[imageSize * 2 + 1];
    LOG_ASSERT(realBuf, "");

    int64_t ret = mysql_real_escape_string(mysql, realBuf, buf, imageSize);
    char *sql = (char *)malloc(imageSize * 2 + 256);
    LOG_ASSERT(sql, "");
    int64_t sqlLen = sprintf(sql, "insert into image(image_name, image_data) value('%s', '%s')", imageName, realBuf);
    ret = mysql_real_query(mysql, sql, sqlLen);
    if (ret) {
        LOGE("mysql_real_query error. %d,%s", mysql_errno(mysql), mysql_error(mysql));
    } else {
        LOGI("insert OK");
    }
    
    mysql_close(mysql);
    return 0;
}
