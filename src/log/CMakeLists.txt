# log 模块
file(GLOB LOG_SRCS *.cc *.cpp)

add_library(log_lib ${LOG_SRCS})

target_include_directories(log_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include/log
)
