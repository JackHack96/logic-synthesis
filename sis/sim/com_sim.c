/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/sim/com_sim.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:46 $
 *
 */
#include "sis.h"
#include "sim_int.h"


#ifdef SIS

static void
dump_outputs(fp, n, vec)
FILE *fp;
network_t *n;
array_t *vec;
{
    int i;
    latch_t *l;
    lsGen gen;
    node_t *po;

    (void) fprintf(fp, "\nNetwork simulation:\n");
    (void) fprintf(fp, "Outputs:");
    i = 0;
    foreach_primary_output(n, gen, po) {
	if (network_is_real_po(n, po)) {
	    (void) fprintf(fp, " %d", array_fetch(int, vec, i));
	}
	i++;
    }
    (void) fprintf(fp, "\n");
    (void) fprintf(fp, "Next state: ");
    i = 0;
    foreach_primary_output(n, gen, po) {
	if ((l = latch_from_node(po)) != NIL(latch_t)) {
	    (void) fprintf(fp, "%d", latch_get_current_value(l));
	}
	i++;
    }
    (void) fprintf(fp, "\n");
    return;
}


static int
com_simulate(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int i, j;
    int c;
    int do_logic = 1;
    int do_stg = 1;
    array_t *in_value, *out_value, *node_vec, *out_char;
    st_table *control_table;
    int value, control_value;
    node_t *pi, *po, *control;
    latch_t *l;
    lsGen gen;
    graph_t *stg;
    vertex_t *state;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "is")) != EOF) {
	switch(c) {
	  case 'i':
	    do_stg = 0;
	    break;
	  case 's':
	    do_logic = 0;
	    break;
	  default:
	    goto usage;
	}
    }
    if (!do_stg && !do_logic) {
	do_stg = do_logic = 1;
    }
    if (do_logic) {
	if (network_num_pi(*network) != 0) {
	    if (argc-util_optind != (network_num_pi(*network) -
				network_num_latch(*network))) {
		(void) fprintf(miserr, 
			       "simulate network: network has %d inputs; %d values were supplied.\n",
			       network_num_pi(*network) -
			       network_num_latch(*network), argc - 1);
		goto usage;
	    }
	    in_value = array_alloc(int, network_num_pi(*network));
	    i = util_optind;
	    foreach_primary_input(*network, gen, pi) {
		if ((l = latch_from_node(pi)) != NIL(latch_t)) {
		    value = latch_get_current_value(l);
		    if ((value != 0) && (value != 1)) {
			(void) fprintf(siserr, "Some latch has the value 'don't care' or 'undefined'.\n");
			(void) fprintf(siserr, "Use set_state to put the network in a valid state.\n");
			array_free(in_value);
			lsFinish(gen);
			return 1;
		    }
		    array_insert_last(int, in_value, value);
		} else {
		    if (strcmp(argv[i], "0") == 0) {
			array_insert_last(int, in_value, 0);
		    } else if (strcmp(argv[i], "1") == 0) {
			array_insert_last(int, in_value, 1);
		    } else {
			(void) fprintf(miserr,
				       "simulate network: bad value '%s' -- should be 0 or 1\n", argv[i]);
			array_free(in_value);
			lsFinish(gen);
			goto usage;
		    }
		    i++;
		}
	    }
	    node_vec = network_dfs(*network);
	    out_value = simulate_network(*network, node_vec, in_value);
	    j = 0;
	    control_table = st_init_table(st_ptrcmp, st_ptrhash);
	    foreach_primary_output(*network, gen, po) {
		value = array_fetch(int, out_value, j);
		(void) st_insert(control_table, (char *) po, (char *) value);
		j++;
	    }
	    j = 0;
	    foreach_primary_output(*network, gen, po) {
		if ((l = latch_from_node(po)) != NIL(latch_t)) {
		    value = array_fetch(int, out_value, j);
		    control = latch_get_control(l);
		    if (control == NIL(node_t)) {
			/* If no control node is specified, clock the latch. */
		        latch_set_current_value(l, value);
		    } else {
		        (void) st_lookup_int(control_table, (char *) control,
					     &control_value);
			if (control_value) {
		            latch_set_current_value(l, value);
			}
		    }
		}
		j++;
	    }
	    st_free_table(control_table);
	    array_free(node_vec);
	    dump_outputs(misout, *network, out_value);
	    array_free(in_value);
	    array_free(out_value);
	}
    }
    if (do_stg) {
	if ((stg = network_stg(*network)) != NIL(graph_t)) {
	    if (argc-util_optind != stg_get_num_inputs(stg)) {
		(void) fprintf(miserr, "simulate stg: stg has %d inputs; %d values were supplied.\n", stg_get_num_inputs(stg), argc-1);
		goto usage;
	    }
	    in_value = array_alloc(int, stg_get_num_inputs(stg));
	    for (i = util_optind; i < stg_get_num_inputs(stg)+util_optind; i++) {
		if (strcmp(argv[i], "0") == 0) {
		    array_insert_last(int, in_value, 0);
		} else if (strcmp(argv[i], "1") == 0) {
		    array_insert_last(int, in_value, 1);
		} else {
		    (void) fprintf(miserr, "simulate stg: bad value '%s' -- should be 0 or 1\n", argv[i]);
		    array_free(in_value);
		    goto usage;
		}
	    }
	    out_char = simulate_stg(stg, in_value, &state);
	    (void) fprintf(misout, "\nSTG simulation:\n");
	    if (state == NIL(vertex_t)) {
		(void) fprintf(misout, "Next state cannot be determined\n");
		array_free(out_char);
		array_free(in_value);
		return 0;
	    }
	    (void) fprintf(misout, "Outputs:");
	    for (i = 0; i < stg_get_num_outputs(stg); i++) {
		(void) fprintf(misout, " %c", array_fetch(char, out_char, i));
	    }
	    (void) fprintf(misout, "\nNext state: %s (%s)\n\n",
			   stg_get_state_name(state),
			   stg_get_state_encoding(state));
	    array_free(in_value);
	    array_free(out_char);
	}
    }
    return 0;

usage:
    (void) fprintf(miserr, "usage: simulate [-s] [-i] in1 in2 in3 ...\n");
    return 1;
}



static int
com_set_state(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int do_stg = 1;
    int do_logic = 1;
    char *state_name;
    char *temp;
    lsGen gen;
    latch_t *l;
    int encoding;
    int c;
    graph_t *stg;
    int i;
    vertex_t *v;

    if ((network_num_pi(*network) == 0) &&
	(network_stg(*network) == NIL(graph_t))) {
	return 0;
    }
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "is")) != EOF) {
	switch(c) {
	  case 'i':
	    do_stg = 0;
	    break;
	  case 's':
	    do_logic = 0;
	    break;
	  default:
	    goto usage;
	}
    }
    if (!do_stg && !do_logic) {
	do_stg = do_logic = 1;
    }
    if (do_logic && (network_num_pi(*network) != 0)) {
	encoding = 1;
    } else {
	encoding = 0;
    }
    if (argc-util_optind == 0) {
	foreach_latch(*network, gen, l) {
	    latch_set_current_value(l, latch_get_initial_value(l));
	}
	if ((stg = network_stg(*network)) != NIL(graph_t)) {
	    stg_reset(stg);
	}
	return 0;
    }
    if (argc-util_optind != 1) {
	(void) fprintf(miserr, "set_state: one state must be given\n");
	goto usage;
    }
    state_name = argv[util_optind];
    if (do_logic) {
	if (network_num_latch(*network) != 0) {
	    if (strlen(state_name) != network_num_latch(*network)) {
		(void) fprintf(miserr, "set_state: network had %d latches; %d values were supplied.\n", network_num_latch(*network), strlen(state_name));
		goto usage;
	    }
	    for (i = 0; i < strlen(state_name); i++) {
		if ((state_name[i] != '0') && (state_name[i] != '1')) {
		    (void) fprintf(miserr, "set_state: bad value '%c' -- should be 0 or 1\n", c);
		    goto usage;
		}
	    }
	    temp = state_name;
	    foreach_latch(*network, gen, l) {
		c = *state_name++;
		if (c == '0') {
		    latch_set_current_value(l, 0);
		} else if (c == '1') {
		    latch_set_current_value(l, 1);
		}
	    }
	    state_name = temp;
	}
    }
    if (do_stg) {
	if ((stg = network_stg(*network)) != NIL(graph_t)) {
	    if (encoding) {
		if ((v = stg_get_state_by_encoding(stg, state_name))
		    != NIL(vertex_t)) {
		    stg_set_current(stg, v);
		} else {
		    (void) fprintf(miserr, "set_state: state with encoding name %s not found in stg\n", state_name);
		    goto usage;
		}
	    } else {
		if ((v = stg_get_state_by_name(stg, state_name))
		    != NIL(vertex_t)) {
		    stg_set_current(stg, v);
		} else {
		    (void) fprintf(miserr, "set_state: state with symbolic name %s not found in stg\n", state_name);
		    goto usage;
		}
	    }
	}
    }
    return 0;

usage:
    (void) fprintf(miserr, "usage: set_state [-s] [-i] [state_name]\n");
    return 1;
}
	
	    
/*ARGSUSED*/
static int
com_print_state(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    lsGen gen;
    latch_t *l;
    graph_t *stg;
    vertex_t *v;

    if (argc != 1) {
	goto usage;
    }
    (void) fprintf(misout, "\n");
    if (network_num_pi(*network) != 0) {
	(void) fprintf(misout, "Network state: ");
	foreach_latch(*network, gen, l) {
	    (void) fprintf(misout, "%d", latch_get_current_value(l));
	}
	(void) fprintf(misout, "\n\n");
    }
    if ((stg = network_stg(*network)) != NIL(graph_t)) {
	(void) fprintf(misout, "STG state: ");
	v = stg_get_current(stg);
	(void) fprintf(misout, "%s (%s)\n\n", stg_get_state_name(v),
		       stg_get_state_encoding(v));
    }
    return 0;
usage:
    (void) fprintf(miserr, "usage:  print_state\n");
    return 1;
}

#else

static int
dump_vector(fp, vec)
FILE *fp;
array_t *vec;
{
    int i;

    for(i = 0; i < array_n(vec); i++) {
       (void) fprintf(fp, " %d", array_fetch(int, vec, i));
    }
    (void) fprintf(fp, "\n");
}



static int
com_simulate(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int i;
    array_t *in_value, *out_value, *node_vec;

    if (argc - 1 != network_num_pi(*network)) {
	(void) fprintf(miserr, 
	    "simulate: network has %d inputs; %d values were supplied.\n",
	    network_num_pi(*network), argc - 1);
	goto usage;
    }

    in_value = array_alloc(int, 10);
    for(i = 1; i < argc; i++) {
	if (strcmp(argv[i], "0") == 0) {
	    array_insert_last(int, in_value, 0);
	} else if (strcmp(argv[i], "1") == 0) {
	    array_insert_last(int, in_value, 1);
	} else {
	    (void) fprintf(miserr, 
		"simulate: bad value '%s' -- should be 0 or 1\n", argv[i]);
	    array_free(in_value);
	    goto usage;
	}
    }

#if 1
    node_vec = network_dfs(*network);
    out_value = simulate_network(*network, node_vec, in_value);
    array_free(node_vec);
#else
    {
	int i, j;
	network_t *net1;
	node_t *node;
	lsGen gen;

	net1 = network_dup(*network);

	i = 0;
	foreach_primary_input(net1, gen, node) {
	    j = array_fetch(int, in_value, i++);
	    node_replace(node, j ? node_constant(1) : node_constant(0));
	}
	(void) network_collapse(net1);
	out_value = array_alloc(int, 0);
	foreach_primary_output(net1, gen, node) {
	    j = node_function(node_get_fanin(node, 0)) == NODE_1;
	    array_insert_last(int, out_value, j);
	}
	network_free(net1);
    }
#endif

    dump_vector(misout, out_value);

    array_free(in_value);
    array_free(out_value);
    return 0;

usage:
    (void) fprintf(miserr, "usage: simulate in1 in2 in3 ...\n");
    return 1;
}

#endif


static int
com_sim_verify(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
  int c;
  int n_patterns = 1024;
  network_t *network1;
  char buffer[1024];
  int status;

  util_getopt_reset();
  while ((c = util_getopt(argc, argv, "n:")) != EOF) {
    switch (c) {
    case 'n':
      n_patterns = atoi(util_optarg);
      if (n_patterns < 1) goto usage;
      if (n_patterns % 32 != 0)
	n_patterns += (32 - (n_patterns % 32));
      break;
    default:
      goto usage;
    }
  }
  if (argc - util_optind != 1) goto usage;
  (void) sprintf(buffer, "read_blif %s", argv[util_optind]);
  network1 = network_alloc();
  if (com_execute(&network1, buffer) != 0) {
    network_free(network1);
    return 1;
  }
  if (network_num_pi(network1) != network_num_pi(*network)) {
    (void) fprintf(miserr, "Number of inputs do not agree\n");
    network_free(network1);
    return 1;
  }
  if (network_num_po(network1) != network_num_po(*network)) {
    (void) fprintf(miserr, "Number of outputs do not agree\n");
    network_free(network1);
    return 1;
  }

  status = sim_verify(*network, network1, n_patterns);
  network_free(network1);
  return status;

 usage:
  (void) fprintf(miserr, "usage: sim_verify [-n n_patterns] network2.blif\n");
  return 1;
}

init_sim()
{
    com_add_command("simulate", com_simulate, 0);
#ifdef SIS
    com_add_command("set_state", com_set_state, 1);
    com_add_command("print_state", com_print_state, 0);
#endif
    com_add_command("sim_verify", com_sim_verify, 0);
}


end_sim()
{
}
