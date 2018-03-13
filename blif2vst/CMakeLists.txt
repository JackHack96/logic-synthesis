cmake_minimum_required(VERSION 3.8)
project(blif2vst)

set(CMAKE_CXX_STANDARD 11)

add_executable(${PROJECT_NAME} blif2vst.c blif2vst.h)