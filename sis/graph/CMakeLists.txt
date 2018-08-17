# Generating the library libgraph.a
set(project "graph")

set(${project}, "")
list(APPEND ${project}_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/com_graph.c
        ${CMAKE_CURRENT_SOURCE_DIR}/graph.c
        ${CMAKE_CURRENT_SOURCE_DIR}/graph.doc
        ${CMAKE_CURRENT_SOURCE_DIR}/graph_dfs.c
        ${CMAKE_CURRENT_SOURCE_DIR}/graph_int.h
        ${CMAKE_CURRENT_SOURCE_DIR}/graph_s.c
        ${CMAKE_CURRENT_SOURCE_DIR}/graph_static_int.h
        )
add_library ( graph STATIC ${${project}_SOURCES} )