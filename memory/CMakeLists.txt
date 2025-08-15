cmake_minimum_required(VERSION 3.10)

project(memory_lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找源文件
file(GLOB MEMORY_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# 创建库（静态库或共享库）
add_library(memory_lib SHARED ${MEMORY_SOURCES})

# 添加头文件目录
target_include_directories(memory_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)