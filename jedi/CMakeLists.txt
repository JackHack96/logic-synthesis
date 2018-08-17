project(jedi)
set(project "jedi")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/cluster.c
        ${CMAKE_CURRENT_SOURCE_DIR}/expand.c
        ${CMAKE_CURRENT_SOURCE_DIR}/getopt.c
        ${CMAKE_CURRENT_SOURCE_DIR}/hill.c
        ${CMAKE_CURRENT_SOURCE_DIR}/hot.c
        ${CMAKE_CURRENT_SOURCE_DIR}/jedi.c
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jedi.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/jedi_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/options.c
        ${CMAKE_CURRENT_SOURCE_DIR}/random.c
        ${CMAKE_CURRENT_SOURCE_DIR}/read.c
        ${CMAKE_CURRENT_SOURCE_DIR}/rp.c
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/rp.h
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/rp_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/util.c
        ${CMAKE_CURRENT_SOURCE_DIR}/inc/util.h
        ${CMAKE_CURRENT_SOURCE_DIR}/weights.c
        ${CMAKE_CURRENT_SOURCE_DIR}/write.c
        )
add_executable(jedi ${${project}_SOURCES})
target_link_libraries(jedi ${CMAKE_SOURCE_DIR}/bin/utility/libutility.a)
target_link_libraries(jedi m)