/*************************************************************************
    > File Name: std_hash.cc
    > Author: hsz
    > Brief: 自定义结构体支持标准库hash
    > Created Time: 2024年04月14日 星期日 12时55分54秒
 ************************************************************************/

#include <stdint.h>
#include <unordered_map>

struct Info
{
    uint8_t one;
    int32_t two;
    float   three;

    bool operator==(const Info& other) const {
        return (two == other.two);
    }
};

template<>
struct std::hash<Info> : public std::__hash_base<size_t, Info>
{
    size_t operator()(Info __val) const noexcept
    {
        return static_cast<size_t>(__val.two);
    }
};

int main(int argc, char **argv)
{
    std::unordered_map<Info, uint32_t> something;
    
    Info info = {0, 2, 3.14};
    something[info] = 100;
    return 0;
}
