# 设置 cmake 的最低版本和项目名称
cmake_minimum_required(VERSION 3.0)

project(Tiny_WebServer)

# 生成 debug 版本，可以进行 gdb 调试
set(CMAKE_BUILD_TYPE "Debug")

# # 设置调试信息
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -fPIC")

# 设置项目可行性文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 设置项目库文件输出的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib) 

# 设置项目编译头文件搜索路径 -I
include_directories(${PROJECT_SOURCE_DIR}/include) 

# # 设置项目库文件搜索路径 -L
# link_directories(${PROJECT_SOURCE_DIR}/lib)

# 包含这些目录下的.cc文件
aux_source_directory(${PROJECT_SOURCE_DIR}/src/base SRC_BASE)
# aux_source_directory(${PROJECT_SOURCE_DIR}/src/net SRC_NET)
# aux_source_directory(${PROJECT_SOURCE_DIR}/src/net/poller SRC_POLLER)
# aux_source_directory(${PROJECT_SOURCE_DIR}/src/http SRC_HTTP)
# aux_source_directory(${PROJECT_SOURCE_DIR}/src/timer SRC_TIMER)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/log SRC_LOG)
# aux_source_directory(${PROJECT_SOURCE_DIR}/src/memory SRC_MEMORY)
# aux_source_directory(${PROJECT_SOURCE_DIR}/src/mysql SRC_MYSQL)

add_library(Tiny_WebServer  
            ${SRC_BASE}
            # ${SRC_NET}
            # ${SRC_POLLER}
            # ${SRC_TIMER}
            ${SRC_LOG}
            # ${SRC_HTTP}
            # ${SRC_MEMORY}
            # ${SRC_MYSQL}
            )

# src 包含了 Tiny_WebServer 所有的相关代码
add_subdirectory(src)

# example 包含了 Tiny_WebServer 框架是由的示例代码
# add_subdirectory(example)