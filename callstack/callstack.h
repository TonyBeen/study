/*************************************************************************
    > File Name: callstaack.h
    > Author: hsz
    > Mail:
    > Created Time: Tue 27 Jul 2021 06:02:23 PM CST
 ************************************************************************/

#ifndef __ALIAS_CALLSTACK_H__
#define __ALIAS_CALLSTACK_H__

#include <stdio.h>
#include <string>
#include <vector>

namespace utils {

class CallStack {
public:
    CallStack();
    CallStack(uint16_t ignoreBegin);
    ~CallStack() = default;

    /**
     * @brief dump the stack of the current call.
     * 
     * @param ignoreBegin 可忽略的起始调用函数层级
     * @param ignoreEnd 可忽略的最后调用函数层级
     */
    void update(uint16_t ignoreBegin = 2, uint16_t ignoreEnd = 2);

    /**
     * @brief 获取堆栈信息
     * 
     * @return const std::vector<std::string>& 
     */
    const std::vector<std::string> &frames() const { return m_stackFrame; }

    /**
     * @brief 堆栈信息转出字符串
     * 
     * @return std::string 
     */
    std::string to_string() const;

    /**
     * @brief 清空堆栈信息
     * 
     */
    inline void clear() { m_stackFrame.clear(); }

    /**
     * @brief 获取堆栈层数
     * 
     * @return uint32_t 
     */
    inline uint32_t size() const { return static_cast<uint32_t>(m_stackFrame.size()); }

private:
    std::vector<std::string> m_stackFrame;
    uint16_t                 m_skip;
    uint16_t                 m_skipEnd;
};

} // namespace eular

#endif // __ALIAS_CALLSTACK_H__