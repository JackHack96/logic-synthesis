
#include "sis.h"
#include "speed_int.h"
#include "buffer_int.h"
#include <math.h>


/* definitions that were part of the original com_gbx.c */
int gbx_verbose_mode;
int print_bypass_mode;
int start_node_mode;	/* Take bypass with 0 gain */

static array_t *current_speedup_alg;

/*
 * Set the defaults for the speedup routines --- assume unmapped
 */
speed_set_default_options(speed_param)
speed_global_t *speed_param;
{
    if (current_speedup_alg == NIL(array_t)){
	current_speedup_alg = sp_get_local_trans(0, NIL(char *));
    }
    speed_param->local_trans = current_speedup_alg;
    speed_param->area_reclaim = TRUE;
    speed_param->trace = FALSE;
    speed_param->debug = FALSE;
    speed_param->add_inv = FALSE;
    speed_param->del_crit_cubes = TRUE;      /* Reduce delay at all cost */
    speed_param->num_tries = 1;
    speed_param->thresh = DEFAULT_SPEED_THRESH;
    speed_param->coeff = DEFAULT_SPEED_COEFF;
    speed_param->dist = DEFAULT_SPEED_DIST;
    speed_param->new_mode = 1;                /* Use the new_speed code */
    speed_param->region_flag = ALONG_CRIT_PATH;
    speed_param->transform_flag = BEST_BENEFIT;
    speed_param->max_recur = 1;
    speed_param->timeout = INFINITY;
    speed_param->max_num_cuts = 50;
    speed_param->red_removal = FALSE;
    speed_param->objective = AREA_BASED;
    speed_param->req_times_set = FALSE;
    
    /* 
     * By default do the script of varying distances
     * loop till improvement at a given distance
     */
    speed_param->do_script = TRUE;
    speed_param->speed_repeat = TRUE;
    speed_param->only_init_decomp = FALSE;

    /* Initialize the library delay data */
    speed_param->model = DELAY_MODEL_UNIT;
    speed_param->pin_cap = 0.0;
    speed_param->library_accl = 0;
}

int
speed_fill_options(speed_param, argc, argv)
speed_global_t *speed_param;
int argc;
char **argv;
{
    int c;
    double coeff;
    int num_tries, flag, dist, level, num_cuts;
    
    speed_set_default_options(speed_param);
    
    if (argc == 0 || argv == NIL(char *)) return 0;
    
    while ((c = util_getopt(argc, argv, "a:d:l:m:s:w:t:C:D:I:ABRTcnfipvr")) != EOF) {
        switch (c) {
        case 'f':
	    speed_param->new_mode = FALSE;
            break;
        case 'p':
	    speed_param->add_inv = TRUE;
            break;
        case 'r':
	    speed_param->area_reclaim = FALSE;
            break;
        case 'c':
            speed_param->speed_repeat = FALSE;
	    speed_param->do_script = FALSE;
            break;
        case 'i':
            speed_param->only_init_decomp = TRUE;
	    speed_param->do_script = FALSE;
            break;
        case 'A':
            speed_param->del_crit_cubes = FALSE;
            break;
        case 'B':
            speed_param->transform_flag =  BEST_BANG_FOR_BUCK;
            break;
        case 'I':
            speed_param->timeout =  atoi(util_optarg);
	    (void)fprintf(sisout, "INFO: optimization limit = %d seconds\n",
			  speed_param->timeout);
            break;
        case 'R':
            speed_param->red_removal = TRUE;
            break;
        case 'n':
            speed_param->objective = TRANSFORM_BASED;
            break;
        case 'T':
            speed_param->trace = TRUE;
            break;
        case 'v':
            speed_param->debug = TRUE;
            break;
        case 'D':
            speed_param->debug = atoi(util_optarg);
 	    if (speed_param->debug < 0) speed_param->debug = FALSE;
            break;
        case 's':
 	    if (strcmp(util_optarg, "crit") == 0) {
		speed_param->region_flag = ALONG_CRIT_PATH;
	    } else if (strcmp(util_optarg, "transitive") == 0){
		speed_param->region_flag = TRANSITIVE_FANIN;
	    } else if (strcmp(util_optarg, "compromise") == 0){
		speed_param->region_flag = COMPROMISE;
	    } else if (strcmp(util_optarg, "tree") == 0){
		speed_param->region_flag = ONLY_TREE;
	    } else {
		(void)fprintf(sisout, "Illegal argument to the -s flag\n");
		return 1;
	    }
            break;
       case 'w':
	    coeff = atof(util_optarg);
	    if (coeff < 0) coeff = 0;
	    if (coeff > 1) coeff = 1;
            speed_param->coeff = coeff;
            break;
        case 't':
            speed_param->thresh = atof(util_optarg);
	    speed_param->do_script = FALSE;
            break;
        case 'a':
            num_tries = atoi(util_optarg);
	    if (num_tries > 0) speed_param->num_tries = num_tries;
            break;
        case 'C':
            num_cuts = atoi(util_optarg);
	    if (num_cuts > 0) speed_param->max_num_cuts = num_cuts;
            break;
        case 'l':
            level = atoi(util_optarg);
	    if (level > 0) speed_param->max_recur = level;
            break;
        case 'd':
            dist = atoi(util_optarg);
	    if (dist >= 0) speed_param->dist = dist;
	    speed_param->do_script = FALSE;
            break;
        case 'm':
            speed_param->model = delay_get_model_from_name(util_optarg);
	    if (speed_param->model == DELAY_MODEL_LIBRARY){
		speed_param->model = DELAY_MODEL_MAPPED;
		if (speed_param->interactive){
		    (void)fprintf(sisout, "Using MAPPED model instead of LIBRARY\n");
		}
	    } else if (speed_param->model == DELAY_MODEL_UNKNOWN){
		(void)fprintf(siserr, "Unknown delay model %s\n", util_optarg);
		return 1;
	    }
            break;
        default:
	    (void)fprintf(siserr, "ERROR: Unknown option\n");
            return 1;
        }
    }
    return 0;
}

static void
speed_print_usage()
{
    (void) fprintf(siserr, "usage: speed_up [-cinprvABIRT] [-a n] [-d n] [-w n] [-s method] [-t n.n] [-l n] [-D n] [-m model] [node-list]\n");
    (void)fprintf(siserr, "The options -s, -a, -l, -C are ignored when -f is specified\n");
    (void)fprintf(sisout, "*********      General options       **********\n");
    (void) fprintf(siserr, "    -c\t\tOnly one pass of speed_up.\n");
    (void) fprintf(siserr, "    -d\tn\tDistance for collapsing (default = 3).\n");
    (void) fprintf(siserr, "    -f\t\tFast mode (heuristic weight computation).\n");
    (void) fprintf(siserr, "    -i\t\tOnly do the initial 2_input NAND decomp.\n");
    (void) fprintf(siserr, "    -v\t\tVerbose option (same as -D 1).\n");
    (void) fprintf(siserr, "    -p\t\tUse explicit inverters (EXPERIMENTAL).\n");
    (void) fprintf(siserr, "    -r\t\tDo not try to reclaim area (EXPERIMENTAL).\n");
    (void) fprintf(siserr, "    -A\t\tArea saving option: Do not delete critical cubes of divisors.\n");
    (void) fprintf(siserr, "    -R\t\tRun redundancy removal after each successful iteration.\n");
    (void) fprintf(siserr, "    -D\tn\tSet the debugging level.\n");
    (void) fprintf(siserr, "    -m\tmodel\tDelay model (default \"unit-fanout\").\n");
    (void) fprintf(siserr, "    -T\t\tPrint the delays as the speed_up proceeds.\n");
    (void)fprintf(sisout, "********* Options for fast mode, -f  **********\n");
    (void) fprintf(siserr, "    -t\tn.n\tCritical threshold.\n");
    (void) fprintf(siserr, "    -a\tn\tNumber of decomposition attempts (default = 1).\n");
    (void) fprintf(siserr, "    -w\tn\tRelative weight of area (default = 0).\n");
    (void)fprintf(sisout, "*********     Options for new mode   **********\n");
    (void) fprintf(siserr, "    -n\t\tChoose the fewest number of transformations\n");
    (void) fprintf(siserr, "    -B\t\tUse Benefit/cost as goodness of transform.\n");
    (void) fprintf(siserr, "    -I\tn\tTimeout limit for each optimization in seconds(default = INFINITY).\n");
    (void) fprintf(siserr, "    -l\tn\t Set the number of recursion levels (default = 1).\n");
    (void) fprintf(siserr, "    -s\tmethod\tMethod for selecting the region to transform.\n");
    (void) fprintf(siserr, "\t one of \"crit\"(default), \"transitive\", \"compromise\", \"tree\"\n");
    (void) fprintf(siserr, "    -C\tn\t Set the number of cutsets to generate (default = 50).\n");
}

/*
 * Routine that calls the local transformation code for reducing the depth
 * of Boolean networks. 
 */
int
com_speed_up(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int i;
    char *name, *new_name;
    double latest, new_latest, area, new_area;
    node_t *np;
    array_t *nodevec;
    speed_global_t speed_param;

    util_getopt_reset();
    error_init();

    speed_param.interactive = TRUE;
    if (speed_fill_options(&speed_param, argc, argv)){
	(void)fprintf(sisout,"%s", error_string());
	speed_print_usage();
	return 1;
    }

    /* First check if the network is a valid one */
    if (network_num_pi(*network) == 0) {
	/* Nothing needs to be done !!! --- there are only constants !!!*/
	return 0;
    }
    nodevec = com_get_nodes(*network, argc-util_optind+1, argv+util_optind-1);
    if (array_n(nodevec) == 0){
        array_free(nodevec);
	return 0;
    }

    /* If mapped model - make sure library is present */
    if (speed_param.model == DELAY_MODEL_MAPPED &&
	    (lib_get_library() == NIL(library_t))){
	(void) fprintf(sisout, "Need to define a library\n");
        array_free(nodevec);
	return 1;
    }
    /* Just warn of the potential of unmapping a mapped network */
    if (lib_network_is_mapped(*network) &&
	speed_param.model != DELAY_MODEL_MAPPED){ 
	(void)fprintf(sisout, "WARNING: Mapped network will be decomposed into 2-input gates\n");
    }
    /* Initialize the globals for the delay routines in this package */
    speed_set_delay_data(&speed_param, 0 /* No library acceleration */);
    speed_param.req_times_set = sp_po_req_times_set(*network);

    /* If a list of nodes is present - decomp only those */
    if (argc - util_optind != 0) {
        for (i = 0; i < array_n(nodevec); i++) {
            np = array_fetch(node_t *, nodevec, i);
            if (np->type == INTERNAL) {
		speed_up_node(*network, np, &speed_param, 0 /* delay values */);
            }
        }
    } else if (speed_param.do_script){
	if (speed_init_decomp(*network, &speed_param) ){
	    return 1;
	}
	speed_up_script(network, &speed_param);
    } else if (speed_param.only_init_decomp) {
	if (speed_init_decomp(*network, &speed_param) ){
	    return 1;
	}
    } else if (speed_param.speed_repeat) {
	if (speed_init_decomp(*network, &speed_param) ){
	    return 1;
	}
        speed_up_loop(network, &speed_param);
    } else {
	if (speed_init_decomp(*network, &speed_param) ){
	    return 1;
	}
        if (delay_trace(*network, speed_param.model) ){
	    SP_GET_PERFORMANCE(&speed_param, *network, np, latest, name, area);
	}
        if (speed_param.new_mode){
	    new_speed(*network, &speed_param);
	} else{
	    speed_up_network(*network, &speed_param);
	}
        if (delay_trace(*network, speed_param.model) ){
	    SP_GET_PERFORMANCE(&speed_param, *network, np, new_latest,
			       new_name, new_area);
        }	
	SP_PRINT(&speed_param, latest, new_latest, area, new_area,
		 name, new_name);
	FREE(name);
	FREE(new_name);
    }

    array_free(nodevec);
    return 0;
}

/*
 * Given the list of nodes, the routine will collapse the specified
 * distance of fanins along the critical path (specified by "-t")
 */
int
com__part_collapse(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int i;
    node_t *node;
    array_t *nodevec;
    speed_global_t speed_param;

    util_getopt_reset();
    error_init();

    speed_param.interactive = TRUE;
    if (speed_fill_options(&speed_param, argc, argv)){
	(void)fprintf(sisout,"%s", error_string());
	goto part_collapse_usage;
    }
    /* Initialize the globals for the delay routines in this package */
    speed_set_delay_data(&speed_param, 0 /* No library acceleration */);

    if (argc - util_optind != 0) {
	if (!delay_trace(*network, speed_param.model)){
	    (void)fprintf(sisout,"%s\n", error_string());
	    return 1;
	}
	set_speed_thresh(*network, &speed_param);
        nodevec = com_get_nodes(*network, argc-util_optind+1, argv+util_optind-1);
        for(i = 0; i < array_n(nodevec); i++){
            node = array_fetch(node_t *, nodevec, i);
            speed_absorb(node, &speed_param);
        }
        array_free(nodevec);
    }
    return 0;

part_collapse_usage:
    (void) fprintf(siserr, "usage: _part_collapse [-m model] [-d n] [-t n.n] list-of-nodes\n");
    (void) fprintf(siserr, "    -d\tn\tDistance for collapsing.\n");
    (void) fprintf(siserr, "    -t\tn.n\tCritical threshold.\n");
    (void) fprintf(siserr, "    -m\tmodel\tDelay model. default (\"unit-fanout\").\n");
    return 1;
}


/*
 * This routine reports the most favorable cutset found
 */
int
com__print_cutset(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    array_t *mincut;
    speed_global_t speed_param;
    st_table *table;

    util_getopt_reset();
    error_init();

    speed_param.interactive = TRUE;
    if (speed_fill_options(&speed_param, argc, argv)){
	(void)fprintf(sisout,"%s", error_string());
	goto print_cutset_usage;
    }

    /*
     * do a delay trace and set the critical slack
     */
    if (!delay_trace(*network, speed_param.model)){
	(void)fprintf(sisout,"%s\n", error_string());
	return 1;
    }
    set_speed_thresh(*network, &speed_param);
    table = speed_compute_weight(*network, &speed_param, NIL(array_t));

    /* compute the minimum weighted cutset of the critical paths */
    (void) com_execute(network, "_maxflow");
    mincut = cutset(*network, table);
    array_free(mincut);
    (void) com_execute(network, "_maxflow");

    st_free_table(table);
    return 0;

print_cutset_usage:
    (void) fprintf(siserr, "usage: _print_cutset [-t n.n] [-d n] [-w n] [-m model]\n");
    (void) fprintf(siserr, "    -d\tn\tDistance for collapsing.\n");
    (void) fprintf(siserr, "    -w\tn\tRelative weight of area.\n");
    (void) fprintf(siserr, "    -t\tn.n\tCritical threshold.\n");
    (void) fprintf(siserr, "    -m\tmodel\tDelay model. default (\"unit-fanout\").\n");
    return 1;
}


/*
 * Routine to check the weight computation 
 */
int
com__print_weight(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    st_table *table;
    array_t *nodevec;
    speed_global_t speed_param;

    util_getopt_reset();
    error_init();

    speed_param.interactive = TRUE;
    if (speed_fill_options(&speed_param, argc, argv)){
	(void)fprintf(sisout,"%s", error_string());
	goto print_weight_usage;
    }

    nodevec = com_get_nodes(*network, argc-util_optind+1, argv+util_optind-1);

    if (array_n(nodevec) > 0){
	if (!delay_trace(*network, speed_param.model)){
	    (void)fprintf(sisout,"%s\n", error_string());
	    return 1;
	}
	set_speed_thresh(*network, &speed_param);
	table = speed_compute_weight(*network, &speed_param, nodevec);
	st_free_table (table);
    }

    array_free(nodevec);
    return 0;

print_weight_usage:
    (void) fprintf(siserr, "usage: print_weight [-m model] [-w n.n] [-d n] [-t n.n] list-of-nodes\n");
    (void) fprintf(siserr, "    -d\tn\tDistance for collapsing.\n");
    (void) fprintf(siserr, "    -t\tn.n\tCritical threshold.\n");
    (void) fprintf(siserr, "    -w\tn\tRelative weight of area. \n");
    (void) fprintf(siserr, "    -m\tmodel\tDelay model. default (\"unit-fanout\").\n");
    return 1;
}

/* Routine to check the internal delay tracing of the speedup routines */
int
com__speed_delay(network, argc, argv)
network_t **network;
int argc;
char **argv;
{

    lsGen gen;
    node_t *node;
    delay_time_t time;
    speed_global_t speed_param;

    util_getopt_reset();
    error_init();

    speed_param.interactive = TRUE;
    if (speed_fill_options(&speed_param, argc, argv)){
	(void)fprintf(sisout,"%s", error_string());
        (void) fprintf(sisout,"Usage:  _speed_delay [-m model]\n");
        return 1;
    }

    /* Initialize the globals for the delay routines in this package */
    speed_set_delay_data(&speed_param, 0 /* No library acceleration */);

    (void) speed_delay_trace(*network, &speed_param);
    foreach_node(*network, gen, node){
        speed_delay_arrival_time(node, &speed_param, &time);
        (void) fprintf(sisout," %-10s : arrival=(%-5.2f %5.2f)\n",
		       node_name(node), time.rise, time.fall);
    }
    return 0;
}

int
com__report_delay_data(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c;
    delay_model_t model;

    error_init();
    util_getopt_reset();
    model = DELAY_MODEL_UNKNOWN;
    while ((c = util_getopt(argc, argv, "m:")) != EOF) {
        switch (c) {
        case 'm':
	    model = delay_get_model_from_name(util_optarg);
	    if (model == DELAY_MODEL_UNKNOWN){
		(void) fprintf(sisout,"Usage: _report_delay_data [-m model]\n");
		return 1;
	    }
            break;
        default:
            (void) fprintf(sisout,"Usage:  _report_delay_data [-m model]\n");
            return 1;
        }
    }
    if (model == DELAY_MODEL_UNKNOWN){
	if (lib_network_is_mapped(*network)){
	    model = DELAY_MODEL_LIBRARY;
	} else {
	    model = DELAY_MODEL_UNIT;
	}
    }

    if (delay_trace(*network, model)){
	sp_print_delay_data(sisout, *network);
    } else {
	(void) fprintf(sisout,"%s\n", error_string());
    }
    return 0;
}


static int
com_speedup_alg(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int no_options = 0;
    int verbose = 0;

    if (current_speedup_alg == NIL(array_t)) {
	current_speedup_alg = sp_get_local_trans(0, NIL(char *));
    }
    argc--; argv++;
    if (argc == 0) no_options = 1;
    while (argc > 0 && argv[0][0] == '-') {
        switch (argv[0][1]) {
        case 'v':
            verbose = 1;
            break;
        default:
            goto usage;
        }
        argc--;
        argv++;
    }
    if (! no_options) {
        current_speedup_alg = sp_get_local_trans(argc, argv);
    }
    if (verbose || no_options) sp_print_local_trans(current_speedup_alg);
    return 0;
usage:
    fprintf(siserr, "usage: speedup_alg [-v] speedup_alg1 speedup_alg2 ...\n");
    fprintf(siserr, "       -v\t verbose mode\n");
    return 1;
}


/* Routine to print the nodes in the network according to level */
int
com_print_level(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    double thresh;
    delay_model_t model;
    int c, crit_only, print_flag, level;
    array_t *node_array;

    print_flag = TRUE;
    crit_only = FALSE;
    model = DELAY_MODEL_UNIT_FANOUT;
    thresh = 0.5;

    util_getopt_reset();
    error_init();
    while ((c = util_getopt(argc, argv, "m:t:lc")) != EOF) {
        switch (c) {
        case 'm':
            model = delay_get_model_from_name(util_optarg);
            if (model == DELAY_MODEL_LIBRARY) {
		(void)fprintf(sisout,"Notice: Using the mapped model instead\n");
            	model = DELAY_MODEL_MAPPED;
            } else if (model == DELAY_MODEL_UNKNOWN) {
            	(void) fprintf(sisout,"Unknown delay model \n");
            	goto print_level_usage;
            }
            break;
        case 't':
            thresh = atof(util_optarg);
            break;
        case 'l':
	    print_flag = FALSE;
            break;
        case 'c':
	    crit_only = TRUE;
            break;
        default:
	    goto print_level_usage;
        }
    }


    print_flag = (crit_only ? TRUE : print_flag);
    /*
     * Do the delay trace if the critical path needs to be printed 
     */
    if (crit_only){
	if(!delay_trace(*network, model)){
	    (void) fprintf(sisout,"%s\n", error_string());
	    return 1;
	}
    }

    node_array = NIL(array_t);
    if (argc - util_optind != 0) {
        node_array = com_get_nodes(*network, argc-util_optind+1, argv+util_optind-1);
    }

    level = speed_print_level(*network, node_array, thresh, crit_only,
			     print_flag);

    if (!print_flag){
	(void) fprintf(sisout, "Total number of levels = %d\n", level);
    }
    if (node_array != NIL(array_t)) array_free(node_array);

    return 0;
print_level_usage:
    (void) fprintf(sisout,"Usage:  print_level [-m model] [-t thresh] [-l] [-c] [node_list]\n");
    (void) fprintf(sisout, "    -l\t\tPrint only a summary.\n");
    (void) fprintf(sisout, "    -c\t\tPrint only critical nodes.\n");
    (void)fprintf(sisout,"\t-m \tmodel\t: model for computing delays - default \"unit_fanout\"\n");
    (void)fprintf(sisout,"\t-t \t#.#\t: Threshold defining critical paths\n");
    (void)fprintf(sisout,"\tnode-list\t: Print nodes in transitive-fanin of list.\n");
    return 1;
	
}
/* ARGSUSED */
/* Utility routine that reports whether the network is in 2-input AND form */
int
com__is_2ip_and(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    node_t *node;
    lsGen gen;

    foreach_node(*network, gen, node) {
	if ((node_num_fanin(node) <= 2)  && ((node_function(node) != NODE_OR) &&
		(node_function(node) != NODE_COMPLEX))){
	    continue;
	}
	(void) fprintf(sisout,"Network is NOT in 2 input and gates\n");
	return;
    }
    (void) fprintf(sisout,"Network is in 2 input and gates\n");
}


/* 
 * Utility routine that prints the specified number of high fanout nodes on
 * critical paths
 */
int
com__print_fanout(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c, limit, max, print_max_only;
    double thresh;
    delay_time_t delay;
    delay_model_t model;
    node_t *node;
    lsGen gen;

    limit = 5;
    max = -1;
    thresh = 0.5;
    print_max_only = FALSE;
    model = DELAY_MODEL_MAPPED;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "sn:t:m:")) != EOF) {
        switch (c) {
        case 'm':
            model = delay_get_model_from_name(util_optarg);
            if (model == DELAY_MODEL_LIBRARY) {
		(void)fprintf(sisout,"Notice: Using the mapped model instead\n");
            	model = DELAY_MODEL_MAPPED;
            } else if (model == DELAY_MODEL_UNKNOWN) {
            	(void) fprintf(sisout,"Unknown delay model \n");
		goto print_fanout_usage;
            }
            break;
        case 's':
	    print_max_only = TRUE;
            break;
        case 't':
	    thresh = atof(util_optarg);
            break;
        case 'n':
	    limit = atoi(util_optarg);
            break;
        default:
            goto print_fanout_usage;
        }
    }

    if (!delay_trace(*network, model)){
	(void)fprintf(sisout,"%s\n", error_string());
	return 1;
    }

    if (!print_max_only){
	(void)fprintf(sisout, "Nodes with fanout >= %d\n", limit);
	(void)fprintf(sisout,"type     node    fo  slack\n");
    }
    foreach_node(*network, gen, node){
        if (node->type != PRIMARY_OUTPUT){
	   delay = delay_slack_time(node);
	   if (!print_max_only){
	       if(node_num_fanout(node) > limit){
		   (void)fprintf(sisout,"%s   %-10s %-3d %-5.2f\n", 
		       (node->type == INTERNAL ?"INT" : "PI "),
		       node_name(node), node_num_fanout(node), delay.rise );
	       }
	   } else {
		if (delay.rise < thresh || delay.fall < thresh){
		     max = MAX(max, node_num_fanout(node));
		}
	   }
        }
    }
    if (print_max_only){
	(void)fprintf(sisout, "Maximum fanout on crit path = %d\n", max);
    }
    return 0;
print_fanout_usage:
    (void)fprintf(sisout,"Usage: print_fanout [-s] [-t #.#] [-n #]\n");
    (void)fprintf(sisout,"\t-s \t\t: Print maximum slack on critical path \n");
    (void)fprintf(sisout,"\t-m \tmodel\t: Delay model to find critical path \n");
    (void)fprintf(sisout,"\t-t \t#.#\t: Threshold of slack defining critical path\n");
    (void)fprintf(sisout,"\t-n \t#\t: Print nodes with fanout greater than n\n");
    return 1;
}


/*
 * Set the defaults that are controllable from the command line ----
 * The rest are controlled/set by the state of the network and library
 * and the relevant routine there is buf_set_buffers()
*/
buf_set_default_options(buf_param)
buf_global_t *buf_param;
{ 
    buf_param->trace = FALSE;
    buf_param->single_pass = FALSE;
    buf_param->do_decomp = FALSE;
    buf_param->debug = FALSE;
    buf_param->limit = 2;
    buf_param->mode = 7;             /* Try all the transformations */
    buf_param->thresh = 2 * V_SMALL;
    buf_param->only_check_max_load = FALSE;
    buf_param->interactive = FALSE;
}

int
buffer_fill_options(buf_param, argc, argv)
buf_global_t *buf_param;
int argc;
char **argv;
{
    int c;

    buf_set_default_options(buf_param);
    if (argc == 0 || argv == NIL(char *)) return 0;

    while ((c = util_getopt(argc, argv, "cdLTDf:v:l:")) != EOF) {
        switch (c) {
	case 'd':
	    buf_param->do_decomp = TRUE;
	    break;
	case 'c':
	    buf_param->single_pass = TRUE;
	    break;
	case 'L':
	    buf_param->only_check_max_load = TRUE;
	    break;
	case 'T':
	    buf_param->trace = TRUE;
	    break;
	case 'f':
	    buf_param->mode = atoi(util_optarg);

    /* Check the range of buffering modes (1-7) */
     if (buf_param->mode < 1 || buf_param->mode > 7){
	error_append("ERROR: Valid range of -f option is 1 to 7\n");
       	return 1;
    }
	    break;
	case 'v':
	    buf_param->debug = atoi(util_optarg);
	    break;
	case 'l':
	    buf_param->limit = atoi(util_optarg);
	    break;
	case 'D':
	    buf_param->debug = TRUE;
	    break;
        default:
            error_append("buffering: Unknown option\n");
	    return 1;
        }
    }
    return 0;
}

buffer_print_usage()
{
    (void)fprintf(sisout,"Usage: buffer_opt [-l #] [-cdTDL] [-f #] [-v #] [node_list]\n");
    (void)fprintf(sisout,"\t-l \t#\t: Fanout limit (default = 2)\n");
    (void)fprintf(sisout,"\t-c \t\t: Buffer only one node in the pass\n");
    (void)fprintf(sisout,"\t-d \t\t: Docompose complex gates as part of buffering\n");
    (void)fprintf(sisout,"\t-T \t\t: Print the delay as algorithm proceeds\n");
    (void)fprintf(sisout,"\t-L \t\t: Only satisfy the max_load constraints\n");
    (void)fprintf(sisout,"\t-D \t\t: Debugging option\n");
    (void)fprintf(sisout,"\t-f \t#\t: Fanout transformations to use (default = 7)\n");
    (void)fprintf(sisout,"\t-v \t#\t: Verbosity level (1-100)\n");
}


int
com_buffer_opt(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    lsGen gen;
    node_t *node;
    array_t *nodevec;
    buf_global_t buf_param;
    st_table *drive_table, *load_table;
    
    util_getopt_reset();
    error_init();

    /* Set the flag for the interactive mode --- printing warnings enabled */
    buf_param.interactive = TRUE;
    if (buffer_fill_options(&buf_param, argc, argv)){
	(void)fprintf(sisout,"%s", error_string());
	buffer_print_usage();
	return 1;
    }
    
    /* First check if the network is a valid one */
    nodevec = com_get_nodes(*network, argc-util_optind+1, argv+util_optind-1);
    if (array_n(nodevec) == 0 ){
	array_free(nodevec);
	return 0;
    }
    
    /* Set the buffers based on whether the network is mapped or not */
    buf_set_buffers(*network, 0, &buf_param);
        
    /*
     * Set the loads at PO's and input drive at PI's if not set
     * These values are derived from the inverters if the network is mapped
     */
    buf_set_pipo_defaults(*network, &buf_param, &load_table, &drive_table);
            
    /* Annotate the delay data */
    if (!delay_trace(*network, buf_param.model)){
	(void)fprintf(sisout,"%s\n", error_string());
	return 1;
    }
    
    if (argc > util_optind) {
	/* Buffer the output of specified nodes only */
	(void)speed_buffer_array_of_nodes(*network, &buf_param, nodevec);
    } else if (buf_param.only_check_max_load){
	(void) sp_satisfy_max_load(*network, &buf_param);
    } else {
	(void)speed_buffer_network(*network, &buf_param);
    }
    
    /* Reset the drive and loads of the Primary IP/OP respectively */
    foreach_primary_output(*network, gen, node){
	if (st_is_member(load_table, (char *)node)){
	    delay_set_parameter(node, DELAY_OUTPUT_LOAD, DELAY_NOT_SET);
	}
    }
    foreach_primary_input(*network, gen, node){
	if (st_is_member(drive_table, (char *)node)){
	    delay_set_parameter(node, DELAY_DRIVE_RISE, DELAY_NOT_SET);
	    delay_set_parameter(node, DELAY_DRIVE_FALL, DELAY_NOT_SET);
	}
    }
    
    st_free_table(drive_table);
    st_free_table(load_table);
    buf_free_buffers(&buf_param);
    array_free(nodevec);
    
    return 0;
}

init_speed()
{
    current_speedup_alg = NIL(array_t);

    (void) com_add_command("speed_up", com_speed_up, 1);
    (void) com_add_command("speedup_alg", com_speedup_alg, 0);
    (void) com_add_command("buffer_opt", com_buffer_opt, 1);
    (void) com_add_command("print_level", com_print_level, 0);
    (void) com_add_command("_print_fanout", com__print_fanout, 0);
    (void) com_add_command("_report_delay_data", com__report_delay_data, 0);
    (void) com_add_command("_part_collapse", com__part_collapse, 1);
    (void) com_add_command("_print_cutset", com__print_cutset, 0);
    (void) com_add_command("_print_weight", com__print_weight, 0);
    (void) com_add_command("_speed_delay", com__speed_delay, 0);
    (void) com_add_command("_is_2ip_and", com__is_2ip_and, 0);

    if (com_graphics_enabled()) {
	com_add_command("_speed_plot", com__speed_plot, 0);
    }

    node_register_daemon(DAEMON_ALLOC, buffer_alloc);
    node_register_daemon(DAEMON_FREE, buffer_free);
}

end_speed() 
{
    sp_free_local_trans(&current_speedup_alg);
}
