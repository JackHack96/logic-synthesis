
#ifdef SIS
#include "sis.h"
#include "stg_int.h"

network_t *stg_to_network();

static void stg_network_add_clock();
static void stg_network_connect_clock();
static int write_encoded_espresso_format();

#define STG_LATCH_LIMIT 16
#define STG_CHECK_EDGE_LIMIT 500

/*ARGSUSED*/
static int
com_stg_extract(network,argc,argv)
network_t **network;
int argc;
char **argv;
{
    int check_containment = FALSE;
    int override = 0;
    int opt = 0;
    long msec;
    int c;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "aec")) != EOF) {
	switch(c) {
	    case 'c':
		check_containment = TRUE;
		break;
	    case 'a':
		opt = 1;
		break;
	    case 'e':
		override = 1;
		break;
	    default:
                (void) fprintf(siserr,"Usage: %s [-a] [-e] [-c]\n",argv[0]);
                (void) fprintf(siserr,
                            "\t-a: find for all possible start states\n");
		(void) fprintf(siserr, "\t    (only if there are no more than %d latches)\n", STG_LATCH_LIMIT);
                (void) fprintf(siserr,
                            "\t-e: extract even if the number of latches exceeds %d\n", STG_LATCH_LIMIT);
                (void) fprintf(siserr,
                            "\t-c: always check that the network covers the STG\n");
                return 1;
	}
    }
    if (network_num_latch(*network) == 0) {
	(void) fprintf(siserr, "Network has no latches\n");
	return 1;
    } 
    if (opt && (network_num_latch(*network) > STG_LATCH_LIMIT)){
	(void) fprintf(siserr, "Network has too many latches,");
	(void) fprintf(siserr, "Cannot use all states as initial states\n");
	return 1;
    }
    if ((network_num_latch(*network) > STG_LATCH_LIMIT) && (!override)) {
	(void) fprintf(siserr, "Network has too many latches,");
	(void) fprintf(siserr, " STG may take a long time to extract.\n");
	(void) fprintf(siserr, "Use stg_extract -e to override.\n");
	return 1;
    }

    msec = util_cpu_time();
    if (stg_extract(*network,opt) != NIL(graph_t)) {
        (void) fprintf(sisout,"Total number of states = %d\n",total_no_of_states);
        (void) fprintf(sisout,"Total number of edges = %d\n",total_no_of_edges);
        (void) fprintf(sisout,"Total time = %g\n",
		       (double) (util_cpu_time() - msec) / 1000);
    }

    if (!check_containment && total_no_of_edges > STG_CHECK_EDGE_LIMIT){
	(void)fprintf(sisout, "NOTE: There are more than %d transitions\n", STG_CHECK_EDGE_LIMIT);
	(void)fprintf(sisout, "\tSkipping the check that network covers the STG\n");
	(void)fprintf(sisout, "\tUse the -c option to force this check\n");
    } else if (!network_stg_check(*network)) {
	return 1;
    }
    return(0);
}


static int
com_stg_state_assign(network,argc,argv)
network_t **network;
int argc;
char **argv;
{
    FILE *to, *from;
    char command[1024];
    char buffer[1024];
    char *infile;
    char *outfile;
    int i, c, status;
    network_t *n;
    network_t *new;
    int help = 0;
    char *error;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "x")) != EOF) {
	switch (c) {
	    case 'x':
		(void) fprintf(siserr,
                       "usage: state_assign program_name options\n");
		return 1;
	}
    }
    error_init();
    if (network_stg(*network) == NIL(graph_t)) {
	(void) fprintf(siserr, "There is no stg\n");
	return 1;
    }

    infile = util_tempnam(NIL(char), "SIS");
    outfile = util_tempnam(NIL(char), "SIS");

    if (argc == 1) {
	(void) sprintf(command, "nova < %s > %s", infile, outfile);
	(void) fprintf(sisout, "Running nova, written by Tiziano Villa,");
	(void) fprintf(sisout, "  UC Berkeley\n");
    } else {
	buffer[0] = '\0';
        if (!strcmp(argv[1], "nova")) {
            (void) fprintf(sisout, "Running nova, written by Tiziano Villa,");
            (void) fprintf(sisout, "  UC Berkeley\n");
        } else if (!strcmp(argv[1], "jedi")) {
            (void) fprintf(sisout, "Running jedi, written by Bill Lin,");
            (void) fprintf(sisout, "  UC Berkeley\n");
        }
        for (i = 1; i < argc; i++) {
	    (void) strcat(buffer, argv[i]);
	    (void) strcat(buffer, " ");
	    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
	        help = 1;
	    }
        }
	(void) sprintf(command, "%s < %s > %s", buffer, infile, outfile);
    }

    if ((to = fopen(infile, "w")) == NULL) {
	(void) fprintf(siserr, "Error: cannot open temporary file - %s\n", infile);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 1;
    }

    if (!help && !write_kiss(to, network_stg(*network))) {
	(void) fprintf(siserr, "Error in write_kiss\n");
	(void) fclose(to);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 1;
    }
    (void) fclose(to);

    status = system(command);

    if (status && !help) {
	(void) fprintf(siserr, "state assignment program returned non-zero exit status\n%s\n", command);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 1;
    }

    if (help) {
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 0;
    }

    if ((from = fopen(outfile, "r")) == NULL) {
	(void) fprintf(siserr, "Error: cannot open output file - %s\n", outfile);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 1;
    }

    /* read_blif requires that the global variable read_filename be set */
    read_register_filename(outfile); 
    if (!read_blif(from, &n)) {
	(void) fprintf(siserr, "Error in state assignment program output:\n");
	(void) fprintf(siserr, "read_blif: %s\n", error_string());
	read_register_filename(NIL(char));
	network_free(n);
	(void) fclose(from);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 1;
    }

    (void) fclose(from);
    (void) unlink(infile);
    (void) unlink(outfile);
    FREE(infile);
    FREE(outfile);

    error = error_string();
    if (error[0] != '\0') {
	(void) fprintf(siserr, "%s", error);
    }

    if (network_num_pi(n) == 0 || network_num_po(n) == 0) {
	new = stg_to_network(network_stg(n), 1);
	if (new == NIL(network_t)) {
	    return 1;
	}
	network_set_stg(n, (graph_t *) NULL);
	network_free(n);
	n = new;
    }
    stg_set_network_pipo_names(n, network_stg(*network));
    /* Add the clocks and set the latch type and control */
    stg_network_add_clock(n, network_stg(*network));
    stg_network_connect_clock(n, network_stg(*network));
    /* The clock info has to be stored with the STG as well */
    stg_copy_clock_data(network_stg(*network), network_stg(n));

    network_set_name(n, network_name(*network));
    network_free(*network);
    *network = n;
    return 0;
}


static int
com_stg_state_minimize(network,argc,argv)
network_t **network;
int argc;
char **argv;
{
    FILE *to, *from;
    int i, c, status;
    int num_states;
    char command[1024];
    char buffer[1024];
    char *infile;
    char *outfile;
    graph_t *g;
    network_t *new_network;
    int help = 0;
    char *error;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "x")) != EOF) {
	switch (c) {
	    case 'x':
		(void) fprintf(siserr,
                       "usage: state_mininimize program_name options\n");
		return 1;
	}
    }
    error_init();
    if (network_stg(*network) == NIL(graph_t)) {
	(void) fprintf(siserr, "There is no stg\n");
	return 1;
    }

    num_states = stg_get_num_states(network_stg(*network));
    infile = util_tempnam(NIL(char), "SIS");
    outfile = util_tempnam(NIL(char), "SIS");

    if (argc == 1) {
	(void) sprintf(command, "stamina < %s > %s", infile, outfile);
	(void) fprintf(sisout, "Running stamina, written by June Rho,");
	(void) fprintf(sisout, " University of Colorado at Boulder\n");
    } else {
	buffer[0] = '\0';
        if (!strcmp(argv[1], "stamina")) {
            (void) fprintf(sisout, "Running stamina, written by June Rho,");
            (void) fprintf(sisout, " University of Colorado at Boulder\n");
        } else if (!strcmp(argv[1], "freduce")) {
            (void) fprintf(sisout, "Running freduce, written by Bill Lin,");
            (void) fprintf(sisout, "  UC Berkeley\n");
        } else if (!strcmp(argv[1], "sred")) {
            (void) fprintf(sisout, "Running sred, written by Tiziano Villa,");
            (void) fprintf(sisout, "  UC Berkeley\n");
        }
        for (i = 1; i < argc; i++) {
	    (void) strcat(buffer, argv[i]);
	    (void) strcat(buffer, " ");
	    if (!strcmp(argv[i], "-h")) {
	        help = 1;
	    }
        }
	(void) sprintf(command, "%s < %s > %s", buffer, infile, outfile);
    }

    if ((to = fopen(infile, "w")) == NULL) {
	(void) fprintf(siserr, "Error: cannot open temporary file - %s\n", infile);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 1;
    }

    if (!help && !write_kiss(to, network_stg(*network))) {
	(void) fprintf(siserr, "Error in write_kiss\n");
	(void) fclose(to);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 1;
    }
    (void) fclose(to);

    status = system(command);

    if (status && !help) {
	(void) fprintf(siserr, "state minimization program returned non-zero exit status\n%s\n", command);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 1;
    }

    if (help) {
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 0;
    }

    if ((from = fopen(outfile, "r")) == NULL) {
	(void) fprintf(siserr, "Error: cannot open output file - %s\n", outfile);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 1;
    }

    if (!read_kiss(from, &g)) {
	(void) fprintf(siserr, "Error in state minimization program output:\n");
	(void) fprintf(siserr, "read_kiss: %s\n", error_string());
	stg_free(g);
	(void) fclose(from);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return 1;
    }

    (void) fclose(from);
    (void) unlink(infile);
    (void) unlink(outfile);
    FREE(infile);
    FREE(outfile);

    error = error_string();
    if (error[0] != '\0') {
	(void) fprintf(siserr, "%s", error);
    }
    /*
     * Retain the names of the inputs and outputs of the STG
     * "stg_copy_names()" also retains the clocking information and edge index
     */
    stg_copy_names(network_stg(*network), g);

    /* The clock info has to be stored with the STG as well */
    stg_copy_clock_data(network_stg(*network), g);

    new_network = network_alloc();
    network_set_name(new_network, network_name(*network));
    network_free(*network);
    network_set_stg(new_network, g);
    stg_set_network_pipo_names(new_network, g);
    stg_network_add_clock(new_network, g);
    stg_network_connect_clock(new_network, g);
    *network = new_network;
    (void) fprintf(sisout, "Number of states in original machine : %d\n",
                           num_states);
    (void) fprintf(sisout, "Number of states in minimized machine : %d\n",
                           stg_get_num_states(g));
    return 0;
}


/*ARGSUSED*/
static int
com_stg_cover(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    if (!network_stg_check(*network)) {
        return 1;
    }
    (void) fprintf(sisout, "STG covers the network\n");
    return 0;
}


/*ARGSUSED*/
static int
com_one_hot(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    graph_t *stg;
    int ns, i;
    char *code;
    lsGen gen;
    vertex_t *s;
    network_t *new;

    stg = network_stg(*network);
    if (stg == NIL(graph_t)) {
	(void) fprintf(sisout, "No stg present\n");
	return 0;
    }
    ns = stg_get_num_states(stg);
    code = ALLOC(char, ns+1);
    for (i = 0; i < ns; i++) {
        code[i] = '0';
    }
    code[ns] = '\0';
    code[0] = '1';
    i = 1;
    stg_foreach_state(stg, gen, s) {
        stg_set_state_encoding(s, code);
        if (i < ns) {
            code[i] = '1';
            code[i-1] = '0';
        }
        i++;
    }
    FREE(code);
    new = stg_to_network(stg, 0);
    network_set_name(new, network_name(*network));
    stg_set_network_pipo_names(new, network_stg(*network));

    /* Add the clocks and set the latch type and control */
    stg_network_add_clock(new, stg);
    stg_network_connect_clock(new, stg);

    network_set_stg(*network, NIL(graph_t));
    network_free(*network);
    *network = new;
    return 0;
}


#ifdef STG_DEBUG
static int
stg_test(network)
network_t **network;
{
    graph_t *stg;
    network_t *net = *network;
    static char *vect = "101101001110010";
    vertex_t *state;
    long msec,new;
    char buf[100];
    static int states = 0;
    
    msec = util_cpu_time();
    (void) stg_extract(net,0);
    new = util_cpu_time();
    (void) fprintf(sisout,"extraction: %g\n",(double) (new - msec) / 1000);
    msec = new;
    stg = network_stg(net);
    stg_check(stg);
    new = util_cpu_time();
    (void) fprintf(sisout,"checking: %g\n",(double) (new - msec) / 1000);
    (void)fprintf(sisout,"The extracted STG is \n");
    stg_dump_graph(sisout, stg);
    new = util_cpu_time();
    msec = new;
    stg_sim(stg,vect);
    stg_sim(stg,vect);
    stg_sim(stg,vect);
    stg_sim(stg,vect);
    new = util_cpu_time();
    (void) fprintf(sisout,"4 simulations: %g\n",(double) (new - msec) / 1000);
    msec = new;
    (void) fprintf(sisout,"%s %s\n",stg_get_state_name(stg_get_start(stg)),
    	    stg_get_state_name(stg_get_current(stg)));
    if (stg_get_num_inputs(stg) == 4 && (stg_get_num_outputs(stg) == 4)) {
	(void) sprintf(buf, "st%d", states++);
        state = stg_create_state(stg, buf, NIL(char));
	(void) stg_create_transition(state,stg_get_current(stg),"01-1","0100");
	(void) write_kiss(sisout,stg);
	(void) putc('\n',sisout);
    }
    stg_reset(stg);
    (void) fprintf(sisout,"%s %s\n",stg_get_state_name(stg_get_start(stg)),
    	stg_get_state_name(stg_get_current(stg)));
    return(0);
}
#endif /* STG_DEBUG */

static int
com_stg_to_network(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    network_t *new;
    int c;
    int option;

    option = 1;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "e:")) != EOF) {
	switch (c) {
	case 'e':
	    option = atoi(util_optarg);
	    if (option < 0 || option > 2) {
		goto usage;
	    }
	    break;
	default:
	    goto usage;
	}
    }

    new = stg_to_network(network_stg(*network), option);
    if (new == NIL(network_t)) {
	return 1;
    }
    /* Add the clocks and set the latch type and control */
    stg_network_add_clock(new, network_stg(*network));
    stg_network_connect_clock(new, network_stg(*network));
    stg_set_network_pipo_names(new, network_stg(*network));

    network_set_stg(*network, (graph_t *) NULL);

    /* The network was given a random name - assign in to the name */
    /* of the original network. */
    network_set_name(new, network_name(*network));
    network_free(*network);
    *network = new;
    return 0;

usage:
    (void) fprintf(siserr, "usage: stg_to_network [-e option]\n");
    return 1;
}

network_t *
stg_to_network(stg, option)
graph_t *stg;
int option;
{
    FILE *to, *from;
    char command[1024];
    char *infile;
    char *outfile;
    int status;
    network_t *network;
    char *error;
    latch_t *l;
    node_t *pi, *po;
    int latches, n, c;
    lsGen genpi, genpo;
    char *start;
    int stg_single;

    if (stg == (graph_t *) NULL) {
	(void) fprintf(siserr, "stg_to_network: no stg specified\n");
	return NIL(network_t);
    }

    start = stg_get_state_encoding(stg_get_start(stg));
    if (start == NIL(char)) {
	(void) fprintf(siserr, "error: FSM has no encoding\n");
	return NIL(network_t);
    }

    infile = util_tempnam(NIL(char), "SIS");
    outfile = util_tempnam(NIL(char), "SIS");

    switch(option) {
	case 0:
	(void) sprintf(command, "espresso -o fd < %s > %s", infile, outfile);
	stg_single = 0;
	break;
	
	case 1:
	(void) sprintf(command, "espresso -o fd < %s > %s", infile, outfile);
	stg_single = 1;
	break;
	
	case 2:
	(void) sprintf(command, "espresso -o fd -Dso < %s > %s", infile, outfile);
	stg_single = 1;
	break;
	
	default:
	return NIL(network_t);
    }

    if ((to = fopen(infile, "w")) == NULL) {
	(void) fprintf(siserr, "error: can not open temporary file - %s\n", infile);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return NIL(network_t);
    }
    
    if (write_encoded_espresso_format(to, stg)) {
	(void) fprintf(siserr, "error in writing encoded state table\n");
	(void) fclose(to);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return NIL(network_t);
    }
    (void) fclose(to);

    status = system(command);

    if (status) {
	(void) fprintf(siserr, "stg to network program returned non-zero exit status\n%s\n", command);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return NIL(network_t);
    }

    /* sis_read_pla requires that the global variable read_filename be set */
    read_register_filename(outfile);

    if ((from = fopen(outfile, "r")) == NULL) {
	(void) fprintf(siserr, "error: can not open output file - %s\n", outfile);
	(void) unlink(infile);
	(void) unlink(outfile);
	FREE(infile);
	FREE(outfile);
	return NIL(network_t);
    }

    network = sis_read_pla(from, stg_single);
    read_register_filename(NIL(char));
    (void) fclose(from);
    (void) unlink(infile);
    (void) unlink(outfile);
    FREE(infile);
    FREE(outfile);
    
    if (network == NIL(network_t)) {
	(void) fprintf(siserr, "read_blif: error reading PLA\n");
	return NIL(network_t);
    }
    
    error = error_string();
    if (error[0] != '\0') {
	(void) fprintf(siserr, "%s", error);
    }
    if (!network_check(network)) {
	(void) fprintf(siserr, "%s\n", error_string());
	return NIL(network_t);
    }


    latches = strlen(start);
    genpi = lsStart(network->pi);
    genpo = lsStart(network->po);
    n = lsLength(network->pi);
    while (n > latches) {
        (void) lsNext(genpi,(lsGeneric *) &pi,LS_NH);
	n--;
    }
    while (latches) {
        (void) lsNext(genpi,(lsGeneric *) &pi,LS_NH);
	(void) lsNext(genpo,(lsGeneric *) &po,LS_NH);
	/* changed to get it to compile */
	(void) network_create_latch(network, &l, po, pi);
	c = *start++;
	switch(c) {
	    case '0' :
	      latch_set_initial_value(l, 0);
	      latch_set_current_value(l, 0);
	      break;
	    case '1' :
	      latch_set_initial_value(l, 1);
	      latch_set_current_value(l, 1);
	      break;
	    default :
	      fail("An encoding bit for the start state has an invalid value");
	  }
	latches--;
    }
    (void) lsFinish(genpi);
    (void) lsFinish(genpo);

    stg_free(network_stg(network));
    network_set_stg(network, stg);
    return network;
}

/*
 * Adds the clock data structure to the network
 */
static void
stg_network_add_clock(network, stg)
network_t *network;
graph_t *stg;
{
    lsGen gen;
    stg_clock_t *clock_data;
    sis_clock_t *clock;
    clock_edge_t edge;
    int add_clock = TRUE;

    /* Add the clocking information to the network as well */
    if ((clock_data = stg_get_clock_data(stg)) == NIL(stg_clock_t)){
	return;
    }
    /* Check if the clock already exists */
    foreach_clock(network, gen, clock){
	if (strcmp(clock_name(clock), clock_data->name) == 0){
	    /* Clock already present in network -- NOVA does this */
	    add_clock = FALSE;
	    lsFinish(gen);
	    break;
	}
    }

    if (add_clock){
	clock = clock_create(clock_data->name);
	(void)clock_add_to_network(network, clock);
    }

    clock_set_cycletime(network, clock_data->cycle_time);
    edge.clock = clock;

    edge.transition = RISE_TRANSITION;
    clock_set_parameter(edge, CLOCK_NOMINAL_POSITION, clock_data->nominal_rise);
    clock_set_parameter(edge, CLOCK_LOWER_RANGE, clock_data->min_rise);
    clock_set_parameter(edge, CLOCK_UPPER_RANGE, clock_data->max_rise);

    edge.transition = FALL_TRANSITION;
    clock_set_parameter(edge, CLOCK_NOMINAL_POSITION, clock_data->nominal_fall);
    clock_set_parameter(edge, CLOCK_LOWER_RANGE, clock_data->min_fall);
    clock_set_parameter(edge, CLOCK_UPPER_RANGE, clock_data->max_fall);

    return;
}

/*
 * Add the PI node corresponding to the clock signal, Also add adummy
 * PO and set that to be the contol of all the latches
 * THIS ROUTINE MUST BE CALLED AFTER (stg_set_network_pipo_names())
 */

static void 
stg_network_connect_clock(network, stg)
network_t *network;
graph_t *stg;
{
    lsGen gen;
    latch_t *latch;
    int edge_index;
    latch_synch_t type;
    stg_clock_t *clock_data;
    node_t *pi_node, *po_node;

    if ((clock_data = stg_get_clock_data(stg)) == NIL(stg_clock_t)){
	return;
    }

    /* Find the type of the latches */
    edge_index = stg_get_edge_index(stg);
    type = (edge_index == 0 ? RISING_EDGE : 
	    (edge_index == 1 ? FALLING_EDGE : UNKNOWN));

    if ((pi_node = network_find_node(network, clock_data->name)) != NIL(node_t)
	    && pi_node->type == PRIMARY_INPUT){
	/* PI node of the same name is present as clock */
    } else {
	/* Add the Primary input node for the clock */
	pi_node = node_alloc();
	network_add_primary_input(network, pi_node);
	network_change_node_name(network, pi_node, util_strsav(clock_data->name));
    }

    if (type != UNKNOWN){
	if (node_num_fanout(pi_node) == 0){
	    po_node = network_add_fake_primary_output(network, pi_node);
	} else {
	    /* Need to find unique fanout that is a PRIMARY_OUTPUT */
	    (void)fprintf(sisout, "Fanouts present for clcok input\n");
	    po_node = node_get_fanout(pi_node, 0);
	}
	foreach_latch(network, gen, latch){
	    latch_set_control(latch, po_node);
	    latch_set_type(latch, type);
	}
    } else {
	/* Latches are not controlled , even though clocks are present */
    }

    return;
}

static int
write_encoded_espresso_format(fp, stg)
FILE *fp;
graph_t *stg;
{
    vertex_t *start;
    edge_t *e;
    lsGen lgen;
    int state_bits;

    start = stg_get_start(stg);
    if (stg_get_state_encoding(start) == NIL(char)) {
	(void) fprintf(siserr, "FSM has no encoding\n");
	return 1;
    }
    state_bits = strlen(stg_get_state_encoding(start));
    (void) fprintf(fp, ".type fr\n");
    (void) fprintf(fp, ".i %d\n", stg_get_num_inputs(stg) + state_bits);
    (void) fprintf(fp, ".o %d\n", stg_get_num_outputs(stg) + state_bits);
    stg_foreach_transition(stg, lgen, e) {
	(void) fprintf(fp, "%s %s %s %s\n", stg_edge_input_string(e),
		        stg_get_state_encoding(stg_edge_from_state(e)),
	                stg_get_state_encoding(stg_edge_to_state(e)),
	                stg_edge_output_string(e));
    }
    (void) fprintf(fp, ".e\n");
    return 0;
}


/*ARGSUSED*/
static int
com_stg_dump_graph(network,argc,argv)
network_t **network;
int argc;
char **argv;
{
    graph_t *stg;

    stg = network_stg(*network);
    stg_dump_graph(stg, *network);
    return 0;
}


init_stg()
{
    com_add_command("stg_extract",com_stg_extract,1);
    com_add_command("stg_to_network",com_stg_to_network,1);
    com_add_command("state_assign",com_stg_state_assign,1);
    com_add_command("state_minimize",com_stg_state_minimize,1);
    com_add_command("stg_cover",com_stg_cover,0);
    com_add_command("one_hot",com_one_hot,1);
    com_add_command("_stg_dump_graph",com_stg_dump_graph,0);
#ifdef STG_DEBUG
    com_add_command("_stg_test",stg_test,0);
#endif /* STG_DEBUG */
}

end_stg()
{
}
#endif /* SIS */
