
/* speed_plot.c -- highlight critical paths. */

#include "sis.h"
#include "speed_int.h"
#include <math.h>

static char *usage[] = {
        "usage: %s [-n name] [-t thresh] [-cHgf] [-m model] [-t n.n] [-w n.n] [-d n] [-s method]\n",
        "    -n name\tPlot name to use instead of network name.\n",
        "    -c\t\tHighlight minimum weight cutset.\n",
        "    -g\t\tUse gate names instead of node names\n",
        "    -f\t\tUse fast routine to compute weights\n",
        "    -H\t\tHighlight critical path nodes.\n",
        "    -t\tn.n\tCritical threshold (used with -f option).\n",
        "    -w\tn.n\tRelative weight of area (used with -f option).\n",
        "    -d\tn\tDistance for collapsing.\n",
        "    -m\tmodel\t Delay model\n",
        "    -s\tmethod\tMethod for selecting the region to transform.\n \t one of \"crit\"(default), \"transitive\", \"compromise\", \"tree\"\n",
        NULL
};

static int speed_plot_use() {
    char **p;
    for (p = usage; *p != NULL; p++) {
        fprintf(sisout, *p, "_speed_plot");
    }
    return 1;
}

static void
do_speed_plot(network, plot_name, thresh, path_flag, cutset_flag,
              print_gate_name, model, dist, coeff, fast_mode,
              region_flag)
        network_t *network;
        char *plot_name;
        delay_model_t model;
        int dist, path_flag, cutset_flag;
        int print_gate_name;      /* Print name of the gate in place of node names */
        double coeff, thresh;
        int fast_mode;    /* Set the method to evaluate cutsets */
        int region_flag; /* Set the region to select the scope of transforms */
{
    int            i, flag;
    FILE           *gfp;
    lsGen          gen;
    node_t         *np;
    array_t        *mincut;
    double         eps, time;
    lib_gate_t     *gate;
    speed_global_t speed_param;
    st_table       *weight_table, *clp_table;
    extern void io_plot_network();

    if (!com_graphics_enabled()) return;

    /* We have already parsed the input --- just set those fields */
    (void) speed_fill_options(&speed_param, 0, NIL(
    char *));
    speed_param.thresh      = thresh;
    speed_param.model       = model;
    speed_param.coeff       = coeff;
    speed_param.dist        = dist;
    speed_param.new_mode    = fast_mode;
    speed_param.region_flag = region_flag;
    speed_param.model       = model;

    assert(delay_trace(network, speed_param.model));
    set_speed_thresh(network, &speed_param);
    np = delay_latest_output(network, &time);

    /* First make a Vannila Network like "plot_blif" and then
      embellish it according to the flags specified by the user */
    gfp = com_graphics_open("blif", plot_name, "new");
    io_plot_network(gfp, network, 0);
    com_graphics_close(gfp);

    /* Now for the add ons --- name changes, highlighting etc... */
    if (print_gate_name) {
        gfp = com_graphics_open("blif", plot_name, "label");
        foreach_node(network, gen, np)
        {
            if (np->type != INTERNAL) continue;
            if ((gate = lib_gate_of(np)) != NIL(lib_gate_t)) {
                fprintf(gfp, ".label\t%s\t%s\n", node_long_name(np),
                        lib_gate_name(gate));
            }
        }
        com_graphics_close(gfp);
    }

    gfp = com_graphics_open("blif", plot_name, "highlight");

    if (path_flag) {
        fprintf(gfp, ".clear\tDelay = %-5.2f\n", time);
    } else if (!cutset_flag) {
        fprintf(gfp, ".clear\t%s\n", network_name(network));
        fprintf(gfp, ".command\t_speed_plot\t_speed_plot -H -n %s", plot_name);
        fputs("\tCritical\tHighlight critical path(s).\n", gfp);
        fprintf(gfp, ".command\t_speed_plot\t_speed_plot -c -n %s", plot_name);
        fputs("\tCutset\tHighlight minimum weight cutset.\n", gfp);
    }

    if (cutset_flag || path_flag) {

        fprintf(sisout, "Distance = %d ; Threshold = %5.2f\n", dist, thresh);
        /* Get the weights for the cutset analysis  and find the cutset*/
        if (speed_param.new_mode) {
            clp_table = st_init_table(st_ptrcmp, st_ptrhash);
            new_speed_compute_weight(network, &speed_param,
                                     clp_table, &eps);
            if (eps < NSP_EPSILON) {
                mincut = array_alloc(node_t * , 0);
            } else {
                mincut = new_speed_select_xform(network, &speed_param,
                                                clp_table, eps);
            }
            new_free_weight_info(clp_table);
        } else {
            weight_table = speed_compute_weight(network, &speed_param, NIL(array_t));
            mincut       = cutset(network, weight_table);
        }

        if (path_flag) {        /* Highlight the critical path. */
            fputs(".nodes", gfp);
            foreach_node(network, gen, np)
            {
                if (speed_critical(np, &speed_param)) {
                    fprintf(gfp, "\t%s", node_long_name(np));
                }
            }
            fputs("\n", gfp);
        }

        if (cutset_flag && mincut != NIL(array_t) && array_n(mincut) > 0) {
            /* Highlight the cutset nodes. */
            if (!path_flag) {
                fprintf(gfp, ".clear\tCutset of %d nodes\n", array_n(mincut));
            }
            fputs(".nodes", gfp);
            for (i = array_n(mincut); i--;) {
                np = array_fetch(node_t * , mincut, i);
                fprintf(gfp, "\t%s", node_long_name(np));
            }
            fputs("\n", gfp);
        }

        array_free(mincut);
        st_free_table(weight_table);

    }

    com_graphics_close(gfp);
}

int
com__speed_plot(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int           c;
    delay_model_t model       = DELAY_MODEL_UNIT;
    int           hilite_flag = FALSE, print_gate_name = FALSE;
    int           cutset_flag = FALSE;
    int           region_flag = ALONG_CRIT_PATH;
    int           new_method  = TRUE;
    int           dist        = DEFAULT_SPEED_DIST;
    double        thresh      = DEFAULT_SPEED_THRESH;
    double        coeff       = DEFAULT_SPEED_COEFF;
    char          *plot_name  = network_name(*network);

    util_getopt_reset();

    while ((c = util_getopt(argc, argv, "Hcfgd:s:m:n:t:w:")) != EOF) {
        switch (c) {
            case 'c': cutset_flag = 1;
                break;
            case 'g': print_gate_name = TRUE;
                break;
            case 'f': new_method = FALSE;
                break;
            case 'H': hilite_flag = 1;
                break;
            case 'n': plot_name = util_optarg;
                break;
            case 't': thresh = atof(util_optarg);
                break;
            case 'w': coeff = atof(util_optarg);
                break;
            case 'd': dist = atoi(util_optarg);
                break;
            case 's':
                if (strcmp(util_optarg, "crit") == 0) {
                    region_flag = ALONG_CRIT_PATH;
                } else if (strcmp(util_optarg, "transitive") == 0) {
                    region_flag = TRANSITIVE_FANIN;
                } else if (strcmp(util_optarg, "compromise") == 0) {
                    region_flag = COMPROMISE;
                } else if (strcmp(util_optarg, "tree") == 0) {
                    region_flag = ONLY_TREE;
                } else {
                    (void) fprintf(sisout, "Illegal argument to the -s flag\n");
                    return speed_plot_use();
                }
                break;
            case 'm':model = delay_get_model_from_name(util_optarg);
                if (model == DELAY_MODEL_LIBRARY) {
                    model = DELAY_MODEL_MAPPED;
                } else if (model == DELAY_MODEL_UNKNOWN) {
                    (void) fprintf(siserr, "Unknown delay model %s\n", util_optarg);
                    return speed_plot_use();
                }
                break;
            default: return speed_plot_use();
        }
    }

    if (coeff < 0) coeff = 0;
    if (coeff > 1) coeff = 1;

    if (argc - util_optind > 0) return speed_plot_use();

    do_speed_plot(*network, plot_name, thresh, hilite_flag, cutset_flag,
                  print_gate_name, model, dist, coeff, new_method,
                  region_flag);
    return 0;
}



