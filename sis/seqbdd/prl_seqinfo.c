

#ifdef SIS
#include "sis.h"
#include "prl_util.h"

static void seq_compute_init_state ARGS((seq_info_t *));
static void seq_build_dc_bdds ARGS((seq_info_t *));
static void seq_build_output_bdds ARGS((seq_info_t *));

/*
 *----------------------------------------------------------------------
 *
 * Prl_SeqInitNetwork -- EXPORTED ROUTINE
 *
 * Allocates and initializes a 'seq_info' structure.
 * That structure contains all the information required to perform
 * traversal of a sequential circuits using BDDs. Even if there is
 * no 'dc_network' attached to 'network', we allocate a 'dc_map' table.
 * A 'dc_map' maps:
 *	1. 'network'    PO to 'dc_network' PO
 *	2. 'dc_network' PI to 'network'    PI
 *
 * Results:
 *	the 'seq_info' structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */ 

seq_info_t *Prl_SeqInitNetwork(network, options)
network_t *network;
prl_options_t *options;
{
  seq_info_t *seq_info = ALLOC(seq_info_t, 1);

  seq_info->network = network;
  seq_info->dc_map = attach_dcnetwork_to_network(network);
  if (seq_info->dc_map == NIL(st_table)) {
      seq_info->dc_map = st_init_table(st_ptrcmp, st_ptrhash);
  }

  (*options->bdd_order)(seq_info, options);

  seq_build_output_bdds(seq_info);
  seq_build_dc_bdds(seq_info);
  seq_compute_init_state(seq_info);

 /* command specific initialization (whatever remains to be done) */
  (*options->init_seq_info)(seq_info, options);

 /* a few consistency checks */
  assert(array_n(seq_info->next_state_fns) == network_num_latch(seq_info->network));
  assert(array_n(seq_info->next_state_fns) == array_n(seq_info->present_state_vars));
  assert(array_n(seq_info->output_nodes) == network_num_po(seq_info->network));
  assert(array_n(seq_info->output_nodes) == array_n(seq_info->next_state_fns) + array_n(seq_info->external_output_fns));
  assert(array_n(seq_info->present_state_vars) == array_n(seq_info->next_state_fns));
  assert(array_n(seq_info->input_vars) == network_num_pi(seq_info->network));
  assert(array_n(seq_info->input_nodes) == array_n(seq_info->input_vars));

  return seq_info;
}


/*
 *----------------------------------------------------------------------
 *
 * seq_build_output_bdds -- INTERNAL ROUTINE
 *
 *----------------------------------------------------------------------
 */

static void seq_build_output_bdds(seq_info)
seq_info_t *seq_info;
{
  int i;
  bdd_t *fn;
  node_t *output;

  seq_info->external_output_fns = array_alloc(bdd_t *, 0);
  seq_info->next_state_fns = array_alloc(bdd_t *, 0);

  for (i = 0; i < array_n(seq_info->output_nodes); i++) {
    output = array_fetch(node_t *, seq_info->output_nodes, i);
    fn = ntbdd_node_to_bdd(output, seq_info->manager, seq_info->leaves);
    if (network_is_real_po(seq_info->network, output)) {
      array_insert_last(bdd_t *, seq_info->external_output_fns, fn);
    } else {
      array_insert_last(bdd_t *, seq_info->next_state_fns, fn);
    }
  }
}


static void seq_register_pis_as_bdd_inputs ARGS((seq_info_t *, network_t *));

/*
 *----------------------------------------------------------------------
 *
 * seq_build_dc_bdds -- INTERNAL ROUTINE
 *
 *----------------------------------------------------------------------
 */

static void seq_build_dc_bdds(seq_info)
seq_info_t *seq_info;
{
    int i;
    lsGen gen;
    char *output_name;
    node_t *input;
    node_t *dc_output, *output;
    network_t *dc_network;
    bdd_t *dc_fn;

    seq_info->next_state_dc = array_alloc(bdd_t *, 0);
    seq_info->external_output_dc = array_alloc(bdd_t *, 0);
		 
    dc_network = seq_info->network->dc_network;
    seq_register_pis_as_bdd_inputs(seq_info, dc_network);

    for (i = 0; i < array_n(seq_info->output_nodes); i++) {
	output = array_fetch(node_t *, seq_info->output_nodes, i);
	/* if no don't care for that node, put a 0 BDD */
	if (st_lookup(seq_info->dc_map, (char *) output, (char **) &dc_output)) {
	    dc_fn = ntbdd_node_to_bdd(dc_output, seq_info->manager, seq_info->leaves);
	} else {
	    dc_fn = bdd_zero(seq_info->manager);
	}
	/* store BDDs in the same order as for the main network */
	if (network_is_real_po(seq_info->network, output)) {
	    array_insert_last(bdd_t *, seq_info->external_output_dc, dc_fn);
	} else {
	    array_insert_last(bdd_t *, seq_info->next_state_dc, dc_fn);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * seq_register_pis_as_bdd_inputs -- INTERNAL ROUTINE
 *
 * Goes through the PIs of the dc_network.
 * The PIs should appear in seq_info->dc_map and be mapped
 * to PIs of the original network.
 * Store a mapping between the dc_network PIs and the bdd_t *vars
 * so that seq_info->leaves can be used to build BDD's over the dc_networks.
 *
 *----------------------------------------------------------------------
 */

static void seq_register_pis_as_bdd_inputs(seq_info, dc_network)
seq_info_t *seq_info;
network_t *dc_network;
{
    int var_index;
    lsGen gen;
    node_t *dc_input;
    node_t *input;

    if (dc_network == NIL(network_t)) return;
    foreach_primary_input(dc_network, gen, dc_input) {
	assert(st_lookup(seq_info->dc_map, (char *) dc_input, (char **) &input));
	assert(st_lookup_int(seq_info->leaves, (char *) input, &var_index));
	st_insert(seq_info->leaves, (char *) dc_input, (char *) var_index);
    }
}


/*
 *----------------------------------------------------------------------
 *
 * seq_compute_initial_state -- INTERNAL ROUTINE
 *
 *----------------------------------------------------------------------
 */

static  void seq_compute_init_state(seq_info)
seq_info_t *seq_info;
{
  lsGen gen;
  latch_t *latch;
  node_t *pi;
  int var_index;
  int init_value;
  bdd_t *tmp, *var;
  int warning_done = 0;
  bdd_t *result = bdd_one(seq_info->manager);
  network_t *network = seq_info->network;

  foreach_latch(network, gen, latch) {
 /* 
  *  first, compute the initial value. If 2, specifying it: -> get a cube, not a minterm
  */
    init_value = latch_get_initial_value(latch);
    assert(init_value >= 0 && init_value <= 3);
    if (init_value == 2) continue;
    if (init_value == 3) {
      init_value = 0;
      if (! warning_done) {
	warning_done = 1;
	pi = latch_get_input(latch);
	(void) fprintf(siserr, "WARNING: unspecified init value of node %s set to 0\n", node_long_name(pi));
      }
    }

 /* 
  *  then compute the literal to AND with the current result
  */
    pi = latch_get_output(latch);
    assert(st_lookup_int(seq_info->leaves, (char *) pi, &var_index));
    var = bdd_get_variable(seq_info->manager, var_index);

    tmp = bdd_and(result, var, 1, init_value);
    bdd_free(var);
    bdd_free(result);
    result = tmp;
  }

  seq_info->init_state_fn = result;
}


/*
 *----------------------------------------------------------------------
 *
 * Prl_SeqInfoFree -- EXPORTED ROUTINE
 *
 * Side effects:
 *	Frees the 'seq_info' record.
 *
 *----------------------------------------------------------------------
 */ 

void Prl_SeqInfoFree(seq_info, options)
seq_info_t *seq_info;
prl_options_t *options;
{
  int i;
  char *name;

 /* don't free those BDD's: stored at nodes: will be done by ntbdd_end_manager */
  array_free(seq_info->next_state_fns);
  array_free(seq_info->external_output_fns);
  array_free(seq_info->output_nodes);
  
  Prl_FreeBddArray(seq_info->present_state_vars);
  Prl_FreeBddArray(seq_info->external_input_vars);
  Prl_FreeBddArray(seq_info->input_vars);
  array_free(seq_info->input_nodes);
  for (i = 0; i < array_n(seq_info->var_names); i++) {
    name = array_fetch(char *, seq_info->var_names, i);
    FREE(name);
  }
  array_free(seq_info->var_names);
  
  array_free(seq_info->next_state_dc);
  array_free(seq_info->external_output_dc);
  
  (*options->free_seq_info)(seq_info, options);

  st_free_table(seq_info->leaves);
  ntbdd_end_manager(seq_info->manager);

  st_free_table(seq_info->dc_map);
  
  FREE(seq_info);
}


/*
 *----------------------------------------------------------------------
 *
 * Prl_ExtractReachableStates -- EXPORTED ROUTINE
 *
 *----------------------------------------------------------------------
 */ 

bdd_t *Prl_ExtractReachableStates(seq_info, options)
seq_info_t *seq_info;
prl_options_t *options;
{
    bdd_t *new_current_set;
    bdd_t *new_total_set;
    bdd_t *current_set;
    bdd_t *total_set;
    bdd_t *care_set;
    bdd_t *new_states;
    double total_onset, new_onset;

    Prl_ReportElapsedTime(options, "build BDDs");
    current_set = bdd_dup(seq_info->init_state_fn);
    total_set = bdd_dup(current_set);
    Prl_ReportElapsedTime(options, "begin STG traversal");
    for (;;) {
	new_current_set = (*options->compute_next_states)(current_set, seq_info, options);
	bdd_free(current_set);
	if (options->verbose >= 1) {
	    total_onset = bdd_count_onset(total_set, seq_info->present_state_vars);
	    new_states = bdd_and(new_current_set, total_set, 1, 0);
	    new_onset = bdd_count_onset(new_states, seq_info->present_state_vars);
	    bdd_free(new_states);
	    (void) fprintf(sisout, "add %2.0f states to %2.0f states\n", new_onset, total_onset);
	}
	if (bdd_leq(new_current_set, total_set, 1, 1)) break;
	care_set = bdd_not(total_set);
	current_set = bdd_cofactor(new_current_set, care_set);
	bdd_free(care_set);
	new_total_set = bdd_or(new_current_set, total_set, 1, 1);
	bdd_free(new_current_set);
	bdd_free(total_set);
	total_set = new_total_set;
    }
    bdd_free(new_current_set);
    Prl_ReportElapsedTime(options, "end STG traversal");
    return total_set;
}

#endif /* SIS */
