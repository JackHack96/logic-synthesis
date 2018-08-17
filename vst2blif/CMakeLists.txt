set(project "vst2blif")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/vst2blif.c
        )
add_executable(vst2blif ${${project}_SOURCES})