/*************************************************************************
    > File Name: use_error_code.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月20日 星期五 15时43分07秒
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <system_error>
#include <iostream>
#include <future>

enum class SomethingError {
    None = 0,
    InvalidArgument = 1,
    FileNotFound = 2,
    UnknownError = 3
};

class ErrorCategory : public std::error_category {
public:
    // 返回错误类别的名称
    const char* name() const noexcept override {
        return "ErrorCategory";
    }

    // 返回错误信息字符串
    std::string message(int ev) const override {
        switch (static_cast<SomethingError>(ev)) {
            case SomethingError::None:
                return "No error";
            case SomethingError::InvalidArgument:
                return "Invalid argument";
            case SomethingError::FileNotFound:
                return "File not found";
            case SomethingError::UnknownError:
                return "Unknown error";
            default:
                return "Unrecognized error";
        }
    }
};

// 获取全局单例
const ErrorCategory& GetErrorCategory() {
    static ErrorCategory instance;
    return instance;
}

std::error_code make_error_code(SomethingError e) {
    return {static_cast<int>(e), GetErrorCategory()};
}

namespace std {
    template <>
    struct is_error_code_enum<SomethingError> : true_type {};
}

int main()
{
    // 自定义
    std::error_code ec = make_error_code(SomethingError::FileNotFound);
    if (ec) {
        std::cerr << "Error: " << ec.message() << std::endl; // File not found
    } else {
        std::cout << "No error" << std::endl;
    }

    // 内置
    // system_category
    std::error_code ec1(ENOENT, std::system_category());
    std::cout << "System error message: " << ec1.message() << std::endl; // No such file or directory
    ec1.assign(10000000, std::system_category());
    std::cout << "System error message: " << ec1.message() << std::endl; // Unknown error 10000000

    // 使用 generic_category
    std::error_code ec2 = std::make_error_code(std::errc::invalid_argument);
    std::cout << "Generic error message: " << ec2.message() << std::endl; // Invalid argument

    // 使用 future_category
    std::error_code ec3 = std::make_error_code(std::future_errc::broken_promise);
    std::cout << "Future error message: " << ec3.message() << std::endl; // Broken promise

    return 0;
}