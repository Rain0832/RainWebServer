# util module
file(GLOB UTIL_SRCS *.cc *.cpp)

add_library(util_lib ${UTIL_SRCS})

target_include_directories(util_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include/util
)
