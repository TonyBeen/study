# cmake的最低版本要求
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

# 工程名称和版本
project(getinfo VERSION 1.0.1)

# 指定的C++编译器版本是必须的
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 指定为C++11 版本
set(CMAKE_CXX_STANDARD 11)

# 给CMAKE_MODULE_PATH添加路径, find_package从CMAKE_MODULE_PATH中找Find<PackageName>.cmake
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /usr/local/lib/cmake/)

message(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")
message(STATUS "PROJECT_SOURCE_DIR = ${PROJECT_SOURCE_DIR}")
message(STATUS "PROJECT_BINARY_DIR = ${PROJECT_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")

message(STATUS "CMAKE_MODULE_PATH = ${CMAKE_MODULE_PATH}")
message(STATUS "CMAKE_CURRENT_LIST_DIR = ${CMAKE_CURRENT_LIST_DIR}")

# 查找OpenSSL包
find_package(OpenSSL REQUIRED)
message(STATUS "OpenSSL:")
message(STATUS "  version: ${OPENSSL_VERSION}")
message(STATUS "  definition: ${OPENSSL_DEFINITIONS}")
message(STATUS "  librarie path: ${OPENSSL_LIBRARY}")
message(STATUS "  libraries: ${OPENSSL_LIBRARIES}")
message(STATUS "  include path: ${OPENSSL_INCLUDE_DIR}")

# 查找jsoncpp包
find_package(JsonCpp REQUIRED)
message(STATUS "JsonCpp:")
message(STATUS "  version: ${jsoncpp_VERSION}")
message(STATUS "  libraries: ${jsoncpp_LIBS}")
message(STATUS "  include path: ${jsoncpp_INCLUDE_DIRS}")

# 查找OpenCV包
find_package(OpenCV REQUIRED)
message(STATUS "OpenCV:")
message(STATUS "  version: ${OpenCV_VERSION}")
message(STATUS "  libraries: ${OpenCV_LIBS}")
message(STATUS "  include path: ${OpenCV_INCLUDE_DIRS}")

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