
#include "decomp.h"
#include "decomp_int.h"
#include "sis.h"

static void td_usage();

static void decomp_usage();

int com_decomp(network_t **network, int argc, char **argv) {
    int     c, method;
    array_t *nodevec;
    node_t  *np;
    int     i;

    method = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "dgq")) != EOF) {
        switch (c) {
            case 'q':method = 0;
                break;
            case 'g':method = 1;
                break;
            case 'd':method = 2;
                break;
            default:decomp_usage();
                return 1;
        }
    }

    nodevec =
            com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);
    for (i = 0; i < array_n(nodevec); i++) {
        np = array_fetch(node_t *, nodevec, i);
        if (np->type == INTERNAL) {
            switch (method) {
                case 0:decomp_quick_node(*network, np);
                    break;
                case 1:decomp_good_node(*network, np);
                    break;
                case 2:decomp_disj_node(*network, np);
                    break;
                default:;
            }
        }
    }
    array_free(nodevec);
    return 0;
}

int com_tdecomp(network_t **network, int argc, char **argv) {
    int c, and_limit, or_limit;

    and_limit = 0;
    or_limit  = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "a:o:")) != EOF) {
        switch (c) {
            case 'a':
                if ((and_limit = atoi(util_optarg)) < 2) {
                    td_usage();
                    return 1;
                }
                break;
            case 'o':
                if ((or_limit = atoi(util_optarg)) < 2) {
                    td_usage();
                    return 1;
                }
                break;
            default:td_usage();
                return 1;
        }
    }

    if (and_limit < 2 && or_limit < 2) {
        td_usage();
        return 1;
    } else if (argc - util_optind == 0) {
        decomp_tech_network(*network, and_limit, or_limit);
    } else {
        td_usage();
        return 1;
    }

    return 0;
}

init_decomp() {
    com_add_command("decomp", com_decomp, 1);
    com_add_command("tech_decomp", com_tdecomp, 1);
}

end_decomp() {}

static void td_usage() {
    (void) fprintf(miserr, "usage: tech_decomp [-a and] [-o or]\n");
    (void) fprintf(miserr, "    -a and \tAnd gate with fanin limit 'and'\n");
    (void) fprintf(miserr, "    -o or \tOr gate with fanin limit 'or'\n");
}

static void decomp_usage() {
    (void) fprintf(miserr, "usage: decomp [-dqg] [node-list]\n");
    (void) fprintf(miserr, "    -q\t\tQuick decomposition (default)\n");
    (void) fprintf(miserr, "    -g\t\tGood decomposition\n");
    (void) fprintf(miserr, "    -d\t\tDisjoint decomposition\n");
}
