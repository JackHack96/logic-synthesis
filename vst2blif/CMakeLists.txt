cmake_minimum_required(VERSION 3.8)
project(vst2blif)

set(CMAKE_CXX_STANDARD 11)

add_executable(${PROJECT_NAME} vst2blif.c vst2blif.h)