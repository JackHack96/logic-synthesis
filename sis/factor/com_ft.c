
#include "sis.h"
#include "../include/factor.h"
#include "../include/factor_int.h"

#define PV_ANY        0
#define PV_DESCENDING 1
#define PV_ASCENDING  2

static node_t *first_fanin();

static void factor_usage();

static int el_valid_val();

int
com_factor(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int     c, method;
    array_t *nodevec;
    node_t  *np;
    int     i;

    method = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "bgq")) != EOF) {
        switch (c) {
            case 'q': method = 0;
                break;
            case 'g': method = 1;
                break;
            case 'b': method = 2;
                break;
            default: factor_usage();
                return 1;
        }
    }

    if (argc - util_optind == 0) {
        factor_usage();
        return 1;
    }

    nodevec = com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);
    for (i  = 0; i < array_n(nodevec); i++) {
        np = array_fetch(node_t * , nodevec, i);
        if (np->type == INTERNAL) {
            switch (method) {
                case 0: factor_quick(np);
                    break;
                case 1: factor_good(np);
                    break;
                case 2: (void) fprintf(misout, "boolean factoring is not available\n");
                    array_free(nodevec);
                    return 1;
            }
        }
    }
    array_free(nodevec);
    return 0;
}

int
com_pf(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    array_t *nodevec;
    int     i, n;
    node_t  *np, *fanin;

    nodevec = com_get_nodes(*network, argc, argv);
    n       = array_n(nodevec);
    for (i  = 0; i < n; i++) {
        np = array_fetch(node_t * , nodevec, i);
        switch (np->type) {
            case PRIMARY_INPUT: break;
            case PRIMARY_OUTPUT: fanin = first_fanin(np);
                if (fanin->type == PRIMARY_INPUT) {
                    (void) fprintf(misout, "    %s = ", node_name(np));
                    (void) fprintf(misout, "%s\n", node_name(fanin));
                }
                break;
            default: (void) fprintf(misout, "    %s = ", node_name(np));
                factor_print(misout, np);
                (void) fprintf(misout, "\n");
        }
    }
    array_free(nodevec);
    return 0;
}

int
com_pv(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    array_t *nodevec;
    int     i, j, n, num;
    int     order, c;
    node_t  *np;

    order = PV_ANY;
    num   = INFINITY;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "p:ad")) != EOF) {
        switch (c) {
            case 'p': num = atoi(util_optarg);
                break;
            case 'a': order = PV_ASCENDING;
                break;
            case 'd': order = PV_DESCENDING;
                break;
            default: (void) fprintf(misout, "usage: print_value [-pr] [node-list]\n");
                (void) fprintf(misout, "    -p\tn\tOnly print top 'n' values\n");
                (void) fprintf(misout, "    -d\t\tPrint values in in");
                (void) fprintf(misout, "descending order\n");
                (void) fprintf(misout, "    -a\t\tPrint values in in");
                (void) fprintf(misout, "ascending order\n");
                return 1;
        }
    }

    nodevec = com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);

    switch (order) {
        case PV_ASCENDING: array_sort(nodevec, value_cmp_inc);
            break;
        case PV_DESCENDING: array_sort(nodevec, value_cmp_dec);
            break;
        case PV_ANY: break;
    }

    n      = array_n(nodevec);
    j      = 0;
    for (i = 0; i < n && j < num; i++) {
        np = array_fetch(node_t * , nodevec, i);
        switch (np->type) {
            case PRIMARY_INPUT:
            case PRIMARY_OUTPUT: break;
            default: value_print(misout, np);
                j++;
        }
    }
    array_free(nodevec);
    return 0;
}

int
com_pft(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    array_t *nodevec;
    int     i, n;
    node_t  *np;

    nodevec = com_get_nodes(*network, argc, argv);
    n       = array_n(nodevec);
    for (i  = 0; i < n; i++) {
        np = array_fetch(node_t * , nodevec, i);
        switch (np->type) {
            case PRIMARY_INPUT:
            case PRIMARY_OUTPUT: break;
            default: (void) fprintf(misout, "--- %s ---\n", node_name(np));
                if (np->factored == NIL(char)) {
            (void) fprintf(misout, "empty.\n");
        } else {
            ft_print(np, (ft_t *) np->factored, 0);
        }
        }
    }
    array_free(nodevec);
    return 0;
}

static char *
my_node_long_name(node)
        node_t *node;
{
    if (name_mode == LONG_NAME_MODE) {
        if (node->name == NIL(char)) node_assign_name(node);
    } else {
        if (node->short_name == NIL(char)) node_assign_short_name(node);
    }

    return node->name;
}

int
com_factor_conv(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    array_t *nodevec, *fa;
    int     i, j, k;
    node_t  *f, *np, *fanin;

    nodevec = com_get_nodes(*network, argc, argv);
    for (i  = 0; i < array_n(nodevec); i++) {
        f = array_fetch(node_t * , nodevec, i);
        if (f->type == INTERNAL) {
            (void) fprintf(misout, "    %s = ", node_name(f));
            factor_print(misout, f);
            (void) fprintf(misout, "\n");
            fa     = factor_to_nodes(f);
            for (j = 0; j < array_n(fa); j++) {
                np = array_fetch(node_t * , fa, j);
                switch (node_function(np)) {
                    case NODE_AND: (void) fprintf(misout, "\t%s = AND(", my_node_long_name(np));
                        foreach_fanin(np, k, fanin)
                        {
                            (void) fprintf(misout, " %s",
                                           my_node_long_name(fanin));
                        }
                        (void) fprintf(misout, ")\n");
                        break;
                    case NODE_OR: (void) fprintf(misout, "\t%s = OR(", my_node_long_name(np));
                        foreach_fanin(np, k, fanin)
                        {
                            (void) fprintf(misout, " %s",
                                           my_node_long_name(fanin));
                        }
                        (void) fprintf(misout, ")\n");
                        break;
                    case NODE_BUF: fanin = node_get_fanin(np, 0);
                        (void) fprintf(misout, "\t%s = BUF", my_node_long_name(np));
                        (void) fprintf(misout, "(%s)\n",
                                       my_node_long_name(fanin));
                        break;
                    case NODE_INV: fanin = node_get_fanin(np, 0);
                        (void) fprintf(misout, "\t%s = INV", my_node_long_name(np));
                        (void) fprintf(misout, "(%s)\n",
                                       my_node_long_name(fanin));
                        break;
                    case NODE_0: (void) fprintf(misout, "\t%s = 0\n", my_node_long_name(np));
                        break;
                    case NODE_1: (void) fprintf(misout, "\t%s = 1\n", my_node_long_name(np));
                        break;
                    default: (void) fprintf(misout, "illegal node function\n");
                        return 1;
                }
                node_free(np);
            }
            array_free(fa);
            (void) fprintf(misout, "\n");
        }
    }
    array_free(nodevec);
    return 0;
}

int
com_eliminate(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int limit, value;

    limit = 1000;
    if (argc == 2) {
        if (el_valid_val(argv[1])) {
            value = atoi(argv[1]);
        } else goto usage;
    } else if (argc == 4 && strcmp(argv[1], "-l") == 0) {
        if (el_valid_val(argv[2])) {
            limit = atoi(argv[2]);
        } else goto usage;
        if (el_valid_val(argv[3])) {
            value = atoi(argv[3]);
        } else goto usage;
    } else goto usage;

    eliminate(*network, value, limit);
    return 0;
    usage:
    (void) fprintf(miserr, "usage: eliminate [-l limit] value\n");
    return 1;
}

static int
el_valid_val(s)
        char *s;
{
    int i, first;

    first  = (*s == '-' ? 1 : 0);
    for (i = first; i < strlen(s); i++) {
        if (!isdigit(*(s + i))) return 0;
    }
    return 1;
}


init_factor() {
    set_line_width();

    node_register_daemon(DAEMON_ALLOC, factor_alloc);
    node_register_daemon(DAEMON_FREE, factor_free);
    node_register_daemon(DAEMON_DUP, factor_dup);
    node_register_daemon(DAEMON_INVALID, factor_invalid);

    com_add_command("print_value", com_pv, 0);
    com_add_command("print_factor", com_pf, 0);
    com_add_command("factor", com_factor, 1);
    com_add_command("eliminate", com_eliminate, 1);

    com_add_command("_pft", com_pft, 0);
    com_add_command("_conv", com_factor_conv, 0);
}

end_factor() {
}

static node_t *
first_fanin(np)
        node_t *np;
{
    node_t *fanin, *first;
    int    i;

    first = NIL(node_t);
    foreach_fanin(np, i, fanin)
    {
        if (i == 0) {
            first = fanin;
        }
    }

    return first;
}


static void
factor_usage() {
    (void) fprintf(miserr, "usage: factor [-qgb] node-list\n");
    (void) fprintf(miserr, "    -q\t\tQuick factoring (default)\n");
    (void) fprintf(miserr, "    -g\t\tGood factoring\n");
    (void) fprintf(miserr, "    -b\t\tBoolean factoring\n");
}
