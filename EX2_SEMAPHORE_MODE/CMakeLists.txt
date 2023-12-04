cmake_minimum_required(VERSION 3.26)
project(EX2_SEMAPHORE_MODE)

set(CMAKE_CXX_STANDARD 20)

add_executable(parent
        parent.c)
add_executable(child_process
        child_process.c)

target_link_libraries(parent rt)
target_link_libraries(child_process rt)
