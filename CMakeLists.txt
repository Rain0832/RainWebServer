cmake_minimum_required(VERSION 3.10)
project(RainWebServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

# 设置输出目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/log
    ${PROJECT_SOURCE_DIR}/include/memory
    ${PROJECT_SOURCE_DIR}/include/net
    ${PROJECT_SOURCE_DIR}/include/util
)

# 添加子目录
add_subdirectory(src/log)
add_subdirectory(src/memory)
add_subdirectory(src/net)
add_subdirectory(src/util)

# main 程序
add_executable(main src/main.cc)
target_link_libraries(main PRIVATE log_lib memory_lib net_lib util_lib pthread)