/*************************************************************************
    > File Name: callstack.cpp
    > Author: hsz
    > Mail:
    > Created Time: Tue 27 Jul 2021 06:02:27 PM CST
 ************************************************************************/

#if defined(WIN64) || defined(_WIN64)
    #define OS_WIN64
    #define OS_WIN32
#elif defined(WIN32)|| defined(_WIN32)
    #define OS_WIN32
#elif defined(linux) || defined(__linux) || defined(__linux__)
    #define OS_LINUX
#elif defined(__APPLE__)
    #define OS_MAC
#else
    #error "unsupported system platform!"
#endif

#include "callstack.h"
#include <stdlib.h>

#if defined(OS_LINUX) || defined(OS_MAC)
#include "cxxabi_stacktrace.hpp"
#elif defined(OS_WIN32) || defined(OS_WIN64)
#include "dbghelp_stacktrace.hpp"
#endif

namespace utils {

CallStack::CallStack() :
    m_skip(2),
    m_skipEnd(0)
{
}

CallStack::CallStack(uint16_t ignoreBegin)
{
    this->update(ignoreBegin);
}

void CallStack::update(uint16_t ignoreBegin, uint16_t ignoreEnd)
{
    m_skip = ignoreBegin;
    m_skipEnd = ignoreEnd;
#if defined(OS_LINUX) || defined(OS_MAC)
    m_stackFrame = detail::stacktrace(m_skip, m_skipEnd);
#elif defined(OS_WIN32) || defined(OS_WIN64)
    detail::DbgStackWalker stack;
    stack.ShowCallstack();
    m_skipEnd += 4; // Windows需要额外跳过4层
    int32_t size = stack.m_stackFrameVec.size();
    for (int32_t i = m_skip; i < (size - m_skipEnd); ++i)
    {
        m_stackFrame.push_back(stack.m_stackFrameVec.at(i));
    }
#endif
}

std::string CallStack::to_string() const
{
    std::string str;
    for (size_t i = 0; i < m_stackFrame.size(); ++i) {
        str.append(m_stackFrame[i]);
        str.push_back('\n');
    }

    return str;
}

} // namespace eular