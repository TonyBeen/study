/*************************************************************************
    > File Name: type_list.cc
    > Author: hsz
    > Brief: g++ type_list.cc -std=c++11
    > Created Time: 2024年03月17日 星期日 16时22分47秒
 ************************************************************************/

#include <iostream>
using namespace std;

template <typename... Ts>
struct type_list {
    using self_type = type_list<Ts...>;
    static constexpr size_t size = sizeof...(Ts);
};

namespace detail {

template<typename, size_t N>
struct list_element;

template<typename T, typename... Ts, size_t N>
struct list_element<type_list<T, Ts...>, N> : list_element<type_list<Ts...>, N - 1>
{
};

template<typename T, typename... Ts>
struct list_element<type_list<T, Ts...>, 0>
{
    using type = T;
};

template <typename>
struct list_size;

// 获取列表大小
template <template <typename...> typename ListType, typename... Ts>
struct list_size<ListType<Ts...>>
{
    static constexpr size_t value = sizeof...(Ts);
};

// 空的 type_list
template <typename TypeList1, typename TypeList2>
struct is_type_list_same {
    static constexpr bool value = std::is_same<TypeList1, TypeList2>::value;
};

// 非空的 type_list
template <typename T, typename... Ts, typename T2, typename... Ts2>
struct is_type_list_same<type_list<T, Ts...>, type_list<T2, Ts2...>> {
    static constexpr bool value = is_type_list_same<type_list<Ts...>, type_list<Ts2...>>::value;
};

} // namespace detail

template<typename TypeList, size_t N>
using list_element_t = typename detail::list_element<TypeList, N>::type;

#if __cplusplus >= 201703L
template <typename TypeList>
constexpr size_t list_size_v = detail::list_size<TypeList>::value;

template <typename TypeList1, typename TypeList2>
constexpr bool is_type_list_same_v = detail::is_type_list_same<TypeList1, TypeList2>::value;
#endif

int main(int argc, char **argv)
{
    using type_list_t = type_list<int, double, char>;
    using type_list_t2 = type_list<int, double, char>;
    static_assert(detail::is_type_list_same<type_list_t, type_list_t2>::value, "error");

    static_assert(std::is_same<list_element_t<type_list_t, 0>, int32_t>::value, "error");
    static_assert(std::is_same<list_element_t<type_list_t, 1>, double>::value, "error");

    static_assert(3 == detail::list_size<type_list_t>::value, "error");

    using type_list_void_1 = type_list<>;
    using type_list_void_2 = type_list<>;

    static_assert(detail::is_type_list_same<type_list_void_1, type_list_void_2>::value, "error");

    return 0;
}
