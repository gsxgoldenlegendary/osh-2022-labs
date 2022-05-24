cmake_minimum_required(VERSION 3.16)
project(lab3 C)

set(CMAKE_C_STANDARD 11)

add_executable(1
        1.c)
target_link_libraries(1 pthread)
#add_executable(0
#        0.c)
#target_link_libraries(0 pthread)
#
add_executable(2
        2.c)
target_link_libraries(2 pthread)
add_executable(3
        3.c)
