/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/timing/com_timing.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:59 $
 *
 */
/*
##############################################################################
#  Package developed by Narendra V. Shenoy (email shenoy@ic.berkeley.edu),   #
#  based on the clock package in SIS.                                        #
#                                                                            #
##############################################################################
*/

#ifdef SIS
#include "timing_int.h"


/* function definition 
    name:  com_clock_optimize()
    args:  network, argc, argv  
    job:   outermost routine for the clock optimization algorithm based on 
           binary search and Bellman-Ford iterations 
    return value: 0 normally
                  1 wrong options or error in circuit
    calls:  tmg_network_to_graph(),
            tmg_print_latch_graph() **Debug**
	    a bunch of set_*** flags for options
	    tmg_compute_optimal_clock(),
	    tmg_free_graph_structure()
	    
	    
*/ 
int
com_clock_optimize(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c;
    graph_t *latch_graph;
    delay_model_t model;
    double clock_cycle, set_up, hold, min_sep, max_sep;

    /* Initialize */
    /* ---------- */

    util_getopt_reset();
    debug_type = NONE;
    model = DELAY_MODEL_LIBRARY;
    tmg_set_set_up((double)0);
    tmg_set_hold((double)0);
    tmg_set_min_sep((double)0);
    tmg_set_max_sep((double)1);
    tmg_set_gen_algorithm_flag(TRUE);
    tmg_set_phase_inv(FALSE);
    /* Parse input command line options */
    while (( c = util_getopt(argc, argv, "d:nS:H:m:M:BI")) != EOF) {
	switch(c) {
	case 'd':
	    switch(atoi(util_optarg)) {
	    case 0:
		debug_type = ALL;
		break;
	    case 1:
		debug_type = LGRAPH;
		break;
	    case 2:
		debug_type = CGRAPH;
		break;
	    case 3:
		debug_type = BB;
		break;
	    case 4:
		debug_type = GENERAL;
		break;
	    default:
		debug_type = NONE;
		break;
	    }
	    break;
	case 'n':
	    model = DELAY_MODEL_UNIT_FANOUT;
	    break;
	case 'S': /* Global set-up time */
	    set_up = atof(util_optarg);
	    tmg_set_set_up(set_up);
	    break;
	case 'H': /* Global Hold time */
	    hold = atof(util_optarg);
	    tmg_set_hold(hold);
	    break;
	case 'm': /* minimum phase separation */
	    min_sep = atof(util_optarg);
	    if (min_sep > 1) goto c_opt_usage;
	    tmg_set_min_sep(min_sep);
	    break;
	case 'M': /* maximum phase separation */
	    max_sep = atof(util_optarg);
	    if (max_sep < min_sep) goto c_opt_usage;
	    if (max_sep > 1) goto c_opt_usage;
	    tmg_set_max_sep(max_sep);
	    break;
	case 'B': /* Use binary search algorithm!!                    */
	          /* Works only under certain conditions- use as debug 
		     mechansism only!!                                */
	    tmg_set_gen_algorithm_flag(FALSE);
	    break;
	case 'I': /* A flag to indicate 2 phase clocking scheme 
		   with phase 2 = inverted (phase 1)          */
	    tmg_set_phase_inv(TRUE);
	    break;
	default:
	    goto c_opt_usage;
	}
    }     
    if(!timing_network_check(*network, model)) {
	(void)fprintf(sisout, "Exiting\n!!");
    }

    (void)fprintf(sisout, "Starting: cpu_time = %s\n",
		  util_print_time(util_cpu_time())); 
    clock_set_current_setting(*network, SPECIFICATION);

    /* Construct the timing graph from the network */
    /* ------------------------------------------- */

    latch_graph = tmg_network_to_graph(*network, model, OPTIMAL_CLOCK);
    if (latch_graph == NIL(graph_t)) {
	return 1;
    }
    (void)fprintf(sisout, "Graph built: cpu_time = %s\n",
		  util_print_time(util_cpu_time()));  
    if (HOST(latch_graph) == NIL(vertex_t )) {
	(void)fprintf(sisout, "Error circuit has no primary inputs: \n");
	(void)fprintf(sisout,
		      "Current implementation requires host vertex for\n");
	(void)fprintf(sisout, "clock lower bounds!!\n");
	tmg_free_graph_structure(latch_graph);
	return 1;
    }
    if(debug_type == LGRAPH || debug_type == ALL) {
	tmg_print_latch_graph(latch_graph);
    }
    
    /* Routine to compute optimal clocking.
       Network is used only to update clock parameters*/
    /* ---------------------------------------------- */

    clock_cycle = tmg_compute_optimal_clock(latch_graph, *network);
    if (clock_cycle > 0) {
	(void)fprintf(sisout, "Optimal clock = %.2lf cpu_time = %s\n", 
		      clock_cycle, util_print_time(util_cpu_time()));
    } 

    /* Free memory associated with latch_graph */
    /* --------------------------------------- */

    tmg_free_graph_structure(latch_graph);
    return 0;
 c_opt_usage:
    (void)fprintf(sisout, "Usage: c_opt -[nBI] -[dSHmM]# \n");
    (void)fprintf(sisout, "\t -d: Debug option\n");
    (void)fprintf(sisout, "\t -n: Use unmapped circuit\n");
    (void)fprintf(sisout, "\t   : unit delay with 0.2 per fanout\n");
    (void)fprintf(sisout, "\t -S: Set up time \n");
    (void)fprintf(sisout, "\t -H: Hold time \n");
    (void)fprintf(sisout, "\t -m: minimum separation between phases [0, 1)\n");
    (void)fprintf(sisout, "\t -M: Maximum separation between phases [m, 1)\n");
    (void)fprintf(sisout, "\t -B: Use binary search\n");
    (void)fprintf(sisout, "\t -I: 2 phase clock with phi and phibar\n");
    (void)fprintf(sisout, "defaults: no debug, mapped, S = 0, H = 0, m = 0\n");
    (void)fprintf(sisout, "        : M = 1, G = TRUE, I = FALSE\n");
    return 1;
}

/* function definition 
    name:     com_clock_check()
    args: **network, argc, **argv
    job:    verify if the clocking scheme specified with the network is valid
    return value: 0 on success, 1 on failure
    calls: tmg_network_to_graph(),
           tmg_check_clocking_scheme(),
	   tmg_free_graph_structure()
    
*/ 
int
com_clock_check(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c, flag;
    graph_t *latch_graph;
    delay_model_t model;
    double set_up, hold;

    /* Initialize */
    /* ---------- */

    util_getopt_reset();
    debug_type = NONE;
    model = DELAY_MODEL_LIBRARY;
    tmg_set_set_up((double)0);
    tmg_set_hold((double)0);

    /* Parse input command line options */
    /* -------------------------------- */

    while (( c = util_getopt(argc, argv, "d:nS:H:")) != EOF) {
	switch(c) {
	case 'd':
	    switch(atoi(util_optarg)) {
	    case 0:
		debug_type = ALL;
		break;
	    case 1:
		debug_type = LGRAPH;
		break;
	    case 5:
		debug_type = VERIFY;
		break;
	    default:
		debug_type = NONE;
		break;
	    }
	    break;
	case 'n':
	    model = DELAY_MODEL_UNIT_FANOUT;
	    break;
	case 'S': /* Global set-up time */
	    set_up = atof(util_optarg);
	    tmg_set_set_up(set_up);
	    break;
	case 'H': /* Global Hold time */
	    hold = atof(util_optarg);
	    tmg_set_hold(hold);
	    break;
	default:
	    goto c_check_usage;
	}
    }     

    if (network_num_internal(*network) == 0 || 
	network_num_latch(*network) == 0) {
	(void)fprintf(sisout, "No memory elements\n");
	(void)fprintf(sisout, "exiting\n");
	return 0;
    }

    if(model == DELAY_MODEL_LIBRARY) {
	if(!lib_network_is_mapped(*network)) {
	    (void)fprintf(sisout, "Warning not all nodes are mapped. \n");
	    goto c_check_usage;
	}
    }

    (void)fprintf(sisout, "Starting: cpu_time = %s\n",
		  util_print_time(util_cpu_time())); 

    /* Construct the timing graph from the network */
    /* ------------------------------------------- */

    clock_set_current_setting(*network, SPECIFICATION);
    latch_graph = tmg_network_to_graph(*network, model, CLOCK_VERIFY);
    if (latch_graph == NIL(graph_t)) {
	return 1;
    }
    (void)fprintf(sisout, "Graph built: cpu_time = %s\n",
		  util_print_time(util_cpu_time()));  
    if (HOST(latch_graph) == NIL(vertex_t )) {
	(void)fprintf(sisout, "Error circuit has no primary inputs: \n");
	(void)fprintf(sisout, 
		      "Current implementation requires host vertex for\n");
	(void)fprintf(sisout, "clock lower bounds!!\n");
	tmg_free_graph_structure(latch_graph);
	return 1;
    }
    if(debug_type == LGRAPH || debug_type == ALL) {
	tmg_print_latch_graph(latch_graph);
    }
    
    /* Routine to verify clock - shouldnt need the network any more*/
    /* ----------------------------------------------------------- */

    if ((flag = tmg_check_clocking_scheme(latch_graph, *network))) {
	(void)fprintf(sisout, "Circuit verified : Time = %s\n",
		      util_print_time(util_cpu_time()));
    } else {
	(void)fprintf(sisout, "Error circuit failed verification!!\n");
    }

    /* Free memory associated with latch_graph */
    /* --------------------------------------- */

    tmg_free_graph_structure(latch_graph);
    return (!flag);
c_check_usage:
    (void)fprintf(sisout, "Usage: c_check -[dn] -[SH]# \n");
    (void)fprintf(sisout, "\t -d: Debug option\n");
    (void)fprintf(sisout, "\t -n: Use unmapped circuit\n");
    (void)fprintf(sisout, "\t   : unit delay with 0.2 per fanout\n");
    (void)fprintf(sisout, "\t -S: Set up time \n");
    (void)fprintf(sisout, "\t -H: Hold time \n");
    (void)fprintf(sisout, "defaults: no debug, mapped, S = 0, H = 0\n");
    
    return 1;
}


init_timing()
{
    (void)com_add_command("c_opt", com_clock_optimize, 1);
    (void)com_add_command("c_check", com_clock_check, 0);
}

end_timing()
{
}
#endif /* SIS */
