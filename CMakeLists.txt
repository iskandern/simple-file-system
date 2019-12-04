cmake_minimum_required(VERSION 3.12)
project(fs C)

set(CMAKE_C_STANDARD 11)

add_executable(fs main.c file_system.c file_system.h)