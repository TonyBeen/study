/*************************************************************************
    > File Name: test_log4cplus.cc
    > Author: hsz
    > Brief:
    > Created Time: 2025年07月24日 星期四 11时53分11秒
 ************************************************************************/

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/initializer.h>
#include <log4cplus/fileappender.h>

int main(int argc, char **argv)
{
    log4cplus::initialize();
    log4cplus::Logger logger = log4cplus::Logger::getInstance("FileLogger");

    // 创建每日滚动文件Appender
    log4cplus::SharedAppenderPtr appender(
        new log4cplus::DailyRollingFileAppender(
            "app.log", // 日志文件路径
            log4cplus::DAILY, // 每天滚动一次
            true, // 追加模式
            10    // 保留10个备份文件
        )
    );
    appender->setName("FileAppender");

    // 设置日志格式
    std::string pattern = "%d{%H:%M:%S} [%t] %-5p %c - %m%n"; // 时间、线程ID、级别、Logger名、消息
    appender->setLayout(std::unique_ptr<log4cplus::Layout>(new log4cplus::PatternLayout(pattern)));

    logger.addAppender(appender);
    logger.setLogLevel(log4cplus::DEBUG_LOG_LEVEL);

    LOG4CPLUS_WARN_FMT(logger, "Hello, World! %s", "This is a test message with format support.");
    return 0;
}
