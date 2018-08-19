cmake_minimum_required(VERSION 3.5)
project(sis)

set(CMAKE_C_STANDARD 90)

include_directories(sis/include)

add_subdirectory(blif2vst)
add_subdirectory(vst2blif)
add_subdirectory(espresso)
add_subdirectory(jedi)
add_subdirectory(nova)
add_subdirectory(options)
add_subdirectory(sis)
add_subdirectory(sred)
add_subdirectory(stamina)
add_subdirectory(utility)