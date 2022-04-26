cmake_minimum_required(VERSION 3.22)
project(shell)

set(CMAKE_CXX_STANDARD 14)

add_executable(shell
        shell.cpp main.h main.cpp shell.h error.cpp error.h)
