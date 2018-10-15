/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/seqbdd/consistency.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:54 $
 *
 */
 /* file %M% release %I% */
 /* last modified: %G% at %U% */
#ifdef SIS
#include "sis.h"


 /* EXTERNAL INTERFACE */

 /* ARGSUSED */
range_data_t *consistency_alloc_range_data(network, options)
network_t *network;
verif_options_t *options;
{
  st_table *input_to_output_table;
  output_info_t *info = options->output_info;
  range_data_t *data = ALLOC(range_data_t, 1);

  data->type = CONSISTENCY_METHOD;

 /* create a BDD manager */
  if (options->verbose >= 3) print_node_table(info->pi_ordering);
  data->manager = ntbdd_start_manager(st_count(info->pi_ordering));

 /* create BDD for external output_fn and init_state_fn */
  data->init_state_fn = ntbdd_node_to_bdd(info->init_node, data->manager, info->pi_ordering);
  if (options->does_verification) {
    data->output_fn = ntbdd_node_to_bdd(info->output_node, data->manager, info->pi_ordering);
  } else {
    data->output_fn = NIL(bdd_t);
  }

  /* consistency fn and the like */
  data->consistency_fn = ntbdd_node_to_bdd(info->main_node, data->manager, info->pi_ordering);
  data->smoothing_inputs = bdd_extract_var_array(data->manager, info->org_pi, info->pi_ordering);

  
 /* create and save two arrays of bdd variables in order: yi's and their corresponding xi's */
  input_to_output_table = extract_input_to_output_table(info->org_pi, info->new_pi, info->po_ordering, network);
  data->input_vars = extract_state_input_vars(data->manager, info->pi_ordering, input_to_output_table);
  data->output_vars = extract_state_output_vars(data->manager, info->pi_ordering, input_to_output_table);

 /* only present state variables: used for counting the onset of reached set */
  data->pi_inputs = data->input_vars;

 /* free space allocated here */
  st_free_table(input_to_output_table);

  return data;
}

void consistency_free_range_data(data, options)
range_data_t *data;
verif_options_t *options;
{
  output_info_t *info = options->output_info;

 /* do not free data->pi_inputs (== data->input_vars) */
  array_free(data->input_vars);
  array_free(data->output_vars);
  array_free(data->smoothing_inputs);
  if (options->does_verification) {
    ntbdd_free_at_node(info->output_node);
  }
  ntbdd_free_at_node(info->init_node);
  ntbdd_free_at_node(info->main_node);
  ntbdd_end_manager(data->manager);
}

 /* ARGSUSED */
bdd_t *consistency_compute_next_states(current_set, data, options)
bdd_t *current_set;
range_data_t *data;
verif_options_t *options;
{
  bdd_t *result_as_output;
  bdd_t *new_current_set;

  result_as_output = bdd_and_smooth(data->consistency_fn, current_set, data->smoothing_inputs);
  new_current_set = bdd_substitute(result_as_output, data->output_vars, data->input_vars);
  bdd_free(result_as_output);
  return new_current_set;
}
 /* ARGSUSED */
bdd_t *consistency_compute_reverse_image(next_set, data, options)
bdd_t *next_set;
range_data_t *data;
verif_options_t *options;
{
  bdd_t *next_set_as_next_state_vars;
  bdd_t *result;

  next_set_as_next_state_vars = bdd_substitute(next_set, data->input_vars, data->output_vars);
  result = bdd_and_smooth(data->consistency_fn, next_set_as_next_state_vars, data->output_vars);
  bdd_free(next_set_as_next_state_vars);
  return result;
}

 /* returns 1 iff everything is OK */
int consistency_check_output(current_set, data, index, options)
bdd_t *current_set;
range_data_t *data;
int *index;
verif_options_t *options;
{
  if (bdd_leq(current_set, data->output_fn, 1, 1)) return 1;
  report_inconsistency(current_set, data->output_fn, options->output_info->pi_ordering);
  return 0;
}

void consistency_bdd_sizes(data, fn_size, output_size)
range_data_t *data;
int *fn_size;
int *output_size;
{
  if (fn_size) {
    *fn_size = bdd_size(data->consistency_fn);
  }
  if (output_size) {
    if (data->output_fn == NIL(bdd_t)) {
      *output_size = 0;
    } else {
      *output_size = bdd_size(data->output_fn);
    }
  }
}


 /* EXPORTED UTILITIES */

 /* returns a table that takes a state input node as key, and returns the new primary input */
 /* associated with the state output node which matches the key */
 /* nodes in org_pi, new_pi and po_ordering belong to the main network */
 /* they have to be matched by name */

st_table *extract_input_to_output_table(org_pi, new_pi, po_ordering, network)
array_t *org_pi;
array_t *new_pi;
array_t *po_ordering;
network_t *network;
{
  int i, index;
  lsGen gen;
  latch_t *l;
  node_t *input, *output;
  st_table *name_table = st_init_table(strcmp,st_strhash);
  st_table *index_table = st_init_table(strcmp,st_strhash);
  st_table *result = st_init_table(st_ptrcmp, st_ptrhash);

 /* index_table: state PO name --> index in po_ordering */
 /* (orderings in po_ordering and new_pi are the same) */
  for (i = 0; i < array_n(po_ordering); i++) {
    output = array_fetch(node_t *, po_ordering, i);
    st_insert(index_table, output->name, (char *) i);
  }

 /* name_table: state PI name --> index of matching state PO */
  foreach_latch(network, gen, l){
/* output is a primary output of the network and input to latch*/
	output = latch_get_input(l);
/*input is a primary input of the circuit and output of latch*/
	input = latch_get_output(l);
    assert(input->type == PRIMARY_INPUT);
    assert(st_lookup_int(index_table, output->name, &index));
    st_insert(name_table, input->name, (char *) index);
  }

  for (i = 0; i < array_n(org_pi); i++) {
    node_t *ps_node = array_fetch(node_t *, org_pi, i);
    if (! st_lookup_int(name_table, ps_node->name, &index)) continue;
    input = array_fetch(node_t *, new_pi, index);
    st_insert(result, (char *) ps_node, (char *) input);
  }
  st_free_table(name_table);
  st_free_table(index_table);
  return result;
}


 /* the order of appearance in pi_ordering */
array_t *extract_state_input_vars(manager, pi_ordering, ito_table)
bdd_manager *manager;
st_table *pi_ordering;
st_table *ito_table;
{
  int index;
  st_generator *gen;
  node_t *ps_node, *ns_node;
  array_t *tmp = array_alloc(node_t *, 0);
  array_t *result;

  st_foreach_item_int(pi_ordering, gen, (char **) &ps_node, &index) {
    if (! st_lookup(ito_table, (char *) ps_node, (char **) &ns_node)) ps_node = NIL(node_t);
    array_insert(node_t *, tmp, index, ps_node);
  }
  result = bdd_extract_var_array(manager, tmp, pi_ordering);
  array_free(tmp);
  return result;
}

 /* the order of appearance in pi_ordering */
array_t *extract_state_output_vars(manager, pi_ordering, ito_table)
bdd_manager *manager;
st_table *pi_ordering;
st_table *ito_table;
{
  int index;
  st_generator *gen;
  node_t *ps_node, *ns_node;
  array_t *tmp = array_alloc(node_t *, 0);
  array_t *result;

  st_foreach_item_int(pi_ordering, gen, (char **) &ps_node, &index) {
    if (! st_lookup(ito_table, (char *) ps_node, (char **) &ns_node)) ns_node = NIL(node_t);
    array_insert(node_t *, tmp, index, ns_node);
  }
  result = bdd_extract_var_array(manager, tmp, pi_ordering);
  array_free(tmp);
  return result;
}
#endif /* SIS */

