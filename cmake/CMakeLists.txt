# cmake的最低版本要求
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# 工程名称和版本 默认 c/c++ 工程 不需要指定language
project(getinfo VERSION 1.0.1) # 可以省略 LANGUAGES C CXX

# 指定c版本为c99
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# 指定为C++11 版本
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 指定的C++编译器版本是必须的

# 给 CMAKE_MODULE_PATH 添加路径, find_package 从 CMAKE_MODULE_PATH 中找 Find<PackageName>.cmake
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH "/usr/local/lib/cmake/")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 设置 CMAKE_PREFIX_PATH find_package 从 CMAKE_PREFIX_PATH 中找 <PackageName>Config.cmake
list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/cmake/")
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

message(STATUS "CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_PREFIX_PATH = ${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_ROOT = ${CMAKE_ROOT}")

# 通过找到文件的所在位置, 将头文件路径赋值给libhv_INCLUDE_DIRS
# find_path(libhv_INCLUDE_DIRS hv/hv.h)
# message("----> libhv_INCLUDE_DIRS: " ${libhv_INCLUDE_DIRS})

find_package(event_wrapper REQUIRED)
if(${event_wrapper_FOUND})
	if (event_wrapper_VERSION VERSION_LESS "1.0.0")
		message(STATUS "event_wrapper version is too old")
	endif()
	if(TARGET event_wrapper::event_wrapper)
		message(STATUS "Found event_wrapper::event_wrapper target")
	else()
		message(STATUS "event_wrapper::event_wrapper target not found")
	endif()

	if(TARGET event_wrapper::event_wrapper_static)
		message(STATUS "Found event_wrapper::event_wrapper_static target")
	else()
		message(STATUS "event_wrapper::event_wrapper_static target not found")
	endif()

	message(STATUS "event_wrapper found:")
	message(STATUS "  version: ${event_wrapper_VERSION}")
	message(STATUS "  libraries: ${event_wrapper_LIBRARIES}")
	message(STATUS "  include path: ${event_wrapper_INCLUDE_DIRS}")
endif()

find_package(libhv REQUIRED)
message(STATUS "")
message(STATUS "libhv:")
message(STATUS "  version: ${libhv_VERSION}")
message(STATUS "  libraries: ${libhv_LIBRARIES}")
message(STATUS "  include path: ${libhv_INCLUDE_DIRS}")

find_package(Libevent REQUIRED)
message(STATUS "")
message(STATUS "libevent:")
message(STATUS "  version: ${libevent_VERSION}")
message(STATUS "  libraries: ${LIBEVENT_STATIC_LIBRARIES}")
message(STATUS "  include path: ${LIBEVENT_INCLUDE_DIRS}")

message(STATUS "")
message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")
message(STATUS "PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
message(STATUS "PROJECT_BINARY_DIR = ${PROJECT_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_LIST_DIR = ${CMAKE_CURRENT_LIST_DIR}")

# 查找OpenSSL包
find_package(OpenSSL REQUIRED)
message(STATUS "")
message(STATUS "OpenSSL:")
message(STATUS "  version: ${OpenSSL_VERSION}")
message(STATUS "  definition: ${OpenSSL_DEFINITIONS}")
message(STATUS "  librarie path: ${OpenSSL_LIBRARY}")
message(STATUS "  libraries: ${OpenSSL_LIBRARIES}")
message(STATUS "  include path: ${OpenSSL_INCLUDE_DIR}")
# openssl 不支持cmake, cmake默认支持查找openssl, 链接OpenSSL::SSL和OpenSSL::Crypto
if(${OpenSSL_FOUND})
if(TARGET OpenSSL::SSL)
    message(STATUS "Found OpenSSL::SSL target")
else()
    message(STATUS "OpenSSL::SSL target not found")
endif()
if(TARGET OpenSSL::Crypto)
    message(STATUS "Found OpenSSL::Crypto target")
else()
    message(STATUS "OpenSSL::Crypto target not found")
endif()
endif()

# 查找jsoncpp包
message(STATUS "")
find_package(JsonCpp REQUIRED)
message(STATUS "JsonCpp: ${JSONCPP_FOUND} ${JsonCpp_FOUND}")
message(STATUS "  version: ${JsonCpp_VERSION_STRING}")
message(STATUS "  libraries: ${JsonCpp_LIBRARIES}")
message(STATUS "  include path: ${JsonCpp_INCLUDE_DIRS}")

# 查找OpenCV包 QUIET表示找不到时也不输出警告信息
message(STATUS "")
find_package(OpenCV QUIET)
if(${OpenCV_FOUND})
	message(STATUS "OpenCV:")
	message(STATUS "  version: ${OpenCV_VERSION}")
	message(STATUS "  libraries: ${OpenCV_LIBRARIES}")
	message(STATUS "  include path: ${OpenCV_INCLUDE_DIRS}")
endif()

find_package(log2 QUIET)
if(${log2_FOUND})
	message(STATUS "")
	message(STATUS "log2:")
	message(STATUS "  version: ${log2_VERSION}")
	message(STATUS "  include path: ${LIBLOG2_INCLUDE_DIRS}")
	if (${log2_STATIC_LIB} MATCHES "ON")
		message(STATUS "  static library: ${LIBLOG2_STATIC_LIBRARIES}")
	endif()
	if (${log2_DYNAMIC_LIB} MATCHES "ON")
		message(STATUS "  dynamic library: ${LIBLOG2_SHARED_LIBRARIES}")
	endif()
endif()

message(STATUS "is the C++ compiler loaded? ${CMAKE_CXX_COMPILER_LOADED}")
if(CMAKE_CXX_COMPILER_LOADED)
	message(STATUS "-> C++ compiler id: ${CMAKE_CXX_COMPILER_ID}")
	if(CMAKE_COMPILER_IS_GNUCXX)
		message("->CMAKE_COMPILER_IS_GNUCXX")
	else()
		message("->CMAKE_COMPILER_IS_NOT_GNUCXX")
	endif()	
	message(STATUS "C++ compiler version is: ${CMAKE_CXX_COMPILER_VERSION}")
endif()
 
message(STATUS "is the C compiler loaded? ${CMAKE_C_COMPILER_LOADED}")
if(CMAKE_C_COMPILER_LOADED)
	message(STATUS "->C compiler id: ${CMAKE_C_COMPILER_ID}")
	if(CMAKE_COMPILER_IS_GNUCC)
		message("->CMAKE_COMPILER_IS_GNUCC")
	else()
		message("->CMAKE_COMPILER_IS_NOT_GNUCC")
	endif()	
	message(STATUS "C compiler version is: ${CMAKE_C_COMPILER_VERSION}")
endif()