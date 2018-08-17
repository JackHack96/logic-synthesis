# Generating the library liblinsolv.a
set(project "linsolv")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/spAllocate.c
        ${CMAKE_CURRENT_SOURCE_DIR}/spBuild.c
        ${CMAKE_CURRENT_SOURCE_DIR}/spDefs.h
        ${CMAKE_CURRENT_SOURCE_DIR}/spFactor.c
        ${CMAKE_CURRENT_SOURCE_DIR}/spOutput.c
        ${CMAKE_CURRENT_SOURCE_DIR}/spSolve.c
        ${CMAKE_CURRENT_SOURCE_DIR}/spUtils.c
        )
add_library(linsolv STATIC ${${project}_SOURCES})