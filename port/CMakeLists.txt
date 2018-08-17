set(project port)
set(${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/regex.h
        ${CMAKE_CURRENT_SOURCE_DIR}/ansi.h
        ${CMAKE_CURRENT_SOURCE_DIR}/port.h
        ${CMAKE_CURRENT_SOURCE_DIR}/copyright.h
        )
add_library(${project} STATIC ${${project}_SOURCES})