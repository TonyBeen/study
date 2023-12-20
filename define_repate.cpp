/*************************************************************************
    > File Name: main.cpp
    > Author: hsz
    > Brief:
    > Created Time: Tue 13 Sep 2022 09:18:41 AM CST
 ************************************************************************/

#include <iostream>
using namespace std;

#define comac_get_args_cnt(...) comac_arg_n(__VA_ARGS__)
#define comac_arg_n( _0,_1,_2,_3,_4,_5,_6,_7, N, ...) N
#define comac_args_seqs() 7, 6, 5, 4, 3, 2, 1, 0
#define comac_join_1( x,y ) x##y

#define comac_argc( ... ) comac_get_args_cnt(0, ##__VA_ARGS__, comac_args_seqs())
#define comac_join( x,y ) comac_join_1( x,y )

#define repeat_0(fun, a, ...) 
#define repeat_1(fun, a, ...) fun(1, a, __VA_ARGS__) repeat_0(fun, __VA_ARGS__)
#define repeat_2(fun, a, ...) fun(2, a, __VA_ARGS__) repeat_1(fun, __VA_ARGS__)
#define repeat_3(fun, a, ...) fun(3, a, __VA_ARGS__) repeat_2(fun, __VA_ARGS__)
#define repeat_4(fun, a, ...) fun(4, a, __VA_ARGS__) repeat_3(fun, __VA_ARGS__)
#define repeat_5(fun, a, ...) fun(5, a, __VA_ARGS__) repeat_4(fun, __VA_ARGS__)
#define repeat_6(fun, a, ...) fun(6, a, __VA_ARGS__) repeat_5(fun, __VA_ARGS__)

#define repeat(n, fun, ...) comac_join(repeat_, n)(fun, __VA_ARGS__)

#if __cplusplus <= 199711L
#define decl_typeof( i,a,... ) typedef typeof( a ) typeof_##a;
#else
#define decl_typeof( i,a,... ) typedef decltype( a ) typeof_##a;
#endif
#define impl_typeof_ref( i,a,... ) typeof_##a &a;
#define impl_typeof_cpy( i,a,... ) typeof_##a a;
#define con_param_typeof( i,a,... ) typeof_##a & a##_ref,
#define param_init_typeof( i,a,... ) a(a##_ref),


#define co_ref( name,... ) \
repeat( comac_argc(__VA_ARGS__) ,decl_typeof,__VA_ARGS__ ) \
class type_##name \
{ \
public: \
    repeat( comac_argc(__VA_ARGS__) ,impl_typeof_ref,__VA_ARGS__ ) \
    int _member_cnt; \
    type_##name( \
        repeat( comac_argc(__VA_ARGS__),con_param_typeof,__VA_ARGS__ ) ... ): \
        repeat( comac_argc(__VA_ARGS__),param_init_typeof,__VA_ARGS__ ) _member_cnt(comac_argc(__VA_ARGS__)) \
    {} \
} name( __VA_ARGS__ )


int main(int argc, char **argv)
{
    int total = 100;
    co_ref(ref, total);

    cout << ref.total << endl;
    cout << ref._member_cnt << endl;
    cout << comac_argc("Hello World") << endl;
    cout << comac_get_args_cnt(0, comac_args_seqs()) << endl;
  //comac_arg_n(_0,_1,_2,_3,_4,_5,_6,_7, N,__VA_ARGS__...);
    comac_arg_n( 0, 1, 2, 3, 7, 6, 5, 4, 3, 2, 1, 0 );
    comac_get_args_cnt(0,"Hello World", comac_args_seqs());
    return 0;
}
