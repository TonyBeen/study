/*************************************************************************
    > File Name: insert_image.cc
    > Author: hsz
    > Brief:
    > Created Time: Sat 08 Jan 2022 07:26:03 PM CST
 ************************************************************************/

#include <sqlutils/mysql.h>
#include <utils/utils.h>
#include <log/log.h>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <string>

#define LOG_TAG "insert image to mysql"

using namespace std;
using namespace eular;

static MySqlConn *mysql = nullptr;
static std::string path;
static std::string host;
static std::string dbName;
static std::string user;
static std::string passwd;
static uint32_t count = 0;
static uint32_t all = 0;

int insert_into_mysql(const std::string &fileName, int fd)
{
    LOG_ASSERT(mysql && fd > 0, "");

    auto deleter = [](char *ptr) {
        if (ptr) {
            delete []ptr;
        }
    };

    uint32_t imageSize = lseek(fd, 0, SEEK_END);
    char *buf = new char[imageSize];
    std::shared_ptr<char> tmp(buf, deleter);
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
            return -1;
        }
        offset += ret;
    }

    uint32_t fmtSize = 0;
    std::shared_ptr<char> ptr = mysql->FormatHexString(buf, offset, fmtSize);
    if (nullptr == ptr) {
        return -1;
    }
    std::shared_ptr<char> sql(new char[fmtSize + 256], deleter);

    int64_t sqlLen = sprintf(sql.get(), "insert into image(image_name, image_data) value('%s', '%s')",
        fileName.c_str(), ptr.get());
    return mysql->SqlCommond(sql.get(), sqlLen);
}

void printfUsage(int argc, char **argv)
{
    printf("usage: %s [opt]\n", argv[0]);
    printf("-h for help\n");
    printf("-f /path/to/file/\n");
    printf("-d database_name\n");
    printf("-H mysql_host\n");
    printf("-u user_name\n");
    printf("-p user_password\n");
    
    exit(0);
}

bool Init(int argc, char **argv)
{
    dbName = "userdb";
    host = "127.0.0.1";
    user = "mysql";
    passwd = "eular123";

    int ch = 0;
    while ((ch = getopt(argc, argv, "hf:d:H:u:p:")) > 0) {
        switch (ch) {
        case 'h':
            printfUsage(argc, argv);
            break;
        case 'f':
            path = optarg;
            break;
        case 'd':
            dbName = optarg;
            break;
        case 'H':
            host = optarg;
            break;
        case 'u':
            user = optarg;
            break;
        case 'p':
            passwd = optarg;
        default:
            break;
        }
    }

    mysql = new MySqlConn(user.c_str(), passwd.c_str(), dbName.c_str(), host.c_str());
    if (mysql && mysql->isSqlValid()) {
        return true;
    }

    if (mysql) {
        printf("connect mysql error. [%u,%s]\n", mysql->getErrno(), mysql->getErrorStr());
    }
    return false;
}

int main(int argc, char **argv)
{
    LOG_ASSERT(Init(argc, argv), "");

    std::vector<std::string> ret = getdir(path);
    if (ret.size() == 0) {
        printf("opendir error. [%d,%s]\n", errno, strerror(errno));
        return 0;
    }

    printf("begin to insert image to mysql. user: %s, database: %s\n", dbName.c_str(), user.c_str());
    for (auto fileName : ret) {
        if (isPicture(fileName)) {
            all++;
            std::string file = path + fileName;
            int fd = open(file.c_str(), O_RDONLY);
            if (fd < 0) {
                continue;
            }

            printf("\tinserting %s\n", file.c_str());
            int ret = insert_into_mysql(fileName, fd);
            if (ret < 0) {
                printf("\tinsert %s failed\n", fileName.c_str());
            } else {
                count++;
                printf("\tinsert %s over\n", fileName.c_str());
            }
            close(fd);
        }
    }
    printf("succeed: %u, failed: %u, all: %u\n", count, all - count, all);
    
    return 0;
}
