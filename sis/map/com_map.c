
/* file @(#)com_map.c	1.22 */
/* Last modified on Tue Jun 22 06:43:58 MET DST 1993 by touati */
#include "cluster.h"
#include "fanout_int.h"
#include "lib_int.h"
#include "map_defs.h"
#include "map_delay.h"
#include "map_int.h"
#include "sis.h"

static array_t *current_fanout_alg;

static void map_report_usage();

#ifdef SIS
st_table *network_type_table;

static void map_latch_table_dup();

static node_t *map_copy_clock();

#endif

static int com_replace(network_t **network, int argc, char **argv) {
    int c;
    int verbose = 0;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "v:")) != EOF) {
        switch (c) {
            case 'v':verbose = atoi(util_optarg);
                break;
            default:goto usage;
        }
    }
    replace_2or(*network, verbose);
    return 0;
    usage:
    (void) fprintf(siserr, "replace [-v verbosity level]\n");
    return 1;
}

/* perform premapping, mapping, fanout optimization... */

static int com_map(network_t **network, int argc, char **argv) {
    network_t    *new_network;
    bin_global_t options;

    if (map_fill_options(&options, &argc, &argv)) {
        fprintf(siserr, "%s", error_string());
        map_report_usage("map");
        return 1;
    }
    new_network = complete_map_interface(*network, &options);
    if (new_network == 0) {
        fprintf(siserr, "%s", error_string());
        return 1;
    }
    new_network->dc_network = network_dup((*network)->dc_network);
#ifdef SIS
    new_network->stg = stg_dup((*network)->stg);
    map_latch_table_dup(*network, new_network);
#endif

    network_free(*network);
    *network = new_network;
    return 0;
}

/* builds a MAPPED network from an ANNOTATED network */

static int com_build_map(network_t **network, int argc, char **argv) {
    network_t    *new_network;
    bin_global_t globals;

    if (map_fill_options(&globals, &argc, &argv)) {
        fprintf(siserr, "%s", error_string());
        map_report_usage("map");
        return 1;
    }
    new_network = build_mapped_network_interface(*network, &globals);
    if (new_network == NIL(network_t)) {
        fprintf(siserr, "%s", error_string());
        return 1;
    }
    network_free(*network);
    *network = new_network;
    return 0;
}

/* takes a RAW or an ANNOTATED network and returns an ANNOTATED network */
/* only do tree mapping */

static int com_tree_map(network_t **network, int argc, char **argv) {
    network_t    *new_network;
    bin_global_t options;

    if (map_fill_options(&options, &argc, &argv)) {
        fprintf(siserr, "%s", error_string());
        map_report_usage("map");
        return 1;
    }
    new_network = tree_map_interface(*network, &options);
    if (new_network == 0) {
        fprintf(siserr, "%s", error_string());
        return 1;
    }
    if (new_network != *network) {
        network_free(*network);
        *network = new_network;
    }
    return 0;
}

/* takes an ANNOTATED network and returns an ANNOTATED network */
/* performs fanout optimization and, if specified, area recovery */

static int com_fanout_opt(network_t **network, int argc, char **argv) {
    network_t    *new_network;
    bin_global_t options;

    if (map_fill_options(&options, &argc, &argv)) {
        fprintf(siserr, "%s", error_string());
        map_report_usage("map");
        return 1;
    }
    new_network = fanout_opt_interface(*network, &options);
    if (new_network == 0) {
        fprintf(siserr, "%s", error_string());
        return 1;
    }
    if (new_network != *network) {
        network_free(*network);
        *network = new_network;
    }
    return 0;
}

/* can be used to add/remove fanout algorithms from the list of active alg's */

/* ARGSUSED */
static int com_fanout_alg(network_t **network, int argc, char **argv) {
    int no_options = 0;
    int verbose    = 0;

    if (current_fanout_alg == NIL(array_t)) {
        current_fanout_alg = fanout_opt_get_fanout_alg(0, NIL(char *));
    }
    argc--;
    argv++;
    if (argc == 0)
        no_options         = 1;
    while (argc > 0 && argv[0][0] == '-') {
        switch (argv[0][1]) {
            case 'v':verbose = 1;
                break;
            default:goto usage;
        }
        argc--;
        argv++;
    }
    if (!no_options)
        current_fanout_alg = fanout_opt_get_fanout_alg(argc, argv);
    if (verbose || no_options)
        fanout_opt_print_fanout_alg(current_fanout_alg);
    return 0;
    usage:
    fprintf(siserr, "usage: fanout_alg [-v] fanout_alg1 fanout_alg2 ...\n");
    fprintf(siserr, "       -v\t verbose mode\n");
    return 1;
}

/* can be used to give values to parameters of specific fanout algorithms */

/* ARGSUSED */
static int com_fanout_param(network_t **network, int argc, char **argv) {
    int fanout_alg_index;
    int verbose = 0;

    if (current_fanout_alg == NIL(array_t))
        current_fanout_alg = fanout_opt_get_fanout_alg(0, NIL(char *));
    argc--;
    argv++;
    while (argc > 0 && argv[0][0] == '-') {
        switch (argv[0][1]) {
            case 'v':verbose = 1;
                break;
            default:goto usage;
        }
        argc--;
        argv++;
    }
    fanout_opt_set_fanout_param(current_fanout_alg, argc, argv,
                                &fanout_alg_index);
    if (fanout_alg_index == -1)
        goto usage;
    if (verbose)
        fanout_opt_print_fanout_param(current_fanout_alg, fanout_alg_index);
    return 0;
    usage:
    fprintf(siserr, "usage: fanout_param [-v] fanout_alg property value\n");
    fprintf(siserr, "       -v\t lists all (property, value) pairs associated "
                    "with algorithm\n");
    fanout_opt_print_fanout_alg(current_fanout_alg);
    return 1;
}

static bin_global_t DEFAULT_OPTIONS = {
        0,   /* int new_mode; 	        // new_mode */
        0.0, /* double new_mode_value;	// 0.0->area ... 1.0->delay */
        0,   /* int load_bins_count;		// historical */
        0,   /* int delay_bins_count;	// historical */
        0.0, /* double old_mode;		// old map; area or delay optimize */
        1,   /* int inverter_optz;		// inverter optimization heuristics */
        -1,  /* int allow_internal_fanout;	// degrees of internal fanouts  allowed
          */
        0,   /* int print_stat;		// print area/delay stats at the end */
        0,   /* int verbose;			// verbosity level */
        0,   /* int raw;			// raw mapping */
        0.0, /* double thresh;		// threshold used in old tree mapper */
        0,   /* library_t *library;		// current library */
        0,   /* int fanout_optimize;		// if set, the fanout optimizer is
        called */
        0,   /* int area_recover;		// if set, area is recovered after
        fanout opt */
        0,   /* int peephole;		// if set, call fanout peephole optimizer */
        0,   /* array_t *fanout_alg;		// array of fanout algorithm
        descriptors */
        0,   /* int remove_inverters;	// if set, remove redundant inverters   after
        mapping */
        0,   /* int no_warning;		// if set, does not print any warning
        message */
        0,   /* int ignore_pipo_data;	// if set, ignore specific values set at
        PIPO's */
        RAW_NETWORK, /* network_type_t network_type; // type of the network when
                mapping is started */
        2,           /* int load_estimation;		// 0-ignore fanout; 1-linear fanout;
                2-load of fanout tree */
        1,           /* int ignore_polarity;		// 0-add an inverter delay; 1-same
                arrival for both pol. */
        0,           /* int cost_function;		// 0-MAX(rise,fall); 1-AVER(rise,fall)
                  */
        0,           /* int n_iterations;		// 0-do it once, 1-repeat once, etc...
                  */
        0,           /* int fanout_iterate;		// 1 if tree covering is called
                afterwards; 0 otherwise */
        1,           /* int opt_single_fanout;	// 0/1 */
        0,           /* int fanout_log_on;		// 0/1 */
        0,           /* int allow_duplication;       // 0/1 */
        INFINITY,    /* int fanout_limit;		// limit fanout of internal
                nodes during matching */
        1, /* int check_load_limit;        // if set, check load limit during fanout
      opt */
        1000, /* int penalty_factor;		// factor by which load is
         multiplied when exceeds max load limit of a gate */
        0 /* int all_gates_area_recover;  // when set, recover area on all gates,
     not only buffers */
};

int map_fill_options(bin_global_t *options, int *argc_addr, char ***argv_addr) {
    int    c;
    double mode;
    int    argc   = *argc_addr;
    char   **argv = *argv_addr;

    error_init();
    *options = DEFAULT_OPTIONS;
    if (current_fanout_alg == NIL(array_t)) {
        current_fanout_alg = fanout_opt_get_fanout_alg(0, NIL(char *));
    }
    options->fanout_alg   = current_fanout_alg;
    if ((options->library = lib_get_library()) == NIL(library_t)) {
        error_append("map_interface: need to load a library with read_library\n");
        return 1;
    }
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "Ab:B:c:dFGf:iI:l:L:m:n:pPrst:v:W")) !=
           EOF) {
        switch (c) {
            case 'A':options->area_recover = 1;
                options->fanout_optimize   = 1;
                break;
            case 'G':options->all_gates_area_recover = 1;
                options->area_recover                = 1;
                options->fanout_optimize             = 1;
                break;
            case 'b':options->penalty_factor = atoi(util_optarg);
                if (options->penalty_factor < 0) {
                    error_append("map_interface: -b n: n should be non negative\n");
                    return 1;
                }
                break;
            case 'B':options->check_load_limit = atoi(util_optarg);
                if (options->check_load_limit != 0 && options->check_load_limit != 1) {
                    error_append("map_interface: -B n: n should be 0 or 1\n");
                    return 1;
                }
                break;
            case 'c':options->cost_function = atoi(util_optarg);
                if (options->cost_function != 0 && options->cost_function != 1) {
                    error_append("map_interface: -c n: n should be 0 or 1\n");
                    return 1;
                }
                break;
            case 'd':options->allow_duplication = 1;
                break;
            case 'F':options->fanout_optimize = 1;
                break;
            case 'f':options->allow_internal_fanout = atoi(util_optarg);
                break;
            case 'i':options->inverter_optz = 0;
                break;
            case 'I':options->n_iterations = atoi(util_optarg);
                break;
            case 'l':options->load_estimation = atoi(util_optarg);
                if (options->load_estimation < 0 || options->load_estimation > 3) {
                    error_append("map_interface: -l n: n should be 0, 1, 2 or 3\n");
                    return 1;
                }
                break;
            case 'L':options->fanout_limit = atoi(util_optarg);
                if (options->load_estimation < 1) {
                    error_append("map_interface: -L n: n should be >= 1\n");
                    return 1;
                }
                break;
            case 'm':options->old_mode = atof(util_optarg);
                if (options->old_mode > 1.0000 || options->old_mode < 0) {
                    error_append("map_interface: -m n: n should be in interval (0,1)\n");
                    return 1;
                }
                break;
            case 'n':options->new_mode  = 1;
                options->new_mode_value = atof(util_optarg);
                mode = options->new_mode_value;
                if (mode < 0.0 || mode > 1.0) {
                    error_append("map_interface: -n n: n should be in interval (0,1)\n");
                    return 1;
                }
                if (mode == 0.0) {
                    options->new_mode = 0;
                    options->old_mode = 0.0;
                }
                break;
            case 'p':options->ignore_pipo_data = 1;
                break;
            case 'P':options->ignore_polarity = 1 - options->ignore_polarity;
                break;
            case 'r':options->raw = 1;
                break;
            case 's':options->print_stat = 1;
                break;
            case 't':options->old_mode = 2.0;
                options->thresh        = atof(util_optarg);
                break;
            case 'v':options->verbose = atoi(util_optarg);
                break;
            case 'W':options->no_warning = 1;
                break;
            default:error_append("map_interface: unknown option\n");
                return 1;
        }
    }

    /*
      removing inverters after old treemapping usually improves delay.
      if (! options->fanout_optimize && options->new_mode == 0 &&
      options->old_mode == 0.0) {
    */
    if (!options->fanout_optimize && options->new_mode == 0) {
        options->remove_inverters = 1;
    }

    if (options->allow_internal_fanout ==
        -1) { /* determine the default for internal fanout: 11 for min area, 0
             otherwise */
        options->allow_internal_fanout = 0;
        if (!options->fanout_optimize && options->new_mode == 0 &&
            options->old_mode == 0.0) {
            options->allow_internal_fanout = 11;
        }
    } else { /* if -f option was specified non zero with the fanout optimizer,
            disallow it */
        /* do not disallow it any more: needed for experiments.
            if (options->allow_internal_fanout != 0 && options->fanout_optimize) {
              options->allow_internal_fanout = 0;
              if (! options->no_warning) (void) fprintf(siserr, "WARNING: internal
           fanout disallowed with fanout optimization\n");
            }
        */
    }
    *argc_addr = argc;
    *argv_addr = argv;
    return 0;
}

static void map_report_usage(char *command_name) {
    (void) fprintf(siserr, "usage: %s\n", command_name);
    (void) fprintf(siserr, "############################## VERBOSE OPTIONS "
                           "##############################\n");
    (void) fprintf(siserr, "\t-v n\tselect verbosity level; for debugging\n");
    (void) fprintf(siserr, "\t-W\tsuppresses warning messages\n");
    (void) fprintf(siserr, "\t-s\tprint stats at the end\n");
    (void) fprintf(siserr, "############################## BASIC OPTIONS "
                           "##############################\n");
    (void) fprintf(
            siserr,
            "\t-m n\told map mode: n=0.0: minimize area; n=1.0 minimize delay\n");
    (void) fprintf(siserr,
                   "\t-n n\tnew map mode: n=1.0: better delay minimization\n");
    (void) fprintf(siserr, "\t-F\tfanout optimization for delay\n");
    (void) fprintf(siserr, "\t-A\tsecond fanout optimization pass: less area for "
                           "no cost in delay\n");
    (void) fprintf(siserr, "\t-G\trecovers area after fanout optimization at no "
                           "cost in delay by resizing all gates\n");
    (void) fprintf(siserr, "############################## ADVANCED OPTIONS "
                           "##############################\n");
    (void) fprintf(siserr, "\t-p\tignore PI/PO delay declarations; use global "
                           "delay declarations instead\n");
    (void) fprintf(miserr,
                   "\t-B n\tenforce load limit during fanout optimization: 0=no "
                   "enforcement 1(default)=enforcement\n");
    (void) fprintf(miserr, "\t-b n\t use with -B: multiply load by n when load "
                           "limit exceeded; default n=1000\n");
    (void) fprintf(siserr, "\t-c n\tcost function in '-n 1': n=0->MAX(rise,fall), "
                           "n=1->AVER(rise,fall)\n");
    (void) fprintf(siserr, "\t-f n\tallow internal fanout (n = 0: none)\n");
    (void) fprintf(siserr, "\t-i\tinverter optimization\n");
    (void) fprintf(siserr, "############################## EXPERIMENTAL OPTIONS "
                           "##############################\n");
    (void) fprintf(siserr, "\t-l n\tn=0->load=load; n=1->load=load*n_fanouts; "
                           "n=2->load=load*sqrt(n_fanouts)\n");
    (void) fprintf(
            siserr,
            "\t-L n\tn = fanout limit when allowing overlaps between covers \n");
    (void) fprintf(siserr, "\t-P\tadd inverter delay if difft polarity than "
                           "source at fanout node\n");
    (void) fprintf(siserr, "\t-r\traw map\n");
    (void) fprintf(siserr, "\t-t n\told_mode with threshold n\n");
}

/* ARGSUSED */
static int com_read_library(network_t **network, int argc, char **argv) {
    FILE      *fp, *outfp;
    char      *filename, *real_filename;
    int       append, add_inverters, nand_flag, raw, c;
    library_t *old_library;

    append        = 0;        /* default is no-append */
    raw           = 0;           /* default is genlib format */
    add_inverters = 1; /* default adds inverters */
    nand_flag     = 0;     /* default is nor */
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "ainrbs")) != EOF) {
        switch (c) {
            case 'a':append = 1;
                break;
            case 'i':add_inverters = 0;
                break;
            case 'n':nand_flag = 1;
                break;
            case 'r':raw = 1;
                break;
            default:goto usage;
        }
    }
    if (argc - util_optind != 1)
        goto usage;

    /* save the old library */
    old_library = lib_current_library;

    filename = argv[util_optind];
    fp       = com_open_file(filename, "r", &real_filename, /* silent */ 0);
    if (fp == NULL)
        goto error_return;

    if (!append) {
        lib_current_library = 0;
    }

    /* run the library though genlib if not reading it raw */
    if (!raw) {
        outfp = util_tmpfile();
        if (outfp == NULL) {
            (void) fprintf(siserr, "Error opening temporary file\n");
            lib_current_library = old_library;
            return 1;
        }
        if (!genlib_parse_library(fp, real_filename, outfp, !nand_flag)) {
            (void) fprintf(siserr, "%s", error_string());
            (void) fclose(fp);
            goto error_return;
        }
        (void) fclose(fp);
        rewind(outfp);
        fp = outfp;
    }

    /* read a new library */
    if (!lib_read_blif(fp, real_filename, add_inverters, nand_flag,
                       &lib_current_library)) {
        (void) fclose(fp);
        (void) fprintf(sisout, "%s", error_string());
        goto error_return;
    }

    (void) fclose(fp);
    FREE(real_filename);
    if (append == 0 && old_library != 0) {
        lib_free(old_library);
    }
    return 0;

    usage:
    (void) fprintf(siserr, "usage: read_library [-adnr] filename\n");
    (void) fprintf(siserr, "	-a\t\tappend to current library\n");
    (void) fprintf(siserr, "	-i\t\tsuppress add inverters\n");
    (void) fprintf(siserr, "	-n\t\tuse NAND gates (instead of NOR)\n");
    (void) fprintf(siserr, "	-r\t\tread raw blif (rather than genlib)\n");
    return 1;

    error_return:
    FREE(real_filename);
    lib_current_library = old_library;
    return 1;
}

typedef struct gate_count_struct gate_count_t;
struct gate_count_struct {
    char   *name;
    int    count;
    double area;
};

static void gate_summary(array_t *node_vec) {
    int           i, total_gates;
    node_t        *node;
    gate_count_t  *count;
    lib_gate_t    *gate;
    char          *gatename, *key, *value;
    double        total_area;
    avl_generator *avlgen;
    avl_tree      *gate_count_table;

    gate_count_table = avl_init_table(strcmp);

    for (i = 0; i < array_n(node_vec); i++) {
        node = array_fetch(node_t *, node_vec, i);
        if (node->type == INTERNAL) {
            gate = lib_gate_of(node);
            if (gate == NIL(lib_gate_t)) {
                gatename = "<none>";
            } else {
                gatename     = lib_gate_name(gate);
                if (gatename == NIL(char))
                    gatename = "<none>";
            }

            if (avl_lookup(gate_count_table, gatename, &value)) {
                count = (gate_count_t *) value;
                count->count++;
            } else {
                count = ALLOC(gate_count_t, 1);
                count->name  = util_strsav(gatename);
                count->count = 1;
                count->area  = lib_gate_area(gate);
                value = (char *) count;
                (void) avl_insert(gate_count_table, count->name, value);
            }
        }
    }

    total_area  = 0;
    total_gates = 0;
    avl_foreach_item(gate_count_table, avlgen, AVL_FORWARD, &key, &value) {
        count = (gate_count_t *) value;
        (void) fprintf(sisout, "%-15s : %4d (area=%4.2f)\n", count->name,
                       count->count, count->area);
        total_area += count->count * count->area;
        total_gates += count->count;
    }
    (void) fprintf(sisout, "Total: %d gates, %4.2f area\n", total_gates,
                   total_area);
    avl_free_table(gate_count_table, free, free);
}

static void print_lib_gate_info(node_t *node, int print_pins) {
    int        j;
    node_t     *fanin;
    lib_gate_t *gate;
    char       *pinname, *gatename;
    double     area;

    if (node->type == INTERNAL) {
        gate = lib_gate_of(node);
        if (gate == NIL(lib_gate_t)) {
            gatename = "<none>";
            area     = 0.0;
        } else {
            gatename = lib_gate_name(gate);
            area     = lib_gate_area(gate);
        }

        (void) fprintf(sisout, "%-10s %-15s %4.2f\t", node_name(node), gatename,
                       area);
        if (print_pins && gate != NIL(lib_gate_t)) {
            (void) fprintf(sisout, "  (");
            foreach_fanin(node, j, fanin) {
                pinname = lib_gate_pin_name(gate, j, /* inflag */ 1);
                if (j != 0)
                    (void) fprintf(sisout, " ");
                (void) fprintf(sisout, "%s=%s", pinname, node_name(fanin));
            }
            (void) fprintf(sisout, ")");
        }
        (void) fprintf(sisout, "\n");
    }
}

/* ARGSUSED */
static int com_print_gate(network_t **network, int argc, char **argv) {
    node_t  *node;
    array_t *node_vec;
    int     i, print_pins, summary, c;

    print_pins = 0;
    summary    = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "ps")) != EOF) {
        switch (c) {
            case 'p':print_pins = 1;
                break;
            case 's':summary = 1;
                break;
            default:goto usage;
        }
    }

    node_vec =
            com_get_nodes(*network, argc - util_optind + 1, argv + util_optind - 1);
    if (array_n(node_vec) < 1)
        goto usage;

    if (summary) {
        gate_summary(node_vec);
    } else {
        for (i = 0; i < array_n(node_vec); i++) {
            node = array_fetch(node_t *, node_vec, i);
            print_lib_gate_info(node, print_pins);
        }
    }
    array_free(node_vec);
    return 0;

    usage:
    (void) fprintf(siserr, "usage: print_gate [-ps] n1 n2 ...\n");
    return 1;
}

/* ARGSUSED */
static int com_treesize(network_t **network, int argc, char **argv) {
    if (argc != 1)
        goto usage;

    map_print_tree_size(sisout, *network);
    return 0;

    usage:
    (void) fprintf(siserr, "usage: _treesize\n");
    return 1;
}

/* ARGSUSED */
static int com_eat_buffer(network_t **network, int argc, char **argv) {
    lsGen  gen;
    node_t *node;

    if (argc != 1)
        goto usage;
    foreach_primary_output(*network, gen, node) {
        (void) network_sweep_node(node);
    }
    (void) network_cleanup(*network);
    return 0;

    usage:
    (void) fprintf(siserr, "usage: _eat_buffer\n");
    return 1;
}

/* ARGSUSED */
static int com_premap(network_t **network, int argc, char **argv) {
    library_t *library;
    library = lib_get_library();

    if (library == 0) {
        (void) fprintf(siserr, "_premap: no current library\n");
        return 1;
    }
    if (!map_check_form(*network, library->nand_flag)) {
        return 0;
    }

    error_init();
    if (!do_tree_premap(*network, library)) {
        (void) fprintf(siserr, "%s", error_string());
    }
    return 0;
}

static int com_map_addinv(network_t **network, int argc, char **argv) {
    int at_pipo, c;

    at_pipo = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "p")) != EOF) {
        switch (c) {
            case 'p':at_pipo = 1;
                break;
            default:goto usage;
        }
    }
    if (argc - util_optind != 0)
        goto usage;

    map_add_inverter(*network, at_pipo);
    return 0;

    usage:
    (void) fprintf(siserr, "usage: _addinv [-p]\n");
    return 1;
}

/* ARGSUSED */
static int com_map_reminv(network_t **network, int argc, char **argv) {
    if (argc != 1) {
        (void) fprintf(siserr, "usage: _reminv\n");
        return 1;
    }
    if (lib_network_is_mapped(*network)) {
        map_remove_inverter(*network, map_report_data_mapped);
        map_report_data_mapped(*network);
    } else {
        (void) fprintf(sisout,
                       ">>> removing extra series and parallel inverters <<<\n");
        map_remove_inverter(*network, (VoidFn) 0);
    }
    return 0;
}

/* ARGSUSED */
static int com_buffer(network_t **network, int argc, char **argv) {
    if (argc != 1) {
        (void) fprintf(siserr, "usage: _buffer\n");
        return 1;
    }
    if (lib_get_library() == 0) {
        (void) fprintf(siserr, "print_library: no current library\n");
        return 1;
    }
    buffer_inputs(*network, lib_get_library());
    return 0;
}

/* ARGSUSED */
static int com_dump_patterns(network_t **network, int argc, char **argv) {
    if (lib_get_library() == 0) {
        (void) fprintf(siserr, "print_library: no current library\n");
        return 1;
    }

    lib_dump(sisout, lib_get_library(), /* detail */ 1);
    return 0;
}

/* ARGSUSED */
static int com_print_library(network_t **network, int argc, char **argv) {
    lib_class_t *class;
    network_t   *net1;
    library_t   *library;
#ifdef SIS
    latch_synch_t type;
    int           c, print_gates;
    lsGen         gen;
    lib_gate_t    *gate;
#endif

    library = lib_get_library();
    if (library == 0) {
        (void) fprintf(siserr, "print_library: no current library\n");
        return 1;
    }

#ifdef SIS
    /* sequential support */
    type        = COMBINATIONAL;
    print_gates = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "afhlrG")) != EOF) {
        switch (c) {
            case 'a':type = ASYNCH;
                break;
            case 'f':type = FALLING_EDGE;
                break;
            case 'h':type = ACTIVE_HIGH;
                break;
            case 'l':type = ACTIVE_LOW;
                break;
            case 'r':type = RISING_EDGE;
                break;
            case 'G':print_gates = 1;
                break;
            default:goto usage;
        }
    }
    if (print_gates) {
        lsForeachItem(library->gates, gen, gate) { lib_dump_gate(gate); }
        return 0;
    }

    if (argc == util_optind) {
        lib_dump(sisout, library, /* detail */ 0);
    } else if (argc - util_optind == 1) {
        net1 = read_eqn_string(argv[util_optind]);
        if (net1 == 0) {
            (void) fprintf(siserr, "%s", error_string());
            return 1;
        }
        class = lib_get_class_by_type(net1, library, type);
        if (class == 0) {
            (void) fprintf(siserr,
                           "print_library: cannot find class for given function\n");
            return 1;
        }
        lib_dump_class(sisout, class);
        network_free(net1);
    } else {
        goto usage;
    }
    return 0;
    usage:
    (void) fprintf(siserr, "usage: print_library [-afhlrG] [function-string]\n");
    (void) fprintf(siserr, "       -a\t\tasynchronous latches only\n");
    (void) fprintf(siserr, "       -f\t\tfalling_edge latches only\n");
    (void) fprintf(siserr, "       -h\t\tactive_high latches only\n");
    (void) fprintf(siserr, "       -l\t\tactive_low latches only\n");
    (void) fprintf(siserr, "       -r\t\trising_edge latches only\n");
    (void) fprintf(siserr, "       -G\t\tprint library gate params\n");
    return 1;
#else
    if (argc == 1) {
      lib_dump(sisout, lib_get_library(), /* detail */ 0);

    } else if (argc == 2) {
      net1 = read_eqn_string(argv[1]);
      if (net1 == 0) {
        fprintf(siserr, "%s", error_string());
        return 1;
      }
      class = lib_get_class(net1, lib_get_library());
      if (class == 0) {
        fprintf(siserr, "print_library: cannot find class for given function\n");
        return 0;
      }
      lib_dump_class(sisout, class);
      network_free(net1);

    } else {
      fprintf(siserr, "usage: print_library [function-string]\n");
      return 1;
    }
    return 0;
#endif
}

/* extract data from a network. Contribution of Tzvi Ben-Tzur */

/* ARGSUSED */
int com_print_map_stats(network_t **network, int argc, char **argv) {
    lsGen        gen;
    int          num_out = 0, count = 0, num_inv = 0, num_buf = 0;
    double       area    = 0.0;
    delay_time_t t;
    double       temp, min_slack, total_slack;
    node_t       *node;

    if (!lib_network_is_mapped(*network)) {
        (void) fprintf(sisout, "Network not mapped\n");
        return 1;
    }

    assert(delay_trace(

            *network, DELAY_MODEL_LIBRARY));

    min_slack   = 100000.0;
    total_slack = 0.0;

    foreach_node(*network, gen, node) {
        if (node->type == INTERNAL) {
            count++;
            area +=

                    lib_gate_area(lib_gate_of(node));

            if (node_function(node) == NODE_INV) {
                num_inv++;
            } else if (node_function(node) == NODE_BUF) {
                num_buf++;
            }
        } else if (node->type == PRIMARY_OUTPUT) {
            t    = delay_slack_time(node);
            temp = MIN(t.rise, t.fall);
            if (temp < 0) {
                num_out++;
                min_slack = MIN(temp, min_slack);
                total_slack += temp;
            }
        }
    }
    (void) fprintf(sisout, "Total Area\t\t= %6.2f\n", area);
    (void) fprintf(sisout, "Gate Count\t\t= %d\n", count);
    (void) fprintf(sisout, "Buffer Count\t\t= %d\n", num_buf);
    (void) fprintf(sisout, "Inverter Count \t\t= %d\n", num_inv);
    min_slack   = (min_slack < 0 ? min_slack : 0);
    total_slack = (total_slack < 0 ? total_slack : 0);
    (void) fprintf(sisout, "Most Negative Slack\t= %6.2f\n", min_slack);
    (void) fprintf(sisout, "Sum of Negative Slacks\t= %6.2f\n", total_slack);
    (void) fprintf(sisout, "Number of Critical PO\t= %d\n", num_out);
    return 0;
}

/* reduces depth by clustering using Lawler's algorithm */

network_t *cluster_under_size_constraint();

network_t *cluster_under_depth_constraint();

network_t *cluster_under_size_as_depth_constraint();

int com_reduce_depth(network_t **network, int argc, char **argv) {
    int             c;
    network_t       *new_network;
    clust_options_t options;

    /* default */
    options.type         = SIZE_CONSTRAINT;
    options.relabel      = 0;
    options.verbose      = 0;
    options.cluster_size = 8;
    options.depth        = -1; /* initialized to unspecified */
    options.dup_ratio    = 2.0;

    util_getopt_reset();

    while ((c = util_getopt(argc, argv, "bd:f:rR:s:S:v:g")) != EOF) {
        switch (c) {
            case 'b':options.type = BEST_RATIO_CONSTRAINT;
                options.relabel   = 1;
                break;
            case 'R':options.dup_ratio = atof(util_optarg);
                if (options.dup_ratio<1.0 || options.dup_ratio>INFINITY)
                    goto usage;
                break;
            case 'd':options.type = DEPTH_CONSTRAINT;
                options.depth     = atoi(util_optarg);
                if (options.depth <= 0)
                    goto usage;
                break;
            case 'f':options.type    = FANIN_CONSTRAINT;
                options.cluster_size = atoi(util_optarg);
                if (options.cluster_size <= 1)
                    goto usage;
                break;
            case 'g':options.type = CLUSTER_STATISTICS;
                break;
            case 'r':options.relabel = 1;
                break;
            case 's':options.type    = SIZE_CONSTRAINT;
                options.cluster_size = atoi(util_optarg);
                if (options.cluster_size <= 0)
                    goto usage;
                break;
            case 'S':options.type    = SIZE_AS_DEPTH_CONSTRAINT;
                options.cluster_size = atoi(util_optarg);
                if (options.cluster_size <= 0)
                    goto usage;
                break;
            case 'v':options.verbose = atoi(util_optarg);
                break;
            default:goto usage;
        }
    }
    if (argc - util_optind != 0)
        goto usage;
    new_network = cluster_under_constraint(*network, &options);
    if (new_network == NIL(network_t))
        return 1;
    *network = new_network;
    return 0;
    usage:
    (void) fprintf(miserr, "usage: reduce_depth [-S n | -s n | -f n | -d n | -b | "
                           "-g] [-R n.n] [-r] [-v n]\n");
    (void) fprintf(miserr,
                   "     -S n=max_cluster_size\tmaximum value of cluster_size; "
                   "pick up the smallest one yielding the same depth\n");
    (void) fprintf(miserr, "     -s n=cluster_size\tmaximum number of nodes in a "
                           "cluster (Default = 8)\n");
    (void) fprintf(
            miserr,
            "     -f n=fanin\tmaximum number of inputs (fanins) for a cluster\n");
    (void) fprintf(
            miserr, "     -d n=max_level\tmaximum level of nodes in final network\n");
    (void) fprintf(miserr, "     -b\tuses the best cluster size for dup ratio "
                           "less than specified with -R\n");
    (void) fprintf(miserr, "     -g\tprints out statistics on a cluster size "
                           "basis; no clustering done\n");
    (void) fprintf(miserr, "     -R n.n\tspecifies max dup ratio; default 2.0\n");
    (void) fprintf(miserr, "     -r\trelabeling to reduce logic duplication "
                           "without increasing # levels\n");
    (void) fprintf(miserr, "     -v n=verbosity_level\tdebug mode\n");
    return 1;
}

/* timeout command: set up an alarm and send an interrupt to the process */
/* when alarm is raised. With "-k" option, forces a fault and kill the process
 */

#include <signal.h>

static void timeout_interrupt() {
    int pid = getpid();

    fprintf(siserr, "time out has occurred: process %d interrupted\n", pid);
    kill(pid, SIGINT);
}

static void timeout_kill() {
    int pid = getpid();

    fprintf(siserr, "time out has occurred: process %d killed\n", pid);
    kill(pid, SIGKILL);
}

static void timeout_noop() {}

/* ARGSUSED */
static int com_timeout(network_t **network, int argc, char **argv) {
    int c;
    int timeout;
    int kill_process = 0;

    if (argc == 1) {
        (void) signal(SIGALRM, timeout_noop);
        return 0;
    }
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "t:k")) != EOF) {
        switch (c) {
            case 't':timeout = atoi(util_optarg);
                if (timeout <= 0 || timeout > 365 * 24 * 3600)
                    goto usage;
                break;
            case 'k':kill_process = 1;
                break;
            default:goto usage;
        }
    }
    if (kill_process) {
        (void) signal(SIGALRM, timeout_kill);
    } else {
        (void) signal(SIGALRM, timeout_interrupt);
    }
    (void) alarm(timeout);
    return 0;
    usage:
    (void) fprintf(siserr, "usage: timeout [-t time_limit (seconds)] [-k]\n");
    (void) fprintf(
            siserr,
            "     -k\t kill the process instead of coming back to the prompt\n");
    return 1;
}

/* ARGSUSED */

static int com_check_load_limit(network_t **network, int argc, char **argv) {
    int     i, j;
    lsGen   gen, gen2;
    node_t  *node;
    array_t *nodevec;
    int     index;
    node_t  *fanin, *fanout;
    double  load, max_load;

    foreach_node(*network, gen, node) {
        if (node->type == PRIMARY_OUTPUT)
            continue;
        if (node->type == PRIMARY_INPUT)
            continue;
#if defined(_IBMR2) || defined(__osf__)
        if (MAP(node) == NIL(alt_map_t)) {
#else
        if (MAP(node) == NIL(map_t)) {
#endif /* _IBMR2, __osf__ */
            (void) fprintf(siserr, "network not mapped\n");
            return 1;
        }
        /* needs to be modified to take into account the correct info */
        MAP(node)->load = 0.0;
    }

    /* write a piece of code that allocates what is necessary */
    nodevec = network_dfs(*network);
    for (i  = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t *, nodevec, i);
        if (node->type == PRIMARY_OUTPUT) {
            fanin = node_get_fanin(node, 0);
            /* need to be computed properly */
            if (fanin->type != PRIMARY_INPUT) {
                MAP(fanin)->load += 0.0;
            }
        } else {
            foreach_fanin(node, index, fanin) {
                if (fanin->type != PRIMARY_INPUT) {
                    MAP(fanin)->load +=
                            delay_get_load(MAP(node)->gate->delay_info[index]);
                }
            }
        }
    }
    for (i = 0; i < array_n(nodevec); i++) {
        node = array_fetch(node_t *, nodevec, i);
        if (node->type != INTERNAL)
            continue;
        if (node_num_fanin(node) == 0)
            continue; /* skip constants for now */
        max_load = delay_get_load_limit(MAP(node)->gate->delay_info[0]);
        if (MAP(node)->load > max_load) {
            (void) fprintf(
                    sisout,
                    "Load at node %s for gate %s exceeded: allowed %2.3f actual %2.3f\n",
                    node->name, MAP(node)->gate->name, max_load, MAP(node)->load);
        }
    }
    array_free(nodevec);

    /* check for PI load limit violations */
    foreach_primary_input(*network, gen, node) {
        load     = 0.0;
        foreach_fanout(node, gen2, fanout) {
            foreach_fanin(fanout, j, fanin) {
                if (fanin == node)
                    break;
            }
            if (node_type(fanout) != PRIMARY_OUTPUT) {
                assert(MAP(fanout)->gate != NIL(lib_gate_t));
                load += delay_get_load(MAP(fanout)->gate->delay_info[j]);
            } else {
                load += pipo_get_po_load(fanout);
            }
        }
        max_load = pipo_get_pi_load_limit(node);
        if (load > max_load) {
            (void) fprintf(sisout,
                           "Load at PI %s exceeded: allowed %2.3f actual %2.3f\n",
                           node_name(node), max_load, load);
        }
    }
    return 0;
}

init_map() {
    com_add_command("replace", com_replace, /* changes_network */ 1);
    com_add_command("reduce_depth", com_reduce_depth, /* changes_network */ 1);
    com_add_command("map", com_map, /* changes_network */ 1);
    com_add_command("_build_map", com_build_map, /* changes_network */ 1);
    com_add_command("_tree_map", com_tree_map, /* changes_network */ 1);
    com_add_command("_fanout_opt", com_fanout_opt, /* changes_network */ 1);
    com_add_command("fanout_alg", com_fanout_alg, /* changes_network */ 0);
    com_add_command("fanout_param", com_fanout_param, /* changes_network */ 0);
    com_add_command("read_library", com_read_library, /* changes_network */ 0);
    com_add_command("print_gate", com_print_gate, /* changes_network */ 0);
    com_add_command("print_library", com_print_library, /*changes_network*/ 0);
    com_add_command("print_map_stats", com_print_map_stats, 0);
    com_add_command("_dump_patterns", com_dump_patterns, /*changes_network*/ 0);
    com_add_command("_addinv", com_map_addinv, /* changes_network */ 1);
    com_add_command("_eat_buffer", com_eat_buffer, /* changes_network */ 1);
    com_add_command("_reminv", com_map_reminv, /* changes_network */ 1);
    com_add_command("_treesize", com_treesize, /* changes_network */ 0);
    com_add_command("_premap", com_premap, /* changes_network */ 1);
    com_add_command("_buffer", com_buffer, /* changes_network */ 1);
    com_add_command("timeout", com_timeout, /* changes_network */ 0);
    com_add_command("_check_load_limit", com_check_load_limit,
            /* changes_network */ 0);

    node_register_daemon(DAEMON_FREE, map_free);
    node_register_daemon(DAEMON_INVALID, map_invalid);
    node_register_daemon(DAEMON_DUP, map_dup);

#ifdef SIS
    /* store gate types in the library */
    /* used in library.c and prim.c */
    network_type_table = st_init_table(strcmp, st_strhash);
#endif /* SIS */
}

end_map() {
    int          i;
    fanout_alg_t *alg;
    st_generator *gen;
    char         *key, *value;

    if (lib_current_library != 0) {
        lib_free(lib_current_library);
    }
    if (current_fanout_alg != NIL(array_t)) {
        for (i = 0; i < array_n(current_fanout_alg); i++) {
            alg = array_fetch_p(fanout_alg_t, current_fanout_alg, i);
            if (alg->properties != NIL(array_t)) {
                array_free(alg->properties);
            }
        }
        array_free(current_fanout_alg);
    }

#ifdef SIS
    /* free gate types */
    st_foreach_item(network_type_table, gen, &key, &value) {
        FREE(key);
        FREE(value);
    }
    st_free_table(network_type_table);
#endif /* SIS */
}

#ifdef SIS

/* Sequential support */
static void map_latch_table_dup(network_t *old, network_t *new) {
    lsGen   gen;
    node_t  *po, *new_po;
    node_t  *pi, *new_pi;
    latch_t *latch, *old_latch;

    /* Allocations */
    if (new->latch_table == NIL(st_table)) {
        new->latch_table = st_init_table(st_ptrcmp, st_ptrhash);
    }
    if (new->latch == 0) {
        new->latch = lsCreate();
    }

    /*
     * Generate the po from the old network and add determine
     * the correspondence by name.  Then add to table.
     */
    foreach_latch(old, gen, old_latch) {
        po     = latch_get_input(old_latch);
        pi     = latch_get_output(old_latch);
        new_po = network_find_node(new, po->name);
        new_pi = network_find_node(new, pi->name);
        network_create_latch(new, &latch, new_po, new_pi);
        latch_set_initial_value(latch, latch_get_initial_value(old_latch));
        latch_set_current_value(latch, latch_get_current_value(old_latch));
        latch_set_type(latch, latch_get_type(old_latch));
        latch_set_gate(latch, lib_gate_of(node_get_fanin(new_po, 0)));
        latch_set_control(latch, map_copy_clock(new, old_latch));
    }
    new->stg = stg_dup(old->stg);
    network_clock_dup(old, new);
}

/* copy the nodes associated with clock
 * for gated clocks - easy (all the nodes are mapped
 * and thus present in the new_network)
 * for non-gated clocks - no internal node (need to
 * create a PI/PO pair in the new network)
 */
static node_t *map_copy_clock(network_t *new_network, latch_t *old_latch) {
    node_t *old_clock, *old_clock_fanin;
    node_t *new_clock, *new_clock_fanin;
    node_t **new_fanin;

    old_clock = latch_get_control(old_latch);
    if (old_clock == NIL(node_t))
        return NIL(node_t);
    new_clock = network_find_node(new_network, old_clock->name);
    if (new_clock != NIL(node_t))
        return new_clock;

    /* make sure that there exists a PI/PO pair in the old network */
    assert(node_type(old_clock) == PRIMARY_OUTPUT &&
           node_num_fanin(old_clock) == 1);
    old_clock_fanin = node_get_fanin(old_clock, 0);
    assert(node_type(old_clock_fanin) == PRIMARY_INPUT);

    /* check if clock fanin node already exists */
    new_clock_fanin = network_find_node(new_network, old_clock_fanin->name);
    if (new_clock_fanin == NIL(node_t)) {
        new_clock_fanin = node_dup(old_clock_fanin);
        network_add_primary_input(new_network, new_clock_fanin);
    }
    new_fanin       = ALLOC(node_t *, 1);
    new_fanin[0] = new_clock_fanin;
    new_clock = node_dup(old_clock);
    node_replace_internal(new_clock, new_fanin, 1, NIL(set_family_t));
    network_add_node(new_network, new_clock);
    return new_clock;
}

#endif
