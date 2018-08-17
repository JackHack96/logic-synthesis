
#include "sis.h"
#include "pld_int.h"

static void print_partition_usage();

FILE *BDNET_FILE;
int  ACT_DEBUG;
int  ACT_STATISTICS;
int  XLN_DEBUG = 0, XLN_BEST = 0;
int  xln_use_best_subkernel;
int  act_is_or_used; /* = 1 if the OR gate is to be exploited */
int  MAXOPTIMAL;

/* 	traverse a DAG, executing the function manipuilate at each vertex
*/
void
traverse(v, manipulate)
        ACT_VERTEX_PTR v;
        void (*manipulate)();

{
    v->mark = (v->mark == 1) ? 0 : 1;
    (*manipulate)(v);
    if ((v->index) != v->index_size) {
        /* not a terminal vertex   */
        if ((v->mark) != v->low->mark) traverse(v->low, manipulate);
        if ((v->mark) != v->high->mark) traverse(v->high, manipulate);
    }
}

/* command line interpreter for the ULM_merge command 
		-Nishizaki*/
/* command line interpreter for the ULM_merge command 
		-Nishizaki*/
int
com_ULM_merge(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int     MAX_FANIN, MAX_COMMON_FANIN, MAX_UNION_FANIN, support;
    int     c;
    char    filename[BUFSIZE];
    int     fileflag, final_collapse_after_merge;
    int     verbose, use_lindo;
    array_t *match1_array, *match2_array;

    /* default parameters */
    /*--------------------*/
    MAX_FANIN                  = 4;
    MAX_COMMON_FANIN           = 4;
    MAX_UNION_FANIN            = 5;
    fileflag                   = 0;
    verbose                    = 0;
    use_lindo                  = 1;
    support                    = 5;
    final_collapse_after_merge = 1;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "f:c:n:u:o:Flv")) != EOF) {
        switch (c) {
            case 'f': MAX_FANIN = atoi(util_optarg);
                if (MAX_FANIN < 0) goto usage;
                break;
            case 'n': support = atoi(util_optarg);
                if (support < 2) goto usage;
                break;
            case 'c': MAX_COMMON_FANIN = atoi(util_optarg);
                if (MAX_COMMON_FANIN < 0) goto usage;
                break;
            case 'u': MAX_UNION_FANIN = atoi(util_optarg);
                if (MAX_UNION_FANIN < 0) goto usage;
                break;
            case 'o':(void) strcpy(filename, util_optarg);
                fileflag = 1;
                break;
            case 'l': use_lindo = 0;
                break;
            case 'F': final_collapse_after_merge = 0;
                break;
            case 'v': verbose = 1;
                break;
            default: goto usage;
        }
    }
    if (argc - util_optind != 0) goto usage;
    if (fileflag == 0) goto usage;

    match1_array = array_alloc(node_t * , 0);
    match2_array = array_alloc(node_t * , 0);
    merge_node(*network, MAX_FANIN, MAX_COMMON_FANIN, MAX_UNION_FANIN, filename, verbose,
               use_lindo, match1_array, match2_array);
    if (final_collapse_after_merge) {
        if (verbose) (void) printf("checking for collapses after merge...\n");
        st_free_table(xln_collapse_nodes_after_merge(*network, match1_array, match2_array, support, verbose));
    }
    array_free(match1_array);
    array_free(match2_array);
    return 0;

    usage:
    (void) fprintf(siserr,
                   "usage: xl_merge [-f MAX_FANIN] [-c MAX_COMMON_FANIN] [-u MAX_UNION_FANIN] [-n support] [-o filename] [-vlF]\n");
    (void) fprintf(siserr,
                   "    -f \t\tMAX_FANIN is the limit on the fanin of a mergeable node(default = 4)\n");
    (void) fprintf(siserr,
                   "    -c \t\tMAX_COMMON_FANIN is the limit on the common fanins of two mergeable nodes(default = 4)\n");
    (void) fprintf(siserr,
                   "    -u \t\tMAX_UNION_FANIN is the limit on the union of the fanins of two mergeable nodes(default = 5)\n");
    (void) fprintf(siserr, "    -n \t\tsupport\tfanin limit of nodes. (default = 5)\n");
    (void) fprintf(siserr,
                   "    -o \t\tfilename is the file in which information about the nodes merged is printed. Must specify.\n");
    (void) fprintf(siserr,
                   "    -l \t\tdo not use lindo, use a heuristic to solve max. card. matching.\n");
    (void) fprintf(siserr,
                   "    -F \t\tdo not do further collapsing after matching pairs of nodes (default: do it).\n");
    (void) fprintf(siserr,
                   "    -v \t\tverbosity option.\n");
    return 1;


}


/*command line interpreter for the ULM_merge command */
int
com_ULM_cover(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int bincov_heuristics; /* if 0, exact binate covering
				      if 1, Luciano's heuristic
				      if 2, Rajeev's heuristic - fast
					    and use for large networks. 
				      if 3, automatically chooses 0 for a 
					    small cover matrix, else chooses 2 */
    int c;
    int n;  /* added by Rajeev Dec. 27, 1989 - parameter to
	      	       partition_network routine -> instead of 5 */
    int cover_node_limit_exact;
    int cover_node_limit_heur_upper;

    /* default */
    /*---------*/
    cover_node_limit_exact = 30;
    cover_node_limit_heur_upper = 200;
    bincov_heuristics = 3;
    n                 = 5;
    XLN_DEBUG         = 0;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "n:h:e:u:d")) != EOF) {
        switch (c) {
            case 'n': n = atoi(util_optarg);
                break;
            case 'e': cover_node_limit_exact = atoi(util_optarg);
                break;
            case 'u': cover_node_limit_heur_upper = atoi(util_optarg);
                break;
            case 'h': bincov_heuristics = atoi(util_optarg);
                break;
            case 'd': XLN_DEBUG = 1;
                break;

            default: goto usage;
        }
    }
    if (argc - util_optind != 0) goto usage;

    if (bincov_heuristics == 3) {
        if (network_num_internal(*network) <= cover_node_limit_exact) {
            partition_network(*network, n, 0);
        } else {
            if (network_num_internal(*network) <= cover_node_limit_heur_upper) {
                partition_network(*network, n, 3);
            }
        }
    } else {
        partition_network(*network, n, bincov_heuristics);
    }
    return 0;

    usage:
    (void) fprintf(siserr,
                   "usage: xl_cover [-n number] [-h heuristic] [-e cover_node_limit_exact] [-u cover_node_limit_heur]\n");
    (void) fprintf(siserr, "-n \t\tlimit on the fanin\n");
    (void) fprintf(siserr, "-h 0,1,2,3 \tuses heuristics for binate covering problem\n");
    (void) fprintf(siserr,
                   "   -  0(exact), 1 (Luciano's heuristic), 2 (greedy - for large examples),\n\t 3(default)(automatically switches heuristic number depending on e and u values)\n");
    (void) fprintf(siserr,
                   "-e \t\tif h = 3, applies exact cover if number of nodes < number (Default 30)\n");
    (void) fprintf(siserr,
                   "-u \t\tif h = 3, applies heur 2 only if number of nodes < number (Default 200)\n");
    return 1;


}

/* This a routine that helps find the number of nodes with fanins greater 
 * than the value 'value'. This is used mainly to keep tabs on the fanin
 * restriction of the nodes in the network*/
int
com__node_value(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    lsGen  gen;
    node_t *node;
    int    c, count, value;

    util_getopt_reset();
    count = 0;

    value     = -1;
    while ((c = util_getopt(argc, argv, "v:")) != EOF) {
        switch (c) {
            case 'v': value = atoi(util_optarg);
                break;
            default: (void) fprintf(sisout, "usage: _xl_node_value -v \n");
                (void) fprintf(sisout, "-v should be followed by the fanin limit for the node\n");
                return 1;
        }
    }
    if (value < 1) return 1;


    foreach_node(*network, gen, node)
    {
        if (node_num_fanin(node) >= value) {
            count++;
            (void) fprintf(sisout, "%s:%d  ", node_name(node),
                           node_num_fanin(node));
        }
    }
    (void) fprintf(sisout, "\n%d nodes with fanin >= %d\n", count, value);
    return 0;
}

/* command line interpreter for the partition network command 
 * This is a routine that guarantees that the network will be 
 * partitioned into nodes with the fanin less than the value 
 * specified. This may be over kill at times! However this rou-
 * tine in very parochial so to speak as it looks only for 
 * optimal parent child relationships and DOES not look at the 
 * whole network.*/
int
com_partition(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int support               = 5;
    int c;
    int trivial_collapse_only = 0; /* does just a trivial collapse without creating infeasibilities*/
    int MOVE_FANINS           = 0;
    int MAX_FANINS            = 15;

    util_getopt_reset();
    XLN_DEBUG = 0;
    while ((c = util_getopt(argc, argv, "M:n:v:mt")) != EOF) {
        switch (c) {
            case 'm': MOVE_FANINS = 1;
                break;
            case 'M': MAX_FANINS = atoi(util_optarg);
                if (MAX_FANINS > 31) {
                    (void) printf("**warning, MAX_FANINS > 31, making it 31\n");
                    MAX_FANINS = 31;
                }
                break;
            case 'n': support = atoi(util_optarg);
                break;
            case 't': trivial_collapse_only = 1;
                break;
            case 'v':XLN_DEBUG = atoi(util_optarg);
                break;
            default: print_partition_usage();
                return 0;
        }
    }
    if (support <= 1) {
        (void) fprintf(sisout, "Error, the partition parameter must be > 1\n");
        print_partition_usage();
        return 0;
    }
    if (trivial_collapse_only) {
        (void) xln_do_trivial_collapse_network(*network, support, MOVE_FANINS,
                                               MAX_FANINS);
        return 0;
    }
    imp_part_network(*network, support, MOVE_FANINS, MAX_FANINS);
    return 0;
}

/* This routine is used to count the number of wires in the network. It
 * can be used as an indication of the routing complexity of the present
 * network. Hope to implement this in a good feedback loop for the system.
 * Currently used to keep track of wirability !*/

int
com__count_nets(network)
        network_t **network;
{
    int net_number;

    util_getopt_reset();
    net_number = estimate_net_no(*network);
    (void) fprintf(sisout, "\n the number of nets is %d \n", net_number);
    return 0;
}


/* This routine is used for an upper bound on the clbs for the current network.
 * This does an and or decomposition and then partitions the network. It
 * hoped that this estimate is good enough . DOes not change network*/

com__estimate_clb(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
int upper_bound, value;
if (argc != 2) {
(void)
fprintf(sisout,
"Error in commamnd\n ");
(void)
fprintf(sisout,
"usage: _xl_clb n , n = maximum # of inputs                                             to a CLB \n");
return 1;
}
value       = atoi(argv[argc - 1]);
upper_bound = estimate_clb_no(*network, value);
(void)
fprintf(sisout,
"estimate is %d\n", upper_bound);
return 0;

}

/* karp decomposer*/
com_karp_decomp(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
int              c;
int              support; /* cardinality of the bound set */
node_t           *node;
int              just_one_node;
xln_init_param_t *init_param;
int              MAX_FANINS_K_DECOMP;
int              EXHAUSTIVE;
int              RECURSIVE;
int              DESPERATE;

/* initialize */
/*------------*/
just_one_node       = 0;
node                = NIL(node_t);
support             = 5;
MAX_FANINS_K_DECOMP = 7;
RECURSIVE           = 0;
DESPERATE           = 1;
EXHAUSTIVE          = 0;
XLN_DEBUG           = 0;

util_getopt_reset();

while ((
c = util_getopt(argc, argv, "n:f:p:v:der")
) != EOF){
switch (c){
case 'n':
support = atoi(util_optarg);
if (support < 2) {
(void)printf("Error: xl_k_decomp: violation*** 2 <=number: exiting \n");
return 1;
}
break;
case 'p':
node = network_find_node(*network, util_optarg);
if (node ==
NIL (node_t)
) {
(void)printf("Error: xl_k_decomp: node %s not found, try altname of the node\n",
util_optarg);
return 1;
}
if (node->type == PRIMARY_INPUT) {
(void)printf("Error: xl_k_decomp: node %s PI\n", util_optarg);
return 1;
}
if (node->type == PRIMARY_OUTPUT)
node          = node_get_fanin(node, 0);
just_one_node = 1;
break;
case 'f':
MAX_FANINS_K_DECOMP = atoi(util_optarg);
break;
case 'v':
XLN_DEBUG = atoi(util_optarg);
break;
case 'r':
RECURSIVE = 1;
break;
case 'e':
EXHAUSTIVE = 1;
break;
case 'd':
DESPERATE = 0;
break;
default:
(void)
fprintf(sisout,
"usage: xl_k_decomp [-n support] [-p nodename] [-v verbosity_level] [-f MAX_FANINS_K_DECOMP] [-der]\n");
(void)
fprintf(sisout,
"-n\tsupport\tCardinality of bound set.\n");
(void)
fprintf(sisout,
"-p\tnodename Node to decompose.\n");
(void)
fprintf(sisout,
"-d\tif k_decomp fails, does not decomp node by cube-packing\n");
(void)
fprintf(sisout,
"-r\trecursively decompose the node\n");
(void)
fprintf(sisout,
"-e\tdo an exhaustive decomposition of the node\n");
(void)
fprintf(sisout,
"-f\tMAX_FANINS_K_DECOMP\t use exhaustive k_decomp if number of fanins\n");
(void)
fprintf(sisout,
"\tno more than this (default 7) and -e option was specified.\n");
return 1;
}
}
assert(!(EXHAUSTIVE && just_one_node));
init_param = ALLOC(xln_init_param_t, 1);
init_param->
support = support;
init_param->
MAX_FANINS_K_DECOMP = MAX_FANINS_K_DECOMP;
init_param->
RECURSIVE = RECURSIVE;
init_param->
DESPERATE = DESPERATE;
if (EXHAUSTIVE) {
xln_exhaustive_k_decomp_network(*network, init_param
);
} else {
(void)
karp_decomp_network(*network, support, just_one_node, node
);
}
FREE(init_param);
return 0;
}

/* And or map only : takes the network and does the and or decomp and
 * partitions the network. A rather crude mapping of the network*/

com_and_or_map(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
int size;

if (argc != 2) {
(void)
fprintf(sisout,
"Error in command\n");
(void)
fprintf(sisout,
"usage: _xl_do_clb n , n = # fanin to a CLB\n");
return 1;
}
size = atoi(argv[argc - 1]);
(void)
and_or_map(*network, size
);
return 0;
}

/* This routine scans the network for nodes larger than size and splits them*/
com_divide_network(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
int size      = 5, c;

util_getopt_reset();

    XLN_DEBUG = 0;

while ((
c = util_getopt(argc, argv, "n:d")
) != EOF){
switch (c) {
case 'n':
size = atoi(util_optarg);
break;
case 'd':
XLN_DEBUG                        = 1;
break;
default:
(void)
fprintf(sisout,
"usage: xl_split -n -d \n");
(void)
fprintf(sisout,
"\t-n upper limit on number of fanins (Default = 5)\n");
(void)
fprintf(sisout,
"\t-d turns debug on\n");
return 0;
}
}
if (size == 0){
(void)
fprintf(sisout,
"Error, split parameter must be greater than zero\n");
(void)
fprintf(sisout,
"usage: xl_split -n -d \n");
(void)
fprintf(sisout,
"\t-n upper limit on number of fanins (Default = 5)\n");
(void)
fprintf(sisout,
"\t-d turns debug on\n");
return 0;
}
(void)
split_network(*network, size
);
return 0;
}

static void
print_partition_usage() {
    (void) fprintf(siserr, "usage: xl_partition [-n support] [-m (MOVE_FANINS)] [-M MAX_FANINS] [-t]\n");
    (void) fprintf(siserr, "n - #of inputs to each partition\n");
    (void) fprintf(siserr, "m - moves fanins of the node if collapsing is not feasible\n");
    (void) fprintf(siserr,
                   "M - if m option used, moves fanins only if number of fanins at most the number (Default:15)\n");
    (void) fprintf(siserr, "t - collapse a node into all fanouts, if all of them remain feasible, delete node\n");
}


com_act_main(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
st_table *cost_table;
int      c;

int   NUM_ITER;       /* maximum num of iterations before stopping - default 0*/
float GAIN_FACTOR; /* go to next iteration only if the
                        gain > GAIN_FACTOR * network_cost - default 0.01*/
int   FANIN_COLLAPSE;/* only collapse those nodes whose fanin
                        is <= FANIN_COLLAPSE - default 3*/
int   DECOMP_FANIN;  /* decomp nodes with fanin >= this - default 4*/
int   QUICK_PHASE;   /* used if number of iterations = 0, and you
                        do/do not wish to perform QUICK_PHASE.
			default= 0 - it will not be performed */
int   LAST_GASP;     /* if non-zero, all the nodes in the network will be remapped */
int   BREAK;    /* break the network such that each node realized by one basic
                        block; create a bdnet like file */
int   DISJOINT_DECOMP; /* do a disjoint decomposition of the network */
int   HEURISTIC_NUM; /* chooses the heuristic for building the
                        ACT. If = 1, constructs optimum ACT for nodes
                        less than 6 inputs; = 2 constructs "ACT"
                        using Narendra's heuristic (no longer ACT - 
                        cofactor tree), = 3 a combination of the two, = 4 contructs both 
                        subject graphs for each node, pick the one with lower cost.*/
float mode;         /* = 0.0 for AREA mode, 1.0 for DELAY mode, in between: weighted
                         sum */

char             filename[BUFSIZE]; /* bdnet file name */
int              fileflag;
act_init_param_t *init_param;

util_getopt_reset();

                 init_param      = ALLOC(act_init_param_t, 1);
/* default settings */
/*------------------*/
                 NUM_ITER        = 0;
                 GAIN_FACTOR     = 0.01;
                 FANIN_COLLAPSE  = 3;
                 DECOMP_FANIN    = 4;
                 QUICK_PHASE     = 0;
                 BREAK           = 0;
                 LAST_GASP       = 0;
                 DISJOINT_DECOMP = 0;
                 HEURISTIC_NUM   = 2;
                 ACT_STATISTICS  = 0;
                 ACT_DEBUG       = 0;
                 MAXOPTIMAL      = 6;
                 mode            = (float) AREA;
                 fileflag        = 0;
                 act_is_or_used  = 1;


/* input handler */
/*---------------*/
util_getopt_reset();

while ((
c = util_getopt(argc, argv, "M:h:n:f:d:g:r:m:i:oqDlsv")
) != EOF) {
switch (c) {
case 'M':
MAXOPTIMAL = atoi(util_optarg);
break;
case 'n':
NUM_ITER = atoi(util_optarg);
if (NUM_ITER < 0) {
(void) printf("num_iteration(%d) should be > 0\n", NUM_ITER);
return 1;
}
break;
case 'h':
HEURISTIC_NUM = atoi(util_optarg);
if ((HEURISTIC_NUM < 1) || (HEURISTIC_NUM > 4)) {
(void) printf("heuristic_num(%d) should be from 1 to 4\n",
HEURISTIC_NUM);
return 1;
}
break;
case 'f':
FANIN_COLLAPSE = atoi(util_optarg);
if (FANIN_COLLAPSE < 0) {
(void) printf("collapse_fanin_limit(%d) should be > 0\n", FANIN_COLLAPSE);
return 1;
}
break;
case 'd':
DECOMP_FANIN = atoi(util_optarg);
if (DECOMP_FANIN < 1) {
(void) printf("decomp_fanin(%d) should be > 0\n", DECOMP_FANIN);
return 1;
}
break;
case 'g':
GAIN_FACTOR = atof(util_optarg);
if ((GAIN_FACTOR < 0) || (GAIN_FACTOR > 1)) {
(void) printf("gain_factor(%f) should be from 0.0 to 1.0\n", GAIN_FACTOR);
return 1;
}
break;
case 'r':
BREAK = 1;
(void)
strcpy(filename, util_optarg
);
BDNET_FILE = fopen(filename, "w");
if (BDNET_FILE == NULL) {
(void) printf(" **error** file %s cannot be opened\n", filename);
return 1;
}
break;
case 'm':
mode = atof(util_optarg);
if ((mode > 1.0) || (mode < 0.0)) {
(void) printf("mode (%f) should be between 0.0 (AREA) and 1.0 (DELAY)\n", mode);
return 1;
}
break;
case 'i':
fileflag = 1;
(void)
strcpy(init_param
->delayfile, util_optarg);
break;
case 'q':
QUICK_PHASE = 1;
break;
case 'o':
act_is_or_used = 0;
break;
case 'l':
LAST_GASP = 1;
break;
case 'D':
DISJOINT_DECOMP = 1;
break;
case 's':
ACT_STATISTICS = 1;
break;
case 'v':
ACT_DEBUG      = 1;
ACT_STATISTICS = 1;  /* if ACT_DEBUG is 1, make ACT_STATISTICS also 1 */
break;
default :
goto
usage;

} /* switch c */
} /* while */

init_param->
HEURISTIC_NUM = HEURISTIC_NUM;
init_param->
NUM_ITER = NUM_ITER;
init_param->
FANIN_COLLAPSE = FANIN_COLLAPSE;
init_param->
GAIN_FACTOR = GAIN_FACTOR;
init_param->
DECOMP_FANIN = DECOMP_FANIN;
init_param->
DISJOINT_DECOMP = DISJOINT_DECOMP;
init_param->
QUICK_PHASE = QUICK_PHASE;
init_param->
LAST_GASP = LAST_GASP;
init_param->
BREAK = BREAK;
init_param->
mode       = mode;

if ((fileflag == 0) && (mode != 0.0)) goto
usage;
cost_table = st_init_table(strcmp, st_strhash);

*
network = act_map_network(*network, init_param, cost_table);
(void)
free_cost_table(cost_table);
st_free_table(cost_table);
if (!
network_check(*network)
) {
(void) printf("com_act_main():%s\n",

error_string()

);
exit(1);
}
if (init_param->BREAK) {
(void)
fclose(BDNET_FILE);
}
FREE(init_param);
return 0;

usage:
(void) printf(" usage: act_map [-h heuristic_num] [-m mode] [-n num_iteration] [-f collapse_fanin] [-M MAXOPTIMAL]\n");
(void) printf("	 \t\t[-g gain_factor] [-d decomp_fanin] [-r filename] [-i delayfile] [-qolDsv]\n");
(void) printf("     -h\theuristic_num\tFor making subject-graphs (default = 2).\n");
(void) printf("     \t\t\t\t1=>ROBDD, 2 =>BDD (cofactor-tree)\n");
(void) printf("     \t\t\t\t3=>program decides, 4 => both 1 and 2\n");
(void) printf("     -m\tmode\tChoose any real number between AREA (0.0) and DELAY (1.0) (default = 0.0).\n");
(void) printf("     -n\tnum_iteration\tNumber of iterations (default = 0).\n");
(void) printf("     -f\tcollapse_fanin\tUpper bound on fanin for collapse (default = 3).\n");
(void) printf("     -M\tMAXOPTIMAL\tall possible orderings tried for ROBDD if number of fanins of the node <= MAXOPTIMAL (default = 6).\n");
(void) printf("     -d\tdecomp_fanin\tLower bound on fanin for decomposition (default = 4).\n");
(void) printf("     -g\tgain_factor\tCriterion for going to next iteration (default = 0.01).\n");
(void) printf("     -r\tfilename\tFinal restructuring to be done and bdnet-like file.\n");
(void) printf("     \t\t\t       filename to be created. \n");
(void) printf("     -i\tfilename\tdelayfile is the name of the file with info. about delay of\n");
(void) printf("     \t\t\t       a basic block for different fanouts. if mode != 0.0, must specify. \n");
(void) printf("     -q\t\t\tQuick phase run before exiting.\n");
(void) printf("     -o\t\t\tignore OR gate in the basic block.\n");
(void) printf("     -l\t\t\tLast gasp: final shot at improving results.\n");
(void) printf("     -D\t\t\tDisjoint decomposition done in the beginning.\n");
(void) printf("     -s\t\t\tStatistics option.\n");
(void) printf("     -v\t\t\tVerbosity option.\n");
FREE(init_param);
return 1;
}

com_xln_ao(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
int support, c;

/* default */
/*---------*/
support   = 5;
XLN_DEBUG = 0;

util_getopt_reset();

while ((
c = util_getopt(argc, argv, "n:v:")
) != EOF){
switch (c){
case 'n':
support = atoi(util_optarg);
if (support < 2) {
(void)printf("Error: xl_ao: violation*** 2 <=number: exiting \n");
return 1;
}
break;
case 'v':
XLN_DEBUG = atoi(util_optarg);
break;
default:
(void)
fprintf(sisout,
"usage: xl_ao [-n number] [-v verbosity level]\n");
(void)
fprintf(sisout,
"-n\tnumber\tfanin limit of nodes.\n");
(void)
fprintf(sisout,
"-v\tverbosity level.\n");
return 1;
}
}
xln_network_ao_map(*network, support
);
return 0;
}

com_xln_improve(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
int              support, cover_node_limit, lit_bound, c;
int              flag_decomp_good, good_or_fast, absorb;
int              MOVE_FANINS, MAX_FANINS;
int              RECURSIVE, DESPERATE, MAX_FANINS_K_DECOMP;
xln_init_param_t *init_param;

/* default */
/*---------*/
support             = 5;
cover_node_limit    = 25;
lit_bound           = 50;
flag_decomp_good    = 0;
good_or_fast        = GOOD;
absorb              = 1;
RECURSIVE           = 0;
DESPERATE           = 1;
MAX_FANINS_K_DECOMP = 7;
MOVE_FANINS         = 0;
MAX_FANINS          = 15;
XLN_DEBUG           = 0;
XLN_BEST            = 0;

util_getopt_reset();

while ((
c = util_getopt(argc, argv, "M:n:c:f:g:l:v:Aabdmr")
) != EOF){
switch (c){
case 'n':
support = atoi(util_optarg);
if (support < 2) {
(void)printf("Error: xl_imp: violation*** 2 <=number: exiting \n");
return 1;
}
break;
case 'm':
MOVE_FANINS = 1;
break;
case 'M':
MAX_FANINS = atoi(util_optarg);
if (MAX_FANINS > 31) {
(void) printf("**warning, MAX_FANINS > 31, making it 31\n");
MAX_FANINS = 31;
}
break;
case 'a':
good_or_fast = FAST;
break;
case 'A':
absorb = 0;
break;
case 'c':
cover_node_limit = atoi(util_optarg);
break;
case 'l':
lit_bound = atoi(util_optarg);
break;
case 'v':
XLN_DEBUG = atoi(util_optarg);
break;
case 'b':
XLN_BEST = 1;
break;
case 'g':
flag_decomp_good = atoi(util_optarg);
break;
case 'd':
DESPERATE = 0;
break;
case 'r':
RECURSIVE = 1;
break;
case 'f':
MAX_FANINS_K_DECOMP = atoi(util_optarg);
break;
default:
(void)
fprintf(sisout,
"usage: xl_imp [-n number] [-c cover_node_limit] [-l lit_bound]\n");
(void)
fprintf(sisout,
"\t [-g flag_decomp_good][-r (REC)] [-f MAX_FANINS_K_DECOMP]\n");
(void)
fprintf(sisout,
"\t [-d(DESP)][-v verbosity level] [-A(don't move fanins after decomp -g)]\n");
(void)
fprintf(sisout,
"\t [-a(don't try all decompositions)] [-m (MOVE_FANINS)][-M MAX_FANINS]\n");
(void)
fprintf(sisout,
"-n\tnumber\tfanin limit of nodes.\n");
(void)
fprintf(sisout,
"-c\tcover_node_limit exact cover of node till these many nodes.\n");
(void)
fprintf(sisout,
"-l\tnum_literals > lit_bound => good decompose the node.\n");
(void)
fprintf(siserr,
"-m\tmoves fanins of the node if collapsing is not feasible\n");
(void)
fprintf(siserr,
"-M\tif m option used, moves fanins only if number of fanins at most the number (Default:15)\n");
(void)
fprintf(sisout,
"-v\tverbosity level.\n");
return 1;
}
}

init_param = ALLOC(xln_init_param_t, 1);
init_param->
support = support;
init_param->
cover_node_limit = cover_node_limit;
init_param->
lit_bound = lit_bound;
init_param->
flag_decomp_good = flag_decomp_good;
init_param->
good_or_fast = good_or_fast;
init_param->
absorb = absorb;
init_param->
DESPERATE = DESPERATE;
init_param->
RECURSIVE = RECURSIVE;
init_param->
MAX_FANINS_K_DECOMP = MAX_FANINS_K_DECOMP;
init_param->xln_move_struct.
MOVE_FANINS = MOVE_FANINS;
init_param->xln_move_struct.
MAX_FANINS = MAX_FANINS;
xln_improve_network(*network, init_param
);
FREE(init_param);
return 0;
}

com_xln_reduce_levels(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
int              support, heur, c;
int              MAX_FANINS, MOVE_FANINS, bound_alphas;
int              collapse_input_limit;
int              traversal_method;  /* 1 then topological traversal, else levels sorted wrt width */
xln_init_param_t *init_param;

/* default */
/*---------*/
support              = 5;
XLN_DEBUG            = 0;
heur                 = 1;
MAX_FANINS           = 15;
MOVE_FANINS          = 1;
bound_alphas         = 1;
collapse_input_limit = 10;
traversal_method     = 1;

util_getopt_reset();

while ((
c = util_getopt(argc, argv, "c:n:v:h:M:A:mt")
) != EOF){
switch (c){
case 'n':
support = atoi(util_optarg);
if (support < 2) {
(void)printf("Error: xl_rl: violation*** 2 <=number: exiting \n");
return 1;
}
break;
case 'v':
XLN_DEBUG = atoi(util_optarg);
break;
case 'h':
heur = atoi(util_optarg);
break;
case 'c':
collapse_input_limit = atoi(util_optarg);
break;
case 'M':
MAX_FANINS = atoi(util_optarg);
break;
case 'A':
bound_alphas = atoi(util_optarg);
break;
case 'm':
MOVE_FANINS = 0;
break;
case 't':
traversal_method = 0;
break;
default:
(void)
fprintf(sisout,
"usage:xl_rl [-n number] [-v verbosity level]\n");
(void)
fprintf(sisout,
"\t[-m (DONT_MOVE_FANINS)] [-M MAX_FANINS] [-h heur]\n");
(void)
fprintf(sisout,
"\t[-t (trav_method-levels)] [-c collapse_input_limit]\n");
(void)
fprintf(sisout,
"-n\tnumber\tfanin limit of nodes.\n");
(void)
fprintf(sisout,
"-h\theur\t1=>collapse into any fanout, 2=> collapse into critical fanouts\n");
(void)
fprintf(sisout,
"-c\tnumber\tconsider collapsing of network if PI's less (def = 10).\n");
(void)
fprintf(siserr,
"-m\tmoves fanins of the node if collapsing is not feasible\n");
(void)
fprintf(siserr,
"-M\tif m option used, moves fanins only if number of fanins at most the number (Default:15\n");
(void)
fprintf(siserr,
"-t\ttraverse the network by levels instead of topologically\n");
(void)
fprintf(sisout,
"-v\tverbosity level.\n");
return 1;
}
}
init_param = ALLOC(xln_init_param_t, 1);
init_param->
support = support;
init_param->
heuristic = heur; /* overuse of heuristic */
init_param->
collapse_input_limit = collapse_input_limit;
init_param->
traversal_method = traversal_method;
init_param->xln_move_struct.
MOVE_FANINS = MOVE_FANINS;
init_param->xln_move_struct.
MAX_FANINS = MAX_FANINS;
init_param->xln_move_struct.
bound_alphas = bound_alphas;
assert(bound_alphas
== 1);
xln_reduce_levels(network, init_param
);
FREE(init_param);
return 0;
}

com_xln_partial_collapse(network, argc, argv
)
network_t **network;
int       argc;
char      **argv;
{
int              size, c;
int              COST_LIMIT, cover_node_limit, lit_bound, flag_decomp_good, good_or_fast;
int              MOVE_FANINS, MAX_FANINS;
int              absorb;
xln_init_param_t *init_param;

/* default */
/*---------*/
size             = 5;
COST_LIMIT       = 1;
cover_node_limit = 15;
lit_bound        = 50;
flag_decomp_good = 0;
good_or_fast     = FAST;
absorb           = 0;
MOVE_FANINS      = 0;
MAX_FANINS       = 15;
XLN_DEBUG        = 0;

util_getopt_reset();

while ((
c = util_getopt(argc, argv, "M:n:C:v:c:g:l:bamA")
) != EOF){
switch (c){
case 'n':
size = atoi(util_optarg);
if (size < 2) {
(void)printf("Error: xl_part_coll: violation*** 2 <=number: exiting \n");
return 1;
}
break;
case 'm':
MOVE_FANINS = 1;
break;
case 'M':
MAX_FANINS = atoi(util_optarg);
if (MAX_FANINS > 31) {
(void) printf("**warning, MAX_FANINS > 31, making it 31\n");
MAX_FANINS = 31;
}
break;
case 'v':
XLN_DEBUG = atoi(util_optarg);
break;
case 'b':
XLN_BEST = 1;
break;
case 'g':
flag_decomp_good = atoi(util_optarg);
break;
case 'a':
good_or_fast = 1;
break;
case 'A':
absorb = 1;
break;
case 'C':
COST_LIMIT = atoi(util_optarg);
break;
case 'c':
cover_node_limit = atoi(util_optarg);
break;
case 'l':
lit_bound = atoi(util_optarg);
break;
default:
(void)
fprintf(sisout,
"usage: xl_part_coll [-n number] [-C cost_limit] [-b]");
(void)
fprintf(sisout,
" [-c cover_node_limit] [-l lit_bound] [-v verbosity level]\n");
(void)
fprintf(sisout,
" [-g (flag_decomp_good)] [-a (apply all_decomp_schemes)]\n");
(void)
fprintf(sisout,
" [-A (move fanins after decomp -g)] [-m (MOVE_FANINS)] [-M (MAX_FANINS)]\n");
(void)
fprintf(sisout,
"-n\tnumber\tfanin limit of nodes.\n");
(void)
fprintf(sisout,
"-C\tcost_limit\tnode with higher cost not to be collapsed\n");
(void)
fprintf(siserr,
"-m\tmoves fanins of the node if collapsing is not feasible\n");
(void)
fprintf(siserr,
"-M\tif m option used, moves fanins only if number of fanins at most the number (Default:15\n");
(void)
fprintf(sisout,
"-c\tcover_node_limit exact cover of node till these many nodes.\n");
(void)
fprintf(sisout,
"-l\t num_literals > lit_bound => good decompose the node.\n");
(void)
fprintf(sisout,
"-v\tverbosity level.\n");
(void)
fprintf(sisout,
"-b\ttry best mapping: large node => recursively decomp\n");
return 1;
}
}

init_param = ALLOC(xln_init_param_t, 1);
init_param->
support = size;
init_param->
COST_LIMIT = COST_LIMIT; /* node has higher cost => do not collapse */
init_param->
cover_node_limit = cover_node_limit;
init_param->
lit_bound = lit_bound;
init_param->
good_or_fast = good_or_fast;
init_param->
absorb = absorb;
init_param->
flag_decomp_good = flag_decomp_good;
init_param->
RECURSIVE = 0;
init_param->
DESPERATE = 1;
init_param->
MAX_FANINS_K_DECOMP = 7;
init_param->xln_move_struct.
MOVE_FANINS = MOVE_FANINS;
init_param->xln_move_struct.
MAX_FANINS = MAX_FANINS;
xln_partial_collapse(*network, init_param
);
FREE(init_param);
return 0;
}

int
com_xln_decomp_for_merging_network(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int              c;
    xln_init_param_t *init_param;

    init_param = ALLOC(xln_init_param_t, 1);

    /* default parameters */
    /*--------------------*/
    init_param->MAX_FANIN                = 4;
    init_param->MAX_COMMON_FANIN         = 4;
    init_param->MAX_UNION_FANIN          = 5;
    init_param->support                  = 5;
    init_param->heuristic                = ALL_CUBES;
    init_param->common_lower_bound       = 2;
    init_param->cube_support_lower_bound = 4;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "f:c:n:u:h:l:L:v")) != EOF) {
        switch (c) {
            case 'f': init_param->MAX_FANIN = atoi(util_optarg);
                if (init_param->MAX_FANIN < 0) goto usage;
                break;
            case 'c': init_param->MAX_COMMON_FANIN = atoi(util_optarg);
                if (init_param->MAX_COMMON_FANIN < 0) goto usage;
                break;
            case 'u': init_param->MAX_UNION_FANIN = atoi(util_optarg);
                if (init_param->MAX_UNION_FANIN < 0) goto usage;
                break;
            case 'n': init_param->support = atoi(util_optarg);
                if (init_param->support < 0) goto usage;
                break;
            case 'h': init_param->heuristic = atoi(util_optarg);
                if ((init_param->heuristic < 1) || (init_param->heuristic > 2))
                    goto usage;
                break;
            case 'l': init_param->common_lower_bound = atoi(util_optarg);
                break;
            case 'L': init_param->cube_support_lower_bound = atoi(util_optarg);
                break;
            case 'v': XLN_DEBUG = 1;
                break;
            default: goto usage;
        }
    }
    if (argc - util_optind != 0) goto usage;

    xln_decomp_for_merging_network(*network, init_param);
    FREE(init_param);
    return 0;

    usage:
    (void) fprintf(siserr,
                   "usage: xl_decomp_two [-f MAX_FANIN] [-c MAX_COMMON_FANIN] [-u MAX_UNION_FANIN]\n");
    (void) fprintf(siserr,
                   "\t\t\t[-n support] [-h heuristic] [-l lower_common_bound]\n");
    (void) fprintf(siserr, "\t\t\t[-L cube_support_lower_bound] [-v]\n");
    (void) fprintf(siserr,
                   " -f\t\tMAX_FANIN is the limit on the fanin of a mergeable node(default = 4)\n");
    (void) fprintf(siserr,
                   " -c\t\tMAX_COMMON_FANIN is the limit on the common fanins of two mergeable nodes(default = 4)\n");
    (void) fprintf(siserr,
                   " -u\t\tMAX_UNION_FANIN is the limit on the union of the fanins of two mergeable nodes(default = 5)\n");
    (void) fprintf(sisout, "-n\tnumber\tfanin limit of nodes. (default = 5)\n");
    (void) fprintf(sisout, "-h\theuristic\t 1 = ALL_CUBES, 2 = PAIR_NODES. (default = 1)\n");
    (void) fprintf(sisout, "-l\tlower_common_bound\t(default = 2)\n");
    (void) fprintf(sisout, "-L\tcube-support lower bound\t(default = 4)\n");
    (void) fprintf(siserr, "-v\tverbosity option.\n");
    FREE(init_param);
    return 1;
}

int
com_xln_absorb_nodes_in_fanins(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int c;
    int support;
    int method;
    int MAX_FANINS;

    /* default parameters */
    /*--------------------*/
    support    = 5;
    XLN_DEBUG  = 0;
    method     = 1; /* move_fanin, not absorb */
    MAX_FANINS = 15; /* do not look at a node with higher fanins */

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "f:m:n:v")) != EOF) {
        switch (c) {
            case 'f': MAX_FANINS = atoi(util_optarg);
                if (MAX_FANINS > 31) {
                    (void) printf("**warning--- can't have MAX_FANINS > 31, making it 31\n");
                    MAX_FANINS = 31;
                }
                break;
            case 'm': method = atoi(util_optarg);
                if ((method > 1) || (method < 0)) goto usage;
                break;
            case 'n': support = atoi(util_optarg);
                if (support < 2) goto usage;
                break;
            case 'v': XLN_DEBUG = 1;
                break;
            default: goto usage;
        }
    }
    if (argc - util_optind != 0) goto usage;
    if (method) {
        xln_network_reduce_infeasibility_by_moving_fanins(*network, support, MAX_FANINS);
    } else {
        xln_absorb_nodes_in_fanins(*network, support);
    }
    return 0;

    usage:
    (void) fprintf(siserr, "xln_absorb \t[-n support] [-m method] [-f MAX_FANINS] [-v]\n");
    (void) fprintf(sisout, "-n\tsupport\tfanin limit of nodes. (default = 5)\n");
    (void) fprintf(sisout, "-f\tMAX_FANINS\t don't consider nodes above these fanins(default = 15)\n");
    (void) fprintf(sisout, "-m\tmethod\t0 => absorb, 1 => move fanins (default = 1)\n");
    (void) fprintf(siserr, "-v \tverbosity option.\n");
    return 1;
}

int
com_xln_collapse_check_area(network, argc, argv)
        network_t **network;
        int argc;
        char **argv;
{
    int              c;
    int              support;
    int              collapse_input_limit;
    int              roth_karp_flag;
    xln_init_param_t *init_param;

    /* default parameters */
    /*--------------------*/
    support              = 5;
    XLN_DEBUG            = 0;
    collapse_input_limit = 9;
    roth_karp_flag       = 1;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "c:n:kv")) != EOF) {
        switch (c) {
            case 'c':collapse_input_limit = atoi(util_optarg);
                break;
            case 'n': support = atoi(util_optarg);
                if (support < 2) goto usage;
                break;
            case 'v': XLN_DEBUG = 1;
                break;
            case 'k': roth_karp_flag = 0; /* do not do roth-karp */
                break;
            default: goto usage;
        }
    }
    if (argc - util_optind != 0) goto usage;
    init_param = ALLOC(xln_init_param_t, 1);
    init_param->support              = support;
    init_param->collapse_input_limit = collapse_input_limit;
    xln_collapse_check_area(network, init_param, roth_karp_flag);
    FREE(init_param);
    return 0;

    usage:
    (void) fprintf(siserr, "xl_coll_ck \t[-n support] [-c input_collapse_lim] [-k (don't roth)] [-v]\n");
    (void) fprintf(sisout, "-n\tsupport\tfanin limit of nodes. (default = 5)\n");
    (void) fprintf(sisout, "-c\tcollapse\t use cofactor and roth-karp(default = 9)\n");
    (void) fprintf(siserr, "-v \tverbosity option.\n");
    return 1;
}

/* init routines*/
init_pld() {
    extern int init_ite(), end_ite();

    com_add_command("act_map", com_act_main, 1);

    node_register_daemon(DAEMON_ALLOC, p_actAlloc);
    node_register_daemon(DAEMON_FREE, p_actFree);
    node_register_daemon(DAEMON_DUP, p_actDup);

    /* old xilinx commands */
    /*---------------------*/
    com_add_command("xl_partition", com_partition, 1);
    com_add_command("xl_merge", com_ULM_merge, 1);
    com_add_command("xl_cover", com_ULM_cover, 1);
    com_add_command("xl_k_decomp", com_karp_decomp, 1);
    com_add_command("xl_split", com_divide_network, 1);
    com_add_command("_xl_nodevalue", com__node_value, 0);
    com_add_command("_xl_cnets", com__count_nets, 0);
    com_add_command("_xl_clb", com__estimate_clb, 0);
    com_add_command("_xl_do_clb", com_and_or_map, 1);

    /* new xilinx commands */
    /*---------------------*/
    com_add_command("xl_imp", com_xln_improve, 1);
    com_add_command("xl_rl", com_xln_reduce_levels, 1);
    com_add_command("xl_ao", com_xln_ao, 1);
    com_add_command("xl_decomp_two", com_xln_decomp_for_merging_network, 1);
    com_add_command("xl_part_coll", com_xln_partial_collapse, 1);
    com_add_command("xl_absorb", com_xln_absorb_nodes_in_fanins, 1);
    com_add_command("xl_coll_ck", com_xln_collapse_check_area, 1);

    init_ite();
}

end_pld() {
    end_ite();
}

