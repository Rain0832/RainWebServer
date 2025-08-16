# memory 模块
file(GLOB MEM_SRCS *.cc *.cpp)

add_library(memory_lib ${MEM_SRCS})

target_include_directories(memory_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include/memory
)
