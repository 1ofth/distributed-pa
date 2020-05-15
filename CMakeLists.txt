cmake_minimum_required(VERSION 3.13)
project(distributedomputing_pa1 C)

set(CMAKE_C_STANDARD 99)
add_compile_options(-Wall -pedantic)
add_executable(distributedomputing_pa1 child.c child.h main.c ipc.h common.h distributed.h ipc.c logger.h
        pa2345.h logger.c distributed.c MinPQ.h MinPQ.c cs_manager.c cs_manager.h )

find_library(libruntime libruntime.so paths ${PROJECT_SOURCE_DIR}/lib64/)
target_link_libraries(distributedomputing_pa1 ${libruntime})