# net 模块
file(GLOB NET_SRCS *.cc *.cpp)

add_library(net_lib ${NET_SRCS})

target_include_directories(net_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include/net
)

target_link_libraries(net_lib PUBLIC log_lib pthread)