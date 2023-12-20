/*************************************************************************
    > File Name: cxxabi_stacktrace.cc
    > Author: hsz
    > Brief: 编译时需要加上 -rdynamic
    > Created Time: Sun 10 Dec 2023 03:28:06 PM CST
 ************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <string>
#include <vector>
#include <sstream>

namespace detail {

uint64_t string2hex(const std::string &hexString, bool &success)
{
    uint64_t value = 0;
    std::stringstream ss;
    ss << std::hex << hexString;
    ss >> value;
    success = !ss.fail();

    return value;
}

static const uint32_t MAX_SYMBOL_SIZE = 256;
static const uint32_t MAX_ADDRESS_SIZE = 32;
static const uint32_t MAX_OFFSET_SIZE = 16;

static bool demangle(const char *str, char *strSymbol, char *strAddress, char *strOffset)
{
    const char *beginName = nullptr;
    const char *endName = nullptr;
    const char *beginOffset = nullptr;
    const char *endOffset = nullptr;
    const char *beginAddress = nullptr;
    const char *endAddress = nullptr;
    char unresolvedSymbols[MAX_SYMBOL_SIZE] = {0};
    size_t symbolSize = MAX_SYMBOL_SIZE;

	// https://panthema.net/2008/0901-stacktrace-demangled/
	// str内容类似: ./test(_ZN6ClassA9functionAEv+0x32) [0x55c8e0f5cab0]
    // abi::__cxa_demangle解析的是_ZN6ClassA9functionAEv
    for (const char *p = str; *p; ++p)
    {
        switch (*p) {
        case '(':
            beginName = p + 1;
            break;
        case '+':
            endName = p;
            beginOffset = p + 1;
            break;
        case ')':
            endOffset = p;
            break;
        case '[':
            beginAddress = p + 1;
            break;
        case ']':
            endAddress = p;
            break;
        default:
            break;
        }
    }

    // 无效的符号字符串
    if (beginName >= endName || beginOffset >= endOffset || beginAddress >= endAddress)
    {
        return false;
    }

    strncpy(unresolvedSymbols, beginName, endName - beginName);
    strncpy(strAddress, beginAddress, endAddress - beginAddress);
    strncpy(strOffset, beginOffset, endOffset - beginOffset);

    int32_t status = 0;
    abi::__cxa_demangle(unresolvedSymbols, strSymbol, &symbolSize, &status);
    if (0 != status)
    {
        // 解析失败, c函数会解析失败
        strcpy(strSymbol, unresolvedSymbols);
    }

    return true;
}

/**
 * @brief 获取堆栈信息
 * 
 * @param ignoreBegin 忽略堆栈开始层数
 * @param ignoreEnd 忽略堆栈结束层数
 * @return std::vector<std::string> 
 */
std::vector<std::string> stacktrace(uint16_t ignoreBegin = 1, uint16_t ignoreEnd = 2)
{
    std::vector<std::string> stackVec;
    // 最大栈回溯层数
    static const uint32_t MAX_FRAMES = 64;
    void *pFrames[MAX_FRAMES];
    int32_t nFrames = ::backtrace(pFrames, MAX_FRAMES);
    char **frameStrings = ::backtrace_symbols(pFrames, nFrames);
    if (nullptr == frameStrings) {
        return stackVec;
    }

    for(int32_t i = ignoreBegin; i < (nFrames - ignoreEnd); ++i) {
        char strSymbol[MAX_SYMBOL_SIZE] = {0};
        char strAddress[MAX_ADDRESS_SIZE] = {0};
        char strOffset[MAX_OFFSET_SIZE] = {0};

        if (demangle(frameStrings[i], strSymbol, strAddress, strOffset))
        {
            bool success = false;
            uint64_t address = string2hex(strAddress, success);
            uint64_t offset = string2hex(strOffset, success);

            static const uint32_t BUFFER_SIZE = 512;
            char symbol[BUFFER_SIZE] = {0};
            snprintf(symbol, BUFFER_SIZE, "-0x%012lx: (%s + 0x%lx)", address, strSymbol, offset);

            stackVec.push_back(symbol);
        }
        else
        {
            stackVec.push_back(frameStrings[i]);
        }
    }

    free(frameStrings);
    return stackVec;
}
} // namespace detail

