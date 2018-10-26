set(CMAKE_C_FLAGS -DSIS)
add_library(map STATIC
        addinv.c
        bin_delay.c


        bottom_up.c
        chkform.c
        cluster.c


        com_map.c


        fanout_delay.c


        fanout_dump.c
        fanout_est.c


        fanout_log.c
        fanout_opt.c

        fanout_tree.c

        fanout_util.c
        gate_link.c

        hackit.c

        library.c
        libutil.c
        lt_trees.c

        map_delay.c


        map_interface.c


        maputil.c
        match.c
        mix_lt_trees.c
        multi_array.c

        noalg.c
        prim.c
        pwl.c


        replace.c
        top_down.c
        tree.c
        treemap.c
        treesize.c
        two_level.c
        virtual_del.c
        virtual_net.c
        )
