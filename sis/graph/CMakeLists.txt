set(CMAKE_C_FLAGS -DSIS)
add_library(graph STATIC
        com_graph.c
        graph.c
        graph.doc
        graph_dfs.c
        graph_s.c
        )
