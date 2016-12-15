cmake_minimum_required(VERSION 3.6)
project(fourDimParity)

set(CMAKE_C_STANDARD 11)

set (CMAKE_C_FLAGS "-Werror -Wpedantic -Wall -Winit-self -Wextra")

set(SOURCE_FILES main.c enc.c enc.h)
add_executable(fourDimParity ${SOURCE_FILES})