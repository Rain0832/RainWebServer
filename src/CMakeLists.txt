file(GLOB SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/*cc)
list(REMOVE_ITEM SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR}/main.cc)

# 创建共享库
add_library(src_lib SHARED ${SRC_FILE})

#创建可执行文件
add_executable(main  main.cc)

#链接必要的库
target_link_libraries(main src_lib memory_lib log_lib ${LIBS})