
#include "phase.h"
#include "phase_int.h"
#include "sis.h"

int com_phase_assign(network_t **network, int argc, char **argv) {
    int c, method;
    int num;

    method = 0;
    util_getopt_reset();
    phase_trace_unset();
    phase_check_unset();
    while ((c = util_getopt(argc, argv, "r:cqgst")) != EOF) {
        switch (c) {
            case 'c':phase_check_set();
                break;
            case 'q':method = 0;
                break;
            case 'g':method = 1;
                break;
            case 'r':method = 3;
                num         = atoi(util_optarg);
                if (num <= 0) {
                    phase_usage();
                    return 1;
                }
                break;
            case 's':method = 2;
                break;
            case 't':phase_trace_set();
                break;
            default:phase_usage();
                return 1;
        }
    }

    if (argc - util_optind != 0) {
        phase_usage();
        return 1;
    }

    switch (method) {
        case 0:phase_quick(*network);
            break;
        case 1:phase_good(*network);
            break;
        case 2:(void) fprintf(misout, "simulated annealing ");
            (void) fprintf(misout, "method has not been implemented\n");
            break;
        case 3:phase_random_greedy(*network, num);
            break;
        default: fail("com_phase_assign: unknown method");
    }

    return 0;
}

int com_add_inv(network_t **network, int argc, char **argv) {
    array_t *nodevec;
    int     i, n;
    node_t  *np;

    if (argc == 1) {
        add_inv_network(*network);
        return 0;
    }

    nodevec = com_get_nodes(*network, argc, argv);
    n       = array_n(nodevec);
    for (i  = 0; i < n; i++) {
        np = array_fetch(node_t *, nodevec, i);
        switch (np->type) {
            case PRIMARY_OUTPUT:break;
            default:(void) add_inv_node(*network, np);
        }
    }
    array_free(nodevec);
    return 0;
}

init_phase() {
    phase_trace_unset();
    phase_check_unset();
    com_add_command("phase", com_phase_assign, 1);
    com_add_command("add_inverter", com_add_inv, 1);
}

end_phase() {}

phase_usage() {
    (void) fprintf(miserr, "usage: phase [-gqst] [-r n]\n");
    (void) fprintf(miserr, "       -g Good phase\n");
    (void) fprintf(miserr, "       -q Quick phase\n");
    (void) fprintf(miserr, "       -s Simulated annealing\n");
    (void) fprintf(miserr, "       -t Trace\n");
    (void) fprintf(miserr, "       -r n Random greedy (n > 0)\n");
}
