cmake_minimum_required(VERSION 3.6)
project(fourDimParity)

set(CMAKE_C_STANDARD 11)

set (CMAKE_C_FLAGS "-Werror -Wmain -Wpedantic -Wall -Winit-self -Wextra -Wswitch-default -Wswitch-enum -Wunreachable-code -Wfloat-equal -Wundef -O3 -s")

set(SOURCE_FILES src/main.c src/enc.c src/enc.h)
add_executable(fourDimParity ${SOURCE_FILES})