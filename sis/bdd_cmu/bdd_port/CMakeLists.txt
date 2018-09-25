add_library(bdd_port STATIC
        bdditer.c
        bddport.c
        )
target_link_libraries(bdd_port bdd_cmu m)