add_library(libntbdd STATIC
        bdd_at_node.c
        bdd_to_ntwk.c
        com_ntbdd.c
        manager.c

        node_to_bdd.c
        verify_ntwk.c
        ../include/ntbdd_int.h
        )