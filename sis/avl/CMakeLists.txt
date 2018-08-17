# Generating the library libavl.a
set(project "avl")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/avl.c
        )
add_library ( avl STATIC ${${project}_SOURCES} )