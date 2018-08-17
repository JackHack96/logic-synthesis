# Generating the library liblatch.a
set(project "latch")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/com_latch.c
        ${CMAKE_CURRENT_SOURCE_DIR}/latch.c
        )
add_library ( latch STATIC ${${project}_SOURCES} )