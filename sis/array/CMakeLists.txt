# Generating the library libarray.a
set(project "array")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/array.c
        )
add_library ( array STATIC ${${project}_SOURCES} )