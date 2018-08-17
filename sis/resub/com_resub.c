
#include "sis.h"
#include "../include/resub.h"
#include "../include/resub_int.h"

static void resub_usage();

int
com_resub(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int     i, c, method, use_complement;
    array_t *nodevec;

    use_complement = 1;
    method         = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "abd")) != EOF) {
        switch (c) {
            case 'a': method = 0;
                break;
            case 'b': method = 1;
                break;
            case 'd': use_complement = 0;
                break;
            default: resub_usage();
                return 1;
        }
    }

    /* Note!, "resub" is different from "resub *" */
    /* "resub" iterates until no more improvement can be made. */
    if (argc - util_optind == 0) {
        switch (method) {
            case 1: resub_bool_network(*network);
                break;
            case 0: resub_alge_network(*network, use_complement);
                break;
        }
        return 0;
    }

    nodevec = com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);
    for (i  = 0; i < array_n(nodevec); i++) {
        switch (method) {
            case 1: resub_bool_node(array_fetch(node_t * , nodevec, i));
                break;
            case 0: (void) resub_alge_node(array_fetch(node_t * , nodevec, i), use_complement);
                break;
        }
    }
    array_free(nodevec);
    return 0;
}

init_resub() {
    (void) com_add_command("resub", com_resub, 1);
}

end_resub() {
}

static void
resub_usage() {
    (void) fprintf(miserr, "usage: resub [-abd] [node-list]\n");
    (void) fprintf(miserr, "    -a\t\tAlgebraic resubstitution (default).\n");
    (void) fprintf(miserr, "    -b\t\tBoolean resubstitution.\n");
    (void) fprintf(miserr, "    -d\t\tDon't use complement (in algebraic resubstitution).\n");
}
