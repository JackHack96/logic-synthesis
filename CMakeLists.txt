cmake_minimum_required(VERSION 3.8)
project(sis)

set(CMAKE_CXX_STANDARD 11)

include_directories("${PROJECT_SOURCE_DIR}")

add_subdirectory(blif2vst)
add_subdirectory(vst2blif)
add_subdirectory(espresso)
add_subdirectory(jedi)
add_subdirectory(nova)
add_subdirectory(options)
#add_subdirectory(sis)
add_subdirectory(sred)
add_subdirectory(stamina)
add_subdirectory(utility)