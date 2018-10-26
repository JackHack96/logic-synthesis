set(CMAKE_C_FLAGS -DSIS)
add_library(command STATIC
        addcom.c
        alias.c
        cmd_set.c
        command.c
        commisc.c
        filec.c
        get_nodes.c
        get_true_po.c
        help.c
        open_file.c
        source.c
        undo.c
        )