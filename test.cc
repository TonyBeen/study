/*************************************************************************
    > File Name: test.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 06 Jan 2026 04:23:21 PM CST
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <array>

#include <fmt/fmt.h>


class Error {
public:
    static constexpr size_t MAX_MSG_LEN = 256;

    // 方法1: 格式化到内部 std::string（直接追加，避免临时对象）
    // template <typename... Args>
    // Error(int32_t code, fmt::format_string<Args...> fmt, Args&&... args)
    //     : code_(code) {
    //     message_.reserve(128);
    //     fmt::format_to(std::back_inserter(message_), fmt, std::forward<Args>(args)...);
    // }

    // 方法2: 格式化到固定大小缓冲区（零堆分配）
    template <typename... Args>
    Error(int32_t code, fmt::format_string<Args...> fmt, Args&&... args)
        : code_(code) {
        auto result = fmt::format_to_n(message_buf_. data(), 
                                        message_buf_.size() - 1, 
                                        fmt, 
                                        std::forward<Args>(args)...);
        *result.out = '\0';
        message_len_ = result.size;
    }

    int32_t code_;

    // 方法1 使用
    std:: string message_;

    // 方法2 使用（固定缓冲区，无堆分配）
    std::array<char, MAX_MSG_LEN> message_buf_;
    size_t message_len_ = 0;
};

template <typename... Args>
void UtpLogV(int32_t level, const char *fileName, const char *funcName, int32_t line, fmt::format_string<Args...> format_str, Args&&... args)
{
    fmt::basic_memory_buffer<char, 1024> buffer;
    buffer.clear();

    try {
        fmt::format_to(std::back_inserter(buffer), "[{}:{}:{}()] -> ", fileName, line, funcName);
        fmt::format_to(std::back_inserter(buffer), format_str, std::forward<Args>(args)...);
    } catch (const fmt::format_error& e) {
        buffer.clear();
        printf("[format error]: %s\n", e.what());
    } catch (... ) {
        printf("[format error]\n");
    }

    // 使用长度限制输出，防止缓冲区未以 '\0' 结束导致越界
    printf("%.*s\n", static_cast<int>(buffer.size()), buffer.data());
}

int main(void) {
    Error err1(404, "Resource not found: {}", "/api/data");
    Error err2(500, "Internal server error: code {}", 12345);

    fmt::print("Error 1: code={}, message={}\n", err1.code_, err1.message_buf_.data());
    fmt::print("Error 2: code={}, message={}\n", err2.code_, err2.message_buf_.data());

    fmt::print("\n-------------------------------------------\n");
    UtpLogV(1, "test.cc", "main", 42, "This is a test log with value: {}", 100);
    return 0;
}