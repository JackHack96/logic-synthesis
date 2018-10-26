
#include "sis.h"
#include "atpg_int.h"

void
free_bdds_in_array(bdd_array)
array_t *bdd_array;
{
    int i;
    bdd_t *fn;

    for (i = 0; i < array_n(bdd_array); i++) {
    	fn = array_fetch(bdd_t *, bdd_array, i);
    	bdd_free(fn);
    }
}

network_t *
convert_bdd_to_network(seq_info, fn)
seq_info_t *seq_info;
bdd_t *fn;
{
    verif_options_t *options = seq_info->seq_opt;
    network_t *result;
    st_table *leaves = options->output_info->pi_ordering;
    array_t *var_names;
    st_generator *gen;
    node_t *node;
    int index;

    var_names = array_alloc(char *, st_count(leaves));
    st_foreach_item_int(leaves, gen, (char **) &node, &index) {
	array_insert(char *, var_names, index, node->name);
    }
    result = ntbdd_bdd_single_to_network(fn, "bdd_out", var_names);
    eliminate(result, 0, 10000);
    assert(network_num_po(result) == 1);
    array_free(var_names);
    return result;
}

/* This procedure converts the first cube of the input bdd into an integer.
 * Don't cares (i.e. 2's) are set to 0.
 */
int
convert_bdd_to_int(state_fn)
bdd_t *state_fn;
{
    int state_int;
    int i, cube_length, current_lit;
    array_t *cube;
    bdd_gen *gen;
    
    state_int = 0;
    gen = bdd_first_cube(state_fn, &cube);
    cube_length = array_n(cube);
    for (i = 0; i < cube_length; i++) {
	current_lit = array_fetch(bdd_literal, cube, i);
	if (current_lit != 2) {
	    state_int += current_lit << i;
	}
    }
    (void) bdd_gen_free(gen);
    return state_int;
}

int
convert_product_bdd_to_key(info, seq_info, state_fn)
atpg_info_t *info;
seq_info_t *seq_info;
bdd_t *state_fn;
{
    bdd_gen *gen;
    array_t *cube;
    int state_int, i, position_in_cube, position_in_key;
    array_t *latch_to_product_pi_ordering = seq_info->latch_to_product_pi_ordering;
    array_t *latch_to_pi_ordering = seq_info->latch_to_pi_ordering;
    lsGen latch_gen;
    latch_t *l;
    bdd_literal current_lit;

    state_int = 0;
    gen = bdd_first_cube(state_fn, &cube);
    i = 0;
    foreach_latch(info->network, latch_gen, l) {
	position_in_cube = array_fetch(int, latch_to_product_pi_ordering, i);
	current_lit = array_fetch(bdd_literal, cube, position_in_cube);
	position_in_key = array_fetch(int, latch_to_pi_ordering, i ++);
	if (current_lit != 2) {
	    state_int += current_lit << position_in_key;
	}
    }
    (void) bdd_gen_free(gen);
    return state_int;
}

st_table *
get_pi_to_var_table(manager, info)
bdd_manager *manager;
output_info_t *info;
{
  int index;
  node_t *pi;
  bdd_t *var;
  st_table *pi_to_var_table;
  st_generator *gen;

  pi_to_var_table = st_init_table(st_ptrcmp, st_ptrhash);
  st_foreach_item_int(info->pi_ordering, gen, (char **) &pi, &index) {
    var = ntbdd_node_to_local_bdd(pi, manager,  info->pi_ordering);
    st_insert(pi_to_var_table, (char *) pi, (char *) var);
  }
  return pi_to_var_table;
}

bdd_t *
seq_get_one_minterm(manager, fn, vars)
bdd_manager *manager;
bdd_t *fn;
st_table *vars;
{
/*    bdd_gen *gen;
    array_t *cube;
    array_t *latch_settings = seq_info->good_state;
    int i, position_in_pi_ordering;
    array_t *latch_to_pi_ordering = seq_info->latch_to_pi_ordering;
    bdd_literal current_lit;
    bdd_t *minterm;

    assert(! bdd_is_tautology(fn, 0));
    gen = bdd_first_cube(fn, &cube);
    for (i = 0; i < n_latch; i++) {
	position_in_pi_ordering = array_fetch(int, latch_to_pi_ordering, i);
	current_lit = array_fetch(bdd_literal, cube, position_in_pi_ordering);
	if (current_lit == 2) current_lit = 0;
	array_insert(bdd_literal, latch_settings, i, current_lit);
    }
    minterm = convert_state_to_bdd(seq_info, latch_settings);
    return minterm;
*/

    int i;
    bdd_t *tmp;
    bdd_t *bdd_lit;
    bdd_t *minterm;
    bdd_gen *gen;
    int n_lits;
    bdd_literal *lits;
    array_t *cube;

    assert(! bdd_is_tautology(fn, 0));

    minterm = bdd_one(manager);
    gen = bdd_first_cube(fn, &cube);
    n_lits = array_n(cube);

    lits = ALLOC(bdd_literal, n_lits);
    for (i = 0; i < n_lits; i++) {
        lits[i] = array_fetch(bdd_literal, cube, i);
    }
    (void) bdd_gen_free(gen);

    for (i = 0; i < n_lits; i++) {
        if (! st_lookup(vars, (char *) i, NIL(char *))) {
            assert(lits[i] == 2);
            continue;
        }
        switch (lits[i]) {
          case 0:
          case 2:
            tmp = bdd_get_variable(manager, i);
            bdd_lit = bdd_not(tmp);
            bdd_free(tmp);
            break;
          case 1:
            bdd_lit = bdd_get_variable(manager, i);
            break;
            default:
            fail("unexpected literal in get_one_minterm");
            break;
        }
        tmp = bdd_and(minterm, bdd_lit, 1, 1);
        bdd_free(minterm);
        bdd_free(bdd_lit);
        minterm = tmp;
    }
    FREE(lits);
    return minterm;
}

bdd_t *
find_good_constraint(current_set, total_set, cofactor_fn)
bdd_t *current_set;
bdd_t *total_set;
BddFn cofactor_fn;
{
  bdd_t *care_set = bdd_not(total_set);
  bdd_t *result = (*cofactor_fn)(current_set, care_set);
  bdd_free(care_set);
  return result;
}


void
use_cofactored_set(current_set, total_set, cofactor_fn)
bdd_t **current_set;
bdd_t **total_set;
BddFn cofactor_fn;
{
  bdd_t *new_total_set, *new_current_set;

  new_current_set = find_good_constraint(*current_set, *total_set, cofactor_fn);
  new_total_set = bdd_or(*current_set, *total_set, 1, 1);
  bdd_free(*current_set);
  bdd_free(*total_set);
  *total_set = new_total_set;
  *current_set = new_current_set;
}

void
bdd_add_varids_to_table(vars, table)
array_t *vars;
st_table *table;
{
    int i;
    int varid;
    array_t *ids;

    ids = bdd_get_varids(vars);
    for (i = 0; i < array_n(vars); i++) {
        varid = array_fetch(int, ids, i);
        st_insert(table, (char *) varid, NIL(char));
    }
    array_free(ids);
}

bdd_t *
convert_states_to_product_bdd(seq_info, good_state, faulty_state)
seq_info_t *seq_info;
array_t *good_state;
array_t *faulty_state;
{
    network_t *product_network = seq_info->product_network;
    range_data_t *data = seq_info->product_range_data;
    verif_options_t *options = seq_info->seq_product_opt;
    bdd_t *init_state;
    int i, value, state_length;
    latch_t *latch;
    lsGen gen;
    node_t *init_node;
    node_t *n1, *n2, *n3;

    state_length = array_n(good_state);
    init_node = node_constant(1);
    i = 0;
    foreach_latch(product_network, gen, latch) {
	if (i < state_length) {
	    value = array_fetch(int, good_state, i);
	} else {
	    value = array_fetch(int, faulty_state, i - state_length);
	}
	i ++;
	switch (value) {
	    case 0:
	    case 1:
      		n1 = latch_get_output(latch);
      		n2 = node_literal(n1, value);
      		n3 = node_and(init_node, n2);
      		node_free(n2);
      		node_free(init_node);
      		init_node = n3;
		break;
	    case 2:
		break;
	    default:
		fail("latch values were not in set {0,1,2}");
		break;
	}
    }
    network_add_node(product_network, init_node);
    init_state = ntbdd_node_to_local_bdd(init_node, data->manager, 								 options->output_info->pi_ordering);
    network_delete_node(product_network, init_node);
    return init_state;
}

bdd_t *
convert_state_to_bdd(seq_info, state)
seq_info_t *seq_info;
array_t *state;
{
    network_t *network = seq_info->network_copy;
    range_data_t *data = seq_info->range_data;
    verif_options_t *options = seq_info->seq_opt;
    bdd_t *init_state;
    int i, value;
    latch_t *latch;
    lsGen gen;
    node_t *init_node;
    node_t *n1, *n2, *n3;

    init_node = node_constant(1);
    i = 0;
    foreach_latch(network, gen, latch) {
	value = array_fetch(int, state, i++);
	switch (value) {
	    case 0:
	    case 1:
      		n1 = latch_get_output(latch);
      		n2 = node_literal(n1, value);
      		n3 = node_and(init_node, n2);
      		node_free(n2);
      		node_free(init_node);
      		init_node = n3;
		break;
	    case 2:
		break;
	    default:
		fail("latch values were not in set {0,1,2}");
		break;
	}
    }
    network_add_node(network, init_node);
    init_state = ntbdd_node_to_local_bdd(init_node, data->manager, 								 options->output_info->pi_ordering);
    network_delete_node(network, init_node);
    return init_state;
}

int
derive_prop_key(seq_info)
seq_info_t *seq_info;
{
    array_t *good_state = seq_info->good_state;
    array_t *faulty_state = seq_info->faulty_state;
    int state_length, i, gval, fval, good_state_int, faulty_state_int, key;

    state_length = array_n(good_state);
    good_state_int = faulty_state_int = 0;

    for (i = 0; i < state_length; i++) {
	gval = array_fetch(int, good_state, i);
	fval = array_fetch(int, faulty_state, i);
	if (ATPG_DEBUG) {
	    assert ((gval == 0) || (gval == 1));
	    assert ((fval == 0) || (fval == 1));
	}
	good_state_int += gval << i;
	faulty_state_int += fval << i;
    }
    key = (good_state_int << state_length) + faulty_state_int;
    return key;
}

int
derive_inverted_prop_key(seq_info)
seq_info_t *seq_info;
{
    array_t *good_state = seq_info->good_state;
    array_t *faulty_state = seq_info->faulty_state;
    int state_length, i, gval, fval, good_state_int, faulty_state_int, key;

    state_length = array_n(good_state);
    good_state_int = faulty_state_int = 0;

    for (i = 0; i < state_length; i++) {
	gval = array_fetch(int, good_state, i);
	fval = array_fetch(int, faulty_state, i);
	if (ATPG_DEBUG) {
	    assert ((gval == 0) || (gval == 1));
	    assert ((fval == 0) || (fval == 1));
	}
	good_state_int += gval << i;
	faulty_state_int += fval << i;
    }
    key = (faulty_state_int << state_length) + good_state_int;
    return key;
}
