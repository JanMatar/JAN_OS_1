cmake_minimum_required(VERSION 3.27)
project(JAN_OS_1)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(JAN_OS_1
        Commands.cpp
        Commands.h
        signals.cpp
        signals.h
        smash.cpp)
