cmake_minimum_required(VERSION 3.6)
project(fourDimParity)

set(CMAKE_C_STANDARD 11)

set (CMAKE_C_FLAGS "-Werror -Wpedantic -Wall -Winit-self -Wextra -O3")

set(SOURCE_FILES src/main.c src/enc.c src/enc.h)
add_executable(fourDimParity ${SOURCE_FILES})