
#ifdef SIS
#include "sis.h"
#include "retime_int.h"

int retime_debug;
static void retime_print_stg_data();

int
com_retime(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    re_graph *graph;
    delay_model_t model;
    network_t *new_network;
    latch_synch_t old_type;
    clock_setting_t setting;
    int num_latch = network_num_latch(*network);
    int found_unknown_type, use_mapped, minimize_reg; 
    int c, flag, success, milp_flag;
    int can_initialize, should_initialize;
    double spec_c, new_c, desired_c, init_c;
    double retime_tol, area_r, delay_r;

    milp_flag = FALSE;		/* To use milp formulation of Saxe's routine */
    minimize_reg = FALSE;	/* To minimize registers or not */
    should_initialize = 1;	/* Initialize if required */
    model = DELAY_MODEL_MAPPED;
    area_r = RETIME_DEFAULT_REG_AREA;
    delay_r = RETIME_DEFAULT_REG_DELAY;
    use_mapped = TRUE;

    desired_c = RETIME_NOT_SET;
    retime_debug = FALSE;
    retime_tol = RETIME_DEF_TOL;

    error_init();
    util_getopt_reset();

    while ((c = util_getopt(argc, argv, "fimnv:c:d:a:t:")) != EOF) {
	switch (c) {
	    case 'n':
		use_mapped = FALSE;
		model = DELAY_MODEL_UNIT_FANOUT;
		break;
	    case 'i':
		should_initialize = 0;		/* Do not initialize latches */
		break;
	    case 'm':
		minimize_reg = TRUE;
		break;
	    case 'f':
		milp_flag = TRUE;
		break;
	    case 't':
		retime_tol = atof(util_optarg );
		break;
	    case 'd':
		delay_r = atof(util_optarg );
		break;
	    case 'a':
		area_r = atof(util_optarg );
		break;
	    case 'c':
		desired_c = atof(util_optarg );
		break;
	    case 'v':
		retime_debug = atoi(util_optarg);
		break;
	    default:
		goto retime_usage;
	}
    }

    /*
     * First check if there are any latches at all
     */
    if (re_empty_network(*network) || num_latch == 0){
	return 0;
    }

    if (use_mapped){
	if (!lib_network_is_mapped(*network)){
	    (void) fprintf(siserr,"Network is not mapped (use -n option for unmapped networks)\n");
	    return 0;
	}
	if (lib_get_library() == NIL(library_t)){
	    (void) fprintf(siserr,"Need to define a library (use -n option for unmapped networks)\n");
	    return 0;
	}
    }

    if (!use_mapped){
	network_sweep(*network);
    }

    if (!delay_trace(*network, model)){
	(void)fprintf(siserr,"%s", error_string());
	return 1;
    }

    /* 
     * Check to see if (single phase), edge triggered system
     */
    if (retime_get_clock_data(*network, use_mapped, &found_unknown_type,
	    &should_initialize, &old_type, &delay_r, &area_r)){
	(void)fprintf(siserr,"%s", error_string());
	return 1;
    }

    if (found_unknown_type && old_type != UNKNOWN){
	(void)fprintf(sisout,"Latches with unknown synchronization type assumed %s triggered\n",
	    (old_type == RISING_EDGE ? "rising edge" : "falling_edge"));
    }

    if (retime_debug > 20){
	(void)fprintf(sisout,"Latch-delay %5.3f, Area %6.2f\n", delay_r, area_r);
    }

    if (desired_c == RETIME_NOT_SET){
	if ((spec_c = clock_get_cycletime(*network)) > 0){
	    (void)fprintf(sisout,"Retiming will try to meet the cycle time of the specification\n");
	    desired_c = spec_c;
	}
    }
    if (desired_c > 0 && desired_c < delay_r){
	(void)fprintf(sisout,"Delay through register exceeds desired cycletime\n");
	return 0;
    }

    /*
     * Convert it into a retime graph
     */
    graph = retime_network_to_graph(*network, 1, use_mapped);
    if (graph == NIL(re_graph)){
	(void)fprintf(siserr,"%s", error_string());
	return 1;
    }

    /* Just for checking */
    if (retime_debug > 60) {
	if (!retime_check_graph(graph)){
	    (void)fprintf(siserr,"%s", error_string());
	}
	error_init();
	retime_dump_graph(sisout, graph);
    }

    if (desired_c == RETIME_NOT_SET){
	/* Not specified in the blif file or command line */
	desired_c = delay_r + retime_cycle_lower_bound(graph);
	(void)fprintf(sisout,"Lower bound on the cycle time = %5.2f\n",
	      desired_c);
	(void)fprintf(sisout,"Retiming will minimize the cycle time \n");
    }

    init_c = re_cycle_delay(graph, delay_r);
    (void)fprintf(sisout,"RETIME: Initial clk = %-5.2f, Desired clk = %-5.2f\n",
	    init_c, desired_c);

    if (!minimize_reg && init_c <= desired_c){
	(void)fprintf(sisout,"Circuit meets specification\n");
	re_delete_temp_nodes(*network);
	re_graph_free(graph);
	return 0;
    }

    /*
     * Retime graph returns -1 if there is an error and 0 if retiming does
     * not change the network
     */
    flag = retime_graph(*network, graph, area_r, delay_r, retime_tol,
	    desired_c, &new_c, milp_flag, minimize_reg, should_initialize,
	    &can_initialize);

    if (flag <= 0){
	/*
	 * Return the original network --- no changes made
	 */
	if (should_initialize && !can_initialize){
	    (void)fprintf(sisout,"RETIMING DOES NOT PRESERVE INITIAL STATES --- NETWORK NOT CHANGED\n");
	    (void)fprintf(sisout,"(use -i option to suppress initial-state computation)\n");
	} else {
	    (void)fprintf(sisout,"No latches were moved by retiming\n");
	}
	re_delete_temp_nodes(*network);
	re_graph_free(graph);
	return 0;
    }

    /*
     * We call the retiming a success only if cycle time improved and we can
     * get a correct set of initial states for the circuit
     */
    success = FALSE;
    if (minimize_reg || (new_c <= init_c)) {
	success = TRUE;
    }

    success = (success && can_initialize);

    if (success){
	new_network = retime_graph_to_network(graph, use_mapped);
	if (new_network == NIL(network_t)){
	    (void)fprintf(siserr,"Unable to reconstruct network: %s", error_string());
	    re_delete_temp_nodes(*network);
	    return 1;
	}
	network_set_name(new_network, network_name(*network));
	network_clock_dup(*network, new_network);  /* To recover the clocks */
	delay_network_dup(new_network, *network);  /* To copy global defaults */
	network_free(*network);
	*network = new_network;

	num_latch = network_num_latch(*network);
	(void)fprintf(sisout,"\nfinal cycle delay         = %-5.2f\n", new_c);
	(void)fprintf(sisout,"final number of registers = %-d\n", num_latch);
	(void)fprintf(sisout,"final logic cost          = %-5.2f\n",
		re_sum_node_area(graph));
	(void)fprintf(sisout,"final register cost       = %-5.2f\n\n",
		num_latch * area_r);

    }

    /*
     * also set the value of the working cycle time to be the one achieved
     */
    setting = clock_get_current_setting(*network);
    clock_set_current_setting(*network, WORKING);
    clock_set_cycletime(*network, new_c);
    clock_set_current_setting(*network, setting);
    
    re_delete_temp_nodes(*network); /* Delete any temporary nodes */
    re_graph_free(graph);

    (void) fprintf(sisout,"RETIME: Final cycle time %s achieved = %-5.2f\n",
	    (can_initialize ? "": "that can be"), new_c);

    return 0;

retime_usage:
    (void)fprintf(sisout,"retime [-fimn] [-c #.#] [-t #.#] [-d #.#] [-a #.#] [-v #]\n");
    (void)fprintf(sisout,"-i\t\t: Do not recompute the initial states\n"); 
    (void)fprintf(sisout,"-n\t\t: Use delay/area of unmapped network\n"); 
    (void)fprintf(sisout,"-m\t\t: Minimize registers subject to given cycle time (-c option)");
     (void)fprintf(sisout,"\t\t: May be very slow for large circuits\n"); 
    (void)fprintf(sisout,"-f\t\t: Use MILP formulation (default is Saxe's relaxation alg)\n"); 
    (void)fprintf(sisout,"-c\t#.#\t: Set the desired clock period\n"); 
    (void)fprintf(sisout,"-t\t#.#\t: Set tolerance for binary search (default = 0.1)\n"); 
    (void)fprintf(sisout,"-d\t#.#\t: Set the delay thru register (with -n option only)\n"); 
    (void)fprintf(sisout,"-a\t#.#\t: Set the area of a register (with -n option only)\n"); 
    (void)fprintf(sisout,"-v\t#\t: Set the verbosity level (0-100)\n"); 
    return 1;
}

/* ARGSUSED */
int
com__print_stats(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c;
    re_graph *graph;
    double init_c;
    latch_synch_t old_type;
    double delay_r, area_r;
    int use_mapped, num_latch, flag, to_init;

    retime_debug = FALSE;
    to_init = FALSE;
    area_r = RETIME_DEFAULT_REG_AREA;
    delay_r = RETIME_DEFAULT_REG_DELAY;
    use_mapped = TRUE;

    error_init();
    util_getopt_reset();

    while ((c = util_getopt(argc, argv, "d:a:")) != EOF) {
	switch (c) {
	    case 'a':
		area_r = atof(util_optarg );
		break;
	    case 'd':
		delay_r = atof(util_optarg );
		break;
	    default:
		(void)fprintf(sisout,"Unknown option\n");
		return 1;
		/* NOTREACHED */
		break;
	}
    }

    if (re_empty_network(*network)){
	retime_print_stg_data(network_stg(*network));
	return 0;
    }
    if (use_mapped && (lib_get_library() == NIL(library_t)) ){
	(void) fprintf(sisout,"Using the \"unit-fanout\" delay model\n");
	use_mapped = FALSE;
    }
    if (use_mapped &&  (!lib_network_is_mapped(*network)) ){
	(void) fprintf(sisout,"Using the \"unit-fanout\" delay model\n");
	use_mapped = FALSE;
    }

    graph = retime_network_to_graph(*network, 1, use_mapped);
    re_delete_temp_nodes(*network);
    if (graph == NIL(re_graph)){
	(void)fprintf(siserr,"%s", error_string());
	return 1;
    }
    if (retime_get_clock_data(*network, use_mapped, &flag,
	    &to_init, &old_type, &delay_r, &area_r)){
	(void)fprintf(siserr,"%s", error_string());
	return 1;
    }
    (void)fprintf(sisout,"Latch-delay %5.3f, Area %6.2f\n", delay_r, area_r);

    init_c = re_cycle_delay(graph, delay_r);

    num_latch = network_num_latch(*network);

    (void)fprintf(sisout,"Cycle delay         = %-5.2f\n", init_c);
    (void)fprintf(sisout,"Number of registers = %-d\n", num_latch);
    (void)fprintf(sisout,"Register area       = %-5.2f\n", area_r * num_latch);
    (void)fprintf(sisout,"Combinational area  = %-5.2f\n\n", re_sum_node_area(graph));

    retime_print_stg_data(network_stg(*network));
    re_delete_temp_nodes(*network);

    return 0;
}

static void
retime_print_stg_data(stg)
graph_t *stg;
{
    if (stg == NIL(graph_t)) return;

    (void)fprintf(sisout,
	    "STG data: %d inputs, %d outputs, %d states, %d edges\n",
	    stg_get_num_inputs(stg), stg_get_num_outputs(stg),
	    stg_get_num_states(stg), stg_get_num_products(stg));
}

int
com__check_tr(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    lsGen gen;
    latch_t *latch;
    re_graph *graph;
    network_t *newnet;
    latch_synch_t old_type;
    int c, to_init, use_mapped, found_unknown_type;
    double delay_r, area_r;

    use_mapped = TRUE;
    to_init = FALSE;
    retime_debug = 100;
    area_r = RETIME_DEFAULT_REG_AREA;
    delay_r = RETIME_DEFAULT_REG_DELAY;
    error_init();
    util_getopt_reset();

    while ((c = util_getopt(argc, argv, "n")) != EOF) {
	switch (c) {
	    case 'n':
		use_mapped = FALSE;
		break;
	    default:
		(void)fprintf(sisout,"Unknown option \n");
		return 1;
		/* NOTREACHED */
		break;
	}
    }

    if (re_empty_network(*network)){
	return 0;
    }

    (void)fprintf(sisout,"Latches have true inputs\n");
    foreach_latch(*network, gen, latch){
	(void)fprintf(sisout, "%s \n",
	    node_name(node_get_fanin(latch_get_input(latch), 0)));
	
    }
    (void)fprintf(sisout,"\n\n \t Initial Network \n\n");
    (void)com_execute(network, "p");
    (void)com_execute(network, "pio");
    (void)com_execute(network, "print_latch");
    if (use_mapped && !lib_network_is_mapped(*network)){
	use_mapped = FALSE;
    }
    graph = retime_network_to_graph(*network, 1, use_mapped);

    if (graph == NIL(re_graph)){
	(void)fprintf(siserr,"%s", error_string());
	return 1;
    }
    (void)fprintf(sisout,"\n\n \t Corresponding retiming graph \n\n");
    retime_dump_graph(sisout, graph);
    (void)retime_check_graph(graph);

    if (retime_get_clock_data(*network, use_mapped, &found_unknown_type,
	    &to_init, &old_type, &delay_r, &area_r)){
	(void)fprintf(siserr,"%s", error_string());
	return 1;
    }
    if (found_unknown_type && old_type != UNKNOWN){
	(void)fprintf(sisout,"Latches with unknown synchronization type assumed %s triggered\n",
	    (old_type == RISING_EDGE ? "rising edge" : "falling_edge"));
    }

    (void)fprintf(sisout,"Latch-delay %5.3f, Area %6.2f\n", delay_r, area_r);

    newnet = retime_graph_to_network(graph, use_mapped);
    if (newnet == NIL(network_t)){
	(void)fprintf(siserr,"Unable to reconstruct network: %s", error_string());
	return 1;
    }

    (void)fprintf(sisout,"\n\n \t Final Network \n\n");
    (void)com_execute(&newnet, "p");
    (void)com_execute(&newnet, "pio");
    (void)com_execute(&newnet, "print_latch");

    if (use_mapped && (!lib_network_is_mapped(newnet)) ){
	(void)fprintf(sisout, "Network after translation is not mapped\n");
	return 1;
    }

    network_clock_dup(*network, newnet);  /* To recover the clocks */
    delay_network_dup(newnet, *network);  /* To copy global defaults */
    network_free(*network);
    *network = newnet;
    re_delete_temp_nodes(*network);
    re_graph_free(graph);

    return 0;
}

init_retime()
{
    (void) com_add_command("retime", com_retime, 1);
    (void) com_add_command("_check_tr", com__check_tr, 1);
    (void) com_add_command("_print_stats", com__print_stats, 0);
}

end_retime()
{
}
#endif /* SIS */



