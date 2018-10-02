
#include "sis.h"

static int com_ripup(network_t **network, int argc, char **argv) {
    array_t   *node_vec;
    network_t *new_network;

    node_vec    = com_get_nodes(*network, argc, argv);
    new_network = network_from_nodevec(node_vec);
    network_free(*network);
    *network = new_network;
    return 0;
}

static int com_collapse(network_t **network, int argc, char **argv) {
    array_t *node_vec;
    node_t  *node1, *node2;

    if (argc == 1) {
        (void) network_collapse(*network);

    } else {
        node_vec = com_get_nodes(*network, argc, argv);
        if (array_n(node_vec) == 1) {
            node1 = array_fetch(node_t *, node_vec, 0);
            (void) network_collapse_single(node1);

        } else if (array_n(node_vec) == 2) {
            node1 = array_fetch(node_t *, node_vec, 0);
            node2 = array_fetch(node_t *, node_vec, 1);
            (void) node_collapse(node1, node2);

        } else {
            (void) fprintf(miserr, "usage: clp [n1] [n2]\n");
            return 1;
        }
        array_free(node_vec);
    }
    return 0;
}

/* ARGSUSED */
static int com_sweep(network_t **network, int argc, char **argv) {
    if (argc != 1) {
        (void) fprintf(miserr, "usage: sweep\n");
        return 1;
    }
    (void) network_sweep(*network);
    return 0;
}

/* ARGSUSED */
static int com_espresso(network_t **network, int argc, char **argv) {
    network_t *new_net;

    if (argc != 1) {
        (void) fprintf(miserr, "usage: espresso\n");
        return 1;
    }
    new_net = network_espresso(*network);
    if (new_net == 0) { /* well, happens for some weird cases */
        return 0;
    } else {
        network_free(*network);
        *network = new_net;
    }
    return 0;
}

static int com_check(network_t **network, int argc, char **argv) {
    int c, verbose, is_okay;

    verbose = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "v")) != EOF) {
        switch (c) {
            case 'v':verbose = 1;
                break;
            default:goto usage;
        }
    }

    if (argc != util_optind)
        goto usage;

    error_init();
    is_okay = network_check(*network) && network_is_acyclic(*network);
    if (is_okay) {
        if (verbose) {
            (void) fprintf(misout, "check: network passes consistency check\n");
        }
        return 0;
    } else {
        (void) fprintf(miserr, "check: problem detected with network\n");
        (void) fprintf(miserr, "%s", error_string());
        return 1;
    }

    usage:
    (void) fprintf(miserr, "usage: _check [-v]\n");
    return 1;
}

/* ARGSUSED */
static int com_verify(network_t **network, int argc, char **argv) {
    int       c, verbose, eql, method;
    network_t *network1, *network2;
    char      cmd[1024];

    verbose = 0;

#ifdef SIS
    if (network_num_latch(*network) != 0) {
        (void) fprintf(
                siserr, "Use the verify_fsm command to verify sequential circuits.\n");
        return 1;
    }
#endif /* SIS */

    util_getopt_reset();
    method    = 0;
    while ((c = util_getopt(argc, argv, "m:v")) != EOF) {
        switch (c) {
            case 'm':
                if (strcmp(util_optarg, "clp") == 0) {
                    method = 0;
                } else if (strcmp(util_optarg, "bdd") == 0) {
                    method = 1;
                } else if (strcmp(util_optarg, "par") == 0) {
                    method = 2;
                } else {
                    goto usage;
                }
                break;
            case 'v':verbose = 1;
                break;
            default:goto usage;
        }
    }

    if (argc - util_optind == 0) {
        network1 = *network;
        network2 = (*network)->original;
        if (network2 == 0) {
            (void) fprintf(miserr, "error -- no original network\n");
            return 1;
        }
        eql = net_verify_with_dc(network1, network2, method, verbose);
        return (eql);
    } else if (argc - util_optind == 1) {
        network1 = *network;
        network2 = network_alloc();
        (void) sprintf(cmd, "read_blif %s", argv[util_optind]);
        if (com_execute(&network2, cmd) != 0)
            return 1;
        eql = net_verify_with_dc(network1, network2, method, verbose);
        network_free(network2);
        return (eql);

    } else if (argc - util_optind == 2) {
        network1 = network_alloc();
        (void) sprintf(cmd, "read_blif %s", argv[argc - 2]);
        if (com_execute(&network1, cmd) != 0)
            return 1;
        network2 = network_alloc();
        (void) sprintf(cmd, "read_blif %s", argv[argc - 1]);
        if (com_execute(&network2, cmd) != 0)
            return 1;
        eql = net_verify_with_dc(network1, network2, method, verbose);
        network_free(network1);
        network_free(network2);
        return (eql);

    } else {
        goto usage;
    }

    usage:
    (void) fprintf(miserr, "usage: verify [-m] [[net1.blif] [net2.blif]]\n");
    (void) fprintf(miserr, "    -m \tclp\tVerifying by collapsing (default)\n");
    (void) fprintf(miserr, "    -m \tbdd\tVerifying using bdd\n");
    return 1;
}

init_network() {
    com_add_command("_check", com_check, 0);
    com_add_command("_ripup", com_ripup, 1);
    com_add_command("collapse", com_collapse, 1);
    com_add_command("espresso", com_espresso, 1);
    com_add_command("sweep", com_sweep, 1);
    com_add_command("verify", com_verify, 0);
}

end_network() {}
