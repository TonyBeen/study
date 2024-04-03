cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

project(test_gzip VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 11)

message("当前源码目录(CMAKE_CURRENT_SOURCE_DIR): ${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/zlib)

# 获取当前目录下所有源文件
file(GLOB SOURCE_FILES *.c *.cpp *.cc)

# 获取当前目录及子目录下所有源文件
# file(GLOB_RECURSE SOURCE_FILES *.c *.cpp *.cc)


message("源代码: ${SOURCE_FILES}")

if(WIN32)
    add_compile_options(/W4 /WX)
elseif(UNIX)
    add_compile_options(-Wall -Wextra)
endif()

set(CMAKE_BUILD_TYPE "Release")

# 添加源文件
add_executable(test_gzip ${SOURCE_FILES})

# 添加头文件目录
target_include_directories(test_gzip PUBLIC 
                           ${CMAKE_CURRENT_SOURCE_DIR}/zlib)

target_link_libraries(test_gzip zlib)

target_link_libraries(test_gzip libutils.so)