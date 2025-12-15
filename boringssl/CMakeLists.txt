cmake_minimum_required(VERSION 3.15)
project(udp_tls13_boringssl_demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 根据你的环境调整 BoringSSL 的头文件与库路径
# 假设安装在 /usr/local/include 与 /usr/local/lib
include_directories(/home/eular/Github/lsquic/boringssl/include)
link_directories(/home/eular/Github/lsquic/boringssl/lib)

add_compile_options(-Wall -Wextra -O2)

add_library(common STATIC
    common/quic_crypto.cpp
    common/boringssl_glue.cpp
    common/udp_socket.cpp
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/common)
target_link_libraries(common
    PUBLIC
    /home/eular/Github/lsquic/boringssl/lib/libssl.a
    /home/eular/Github/lsquic/boringssl/lib/libcrypto.a
)

add_executable(server server.cpp)
target_link_libraries(server common /home/eular/Github/lsquic/boringssl/lib/libssl.a /home/eular/Github/lsquic/boringssl/lib/libcrypto.a pthread)

add_executable(client client.cpp)
target_link_libraries(client common /home/eular/Github/lsquic/boringssl/lib/libssl.a /home/eular/Github/lsquic/boringssl/lib/libcrypto.a pthread)