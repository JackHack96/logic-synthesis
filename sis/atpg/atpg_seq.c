/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/atpg/atpg_seq.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:17 $
 *
 */
#include "sis.h"
#include "atpg_int.h"

typedef struct {
    int which_case;
    node_t *faulty_node;
    node_t *faulty_fanin;
    node_t *orig_faulty_node;
    node_t *orig_faulty_fanin;
    node_t *const_node;
    int n_fanout;
    node_t **fanouts;
} saved_nodes_t;

static void set_options();
static void reverse_generate_Sj();
static void convert_trace_to_sequence();
static int seq_check_output();
static void insert_fault();
static void remove_fault();
static void add_minterms_to_product_start_states();

void
seq_setup(seq_info, info)
seq_info_t *seq_info;
atpg_info_t *info;
{
    range_data_t *data;
    verif_options_t *options = seq_info->seq_opt;
    network_t *network = info->network;
    lsGen gen;
    latch_t *l;
    node_t *pi;
    st_table *pi_ordering;
    int i, position;

    seq_info->network_copy = network_dup(network);
    seq_info->good_state = array_alloc(int, info->n_latch);
    seq_info->faulty_state = array_alloc(int, info->n_latch);
    seq_info->just_sequence_table = st_init_table(st_numcmp, st_numhash);
    seq_info->prop_sequence_table = st_init_table(st_numcmp, st_numhash);
    set_options(options);
    options->does_verification = 0;
    output_info_init(options->output_info);
    extract_network_info(seq_info->network_copy, options);
    assert(options->output_info->init_node != NIL(node_t));
    data = product_alloc_range_data(seq_info->network_copy, options);
    data->total_set = bdd_zero(data->manager);
    assert(data->init_state_fn != NIL(bdd_t));
    seq_info->range_data = data;

    seq_info->latch_to_pi_ordering = array_alloc(int, info->n_latch);
    pi_ordering = options->output_info->pi_ordering;
    i = 0;
    foreach_latch(seq_info->network_copy, gen, l) {
	pi = latch_get_output(l);
	assert(st_lookup_int(pi_ordering, (char *) pi, &position));
	array_insert(int, seq_info->latch_to_pi_ordering, i ++, position);
    }

    if (info->atpg_opt->use_internal_states) {
    	seq_info->state_sequence_table = st_init_table(st_numcmp, st_numhash);
    }
}

void
seq_product_setup(info, seq_info, network)
atpg_info_t *info;
seq_info_t *seq_info;
network_t *network;
{
    network_t *network_copy;
    range_data_t *data;
    verif_options_t *options;
    st_table *product_pi_ordering;
    int i, position;
    lsGen gen;
    latch_t *l;
    node_t *pi;

    seq_info->product_network = network_dup(network);
    options = seq_info->seq_product_opt;
    network_copy = network_dup(seq_info->product_network);
    set_options(options);
    options->does_verification = 1;
    output_info_init(options->output_info);
    options->output_info->is_product_network = 1;
    extract_product_network_info(network_copy, seq_info->product_network, options);
    network_free(network_copy);
    assert(options->output_info->init_node != NIL(node_t));
    data = product_alloc_range_data(seq_info->product_network, options);
    assert(data->init_state_fn != NIL(bdd_t));
    seq_info->product_range_data = data;
    if (info->atpg_opt->use_internal_states) {
	seq_info->latch_to_product_pi_ordering = array_alloc(int, info->n_latch);
	product_pi_ordering = options->output_info->pi_ordering;
	i = 0;
	foreach_latch(seq_info->product_network, gen, l) {
	    pi = latch_get_output(l);
	    assert(st_lookup_int(product_pi_ordering, (char *) pi, &position));
	    if (i < info->n_latch) {
	    	array_insert(int, seq_info->latch_to_product_pi_ordering, i ++, position);
	    }
	}
    }
}

static void
set_options(options)
verif_options_t *options;
{
    options->verbose = 0;
    options->ordering_depth = 0;
    options->n_iter = INFINITY;
    options->timeout = 0;
    options->keep_old_network = 1;
    options->type = PRODUCT_METHOD;
    options->alloc_range_data = product_alloc_range_data;
    options->compute_next_states = product_compute_next_states;
    options->compute_reverse_image = product_compute_reverse_image;
    options->free_range_data = product_free_range_data;
    options->check_output = product_check_output;
    options->bdd_sizes = product_bdd_sizes;
    options->use_manual_order = 0;    
    options->order_network = NIL(network_t);
    options->last_time = util_cpu_time();
    options->total_time = 0;
    options->sim_file = NIL(char);
    return;
}

void
copy_orig_bdds(seq_info)
seq_info_t *seq_info;
{
    int i;
    range_data_t *range_data;
    array_t *ext_outputs;
    array_t *trans_outputs;
    bdd_t *copy;

    range_data = seq_info->product_range_data;
    ext_outputs = array_alloc(bdd_t *, array_n(range_data->external_outputs));
    trans_outputs = array_alloc(bdd_t *, array_n(range_data->transition_outputs));
    for (i = 0; i < array_n(range_data->external_outputs); i++) {
	copy = bdd_dup(array_fetch(bdd_t *, range_data->external_outputs, i));
      	array_insert(bdd_t *, ext_outputs, i, copy);
    }
    for (i = 0; i < array_n(range_data->transition_outputs); i++) {
	copy = bdd_dup(array_fetch(bdd_t *, range_data->transition_outputs, i));
      	array_insert(bdd_t *, trans_outputs, i, copy);
    }
    seq_info->orig_external_outputs = ext_outputs;
    seq_info->orig_transition_outputs = trans_outputs;
}

void
record_reset_state(seq_info, info)
seq_info_t *seq_info;
atpg_info_t *info;
{
    range_data_t *data = seq_info->range_data;
    array_t *just_sequence = array_alloc(unsigned *, 0);
    int key;
    lsList sequence_list;

    key = convert_bdd_to_int(data->init_state_fn);
    st_insert(seq_info->just_sequence_table, (char *) key, (char *) just_sequence);
    seq_info->justified_states = bdd_dup(data->init_state_fn);

    if (info->atpg_opt->use_internal_states) {
	seq_info->start_states = bdd_dup(data->init_state_fn);
	sequence_list = lsCreate();
    	st_insert(seq_info->state_sequence_table, (char *) key, 
		  (char *) sequence_list);
    }
}

/* convert the set of start_states into equivalent product_start_states */
void
construct_product_start_states(seq_info, n_latch)
seq_info_t *seq_info;
int n_latch;
{
    bdd_t *start_states = seq_info->start_states;
    array_t *latch_to_pi_ordering = seq_info->latch_to_pi_ordering;
    bdd_gen *gen;
    array_t *cube;
    int i, position_in_pi_ordering;
    bdd_literal current_lit;
    array_t *state_cube = seq_info->good_state;

    seq_info->product_start_states = bdd_zero(seq_info->product_range_data->manager);
    foreach_bdd_cube(start_states, gen, cube) {
	for (i = 0; i < n_latch; i++) {
	    position_in_pi_ordering = array_fetch(int, latch_to_pi_ordering, i);
	    current_lit = array_fetch(bdd_literal, cube, position_in_pi_ordering);
	    array_insert(bdd_literal, state_cube, i, current_lit);
	}
	add_minterms_to_product_start_states(seq_info, state_cube, n_latch);
    }
}

static void
add_minterms_to_product_start_states(seq_info, state_cube, n_to_check)
seq_info_t *seq_info;
array_t *state_cube;
int n_to_check;
{
    int i, val;
    bdd_t *minterm_bdd, *new_product_start_states;

    for (i = n_to_check; i --; ) {
	val = array_fetch(int, state_cube, i);
	if (val == 2) {
	    array_insert(int, state_cube, i, 0);
	    add_minterms_to_product_start_states(seq_info, state_cube, i);
	    array_insert(int, state_cube, i, 1);
	    add_minterms_to_product_start_states(seq_info, state_cube, i);
	}
    }
    minterm_bdd = convert_states_to_product_bdd(seq_info, state_cube, state_cube);
    new_product_start_states = bdd_or(seq_info->product_start_states, minterm_bdd, 1, 1);
    bdd_free(minterm_bdd);
    bdd_free(seq_info->product_start_states);
    seq_info->product_start_states = new_product_start_states;
}


bool
calculate_reachable_states(seq_info)
seq_info_t *seq_info;
{
    int i;   
    bdd_t *total_set, *reset_states;
    bdd_t *set_i;
    bdd_t *previous_set;
    range_data_t *range_data = seq_info->range_data;
    verif_options_t *options = seq_info->seq_opt;
    array_t *reached_sets = seq_info->reached_sets;

    total_set = bdd_zero(range_data->manager);
    reset_states = range_data->init_state_fn;

    for (i = 0; i < options->n_iter; i++) {
	if (i == 0) {
	    set_i = bdd_dup(reset_states);
	} else {
	    previous_set = bdd_dup(array_fetch(bdd_t *, reached_sets, i-1));
	    set_i = product_compute_next_states(previous_set, range_data, options);
	    bdd_free(previous_set);
	}
	if (bdd_leq(set_i, total_set, 1, 1)) {
	    range_data->total_set = total_set;
	    return TRUE;
	}
	use_cofactored_set(&set_i, &total_set, bdd_cofactor);
	array_insert(bdd_t *, reached_sets, i, set_i);
    }
    range_data->total_set = total_set;
    return FALSE;
}

bdd_t *
seq_derive_excitation_states(info, seq_info, n_pi_vars)
atpg_ss_info_t *info;
seq_info_t *seq_info;
int n_pi_vars;
{
    network_t *network_copy = seq_info->network_copy;
    array_t *input_vars = info->sat_input_vars;
    range_data_t *data = seq_info->range_data;
    verif_options_t *options = seq_info->seq_opt;
    bdd_t *excite_states;
    st_table *table;
    int i, value;
    node_t *pi;
    node_t *pi_in_copy;
    sat_input_t sat_var;
    lsGen gen;
    node_t *excite_node;
    node_t *n1, *n2, *n3;

    table = st_init_table(st_ptrcmp, st_ptrhash);
    /* unassigned PI's are set to 2 */
    foreach_primary_input(info->network, gen, pi) {
	(void) st_insert(table, (char *) pi, (char *) 2);
    }

    for (i = n_pi_vars; i-- ; ) {
	sat_var = array_fetch(sat_input_t, input_vars, i);
	(void) st_insert(table, sat_var.info, 
				(char *) sat_get_value(info->atpg_sat, sat_var.sat_id));
    }
    excite_node = node_constant(1);
    foreach_primary_input(info->network, gen, pi) {
	if ( (latch_from_node(pi)) != NIL(latch_t) ) {
	    assert(st_lookup_int(table, (char *) pi, &value));
	    pi_in_copy = network_find_node(network_copy, pi->name);
	    switch (value) {
		case 0:
		case 1:
      	    	    n1 = pi_in_copy;
      		    n2 = node_literal(n1, value);
      		    n3 = node_and(excite_node, n2);
      		    node_free(n2);
      		    node_free(excite_node);
      		    excite_node = n3;
	  	    break;
    		case 2:
	  	    break;
	    }
	}
    }
    network_add_node(network_copy, excite_node);
    excite_states = ntbdd_node_to_local_bdd(excite_node, data->manager, 									options->output_info->pi_ordering);
    network_delete_node(network_copy, excite_node);
    st_free_table(table);
    return excite_states;
}

int
seq_reuse_just_sequence(n_inputs, n_latch, seq_info, already_justified)
int n_inputs;
int n_latch;
seq_info_t *seq_info;
bdd_t *already_justified;
{
    range_data_t *range_data = seq_info->range_data;
    array_t *just_sequence = seq_info->just_sequence;
    bdd_t *minterm, *justified_excite_state;
    array_t *old_just_sequence;
    int i, j, seq_length;
    unsigned *vector, *old_vector;

    minterm = seq_get_one_minterm(range_data->manager,
                                     already_justified, seq_info->var_table);
    justified_excite_state = bdd_smooth(minterm, seq_info->real_pi_bdds);
    assert(st_lookup(seq_info->just_sequence_table,
                     (char *) convert_bdd_to_int(justified_excite_state),
                     (char **) &old_just_sequence));
    seq_length = array_n(old_just_sequence);
    if (ATPG_DEBUG) {
	if (array_n(old_just_sequence) == 0) 
	    assert(bdd_leq(justified_excite_state, range_data->init_state_fn, 1, 1));
    }
    for (i = seq_length; i -- ; ) {
	vector = array_fetch(unsigned *, just_sequence, i);
	old_vector = array_fetch(unsigned *, old_just_sequence, i);
	for (j = n_inputs; j -- ; ) {
	    vector[j] = old_vector[j];
	}
    }
    bdd_free(minterm);
    bdd_free(justified_excite_state);
    return seq_length;
}

int
seq_state_justify(info, seq_info, excite_states)
atpg_info_t *info;
seq_info_t *seq_info;
bdd_t *excite_states;
{
    int i, stg_depth;   
    bdd_t *reached_excite_states;
    array_t *reached_sets = seq_info->reached_sets;

    stg_depth = array_n(reached_sets);
    /* i starts at 1; if the excite states contain the reset state, then
     * the just_sequence was already generated by seq_reuse_just_sequence.
     */
    for (i = 1; i < stg_depth; i++ ) {
	reached_excite_states = bdd_and(excite_states, 
					array_fetch(bdd_t *, reached_sets, i), 1, 1);
	if (!(bdd_is_tautology(reached_excite_states, 0))) {
	    reverse_generate_Sj(info, seq_info, i, reached_excite_states);
	    bdd_free(reached_excite_states);
	    return i;
	}
	bdd_free(reached_excite_states);
    }
    /* shouldn't ever get to this line, since we checked previously that 
	excite_states were contained in total_set */
    return -1;
}

int
internal_states_seq_state_justify(info, seq_info, excite_states)
atpg_info_t *info;
seq_info_t *seq_info;
bdd_t *excite_states;
{
    int i;   
    bdd_t *total_set, *reset_states;
    bdd_t *set_i;
    bdd_t *previous_set;
    bdd_t *reached_excite_states;
    range_data_t *range_data = seq_info->range_data;
    verif_options_t *options = seq_info->seq_opt;
    array_t *reached_sets = seq_info->reached_sets;

    total_set = bdd_zero(range_data->manager);
    reset_states = seq_info->start_states;

    for (i = 0; i < options->n_iter; i++) {
	if (i == 0) {
	    set_i = bdd_dup(reset_states);
	} else {
	    previous_set = bdd_dup(array_fetch(bdd_t *, reached_sets, i-1));
	    set_i = product_compute_next_states(previous_set, range_data, options);
	    bdd_free(previous_set);
	}
	if (bdd_leq(set_i, total_set, 1, 1)) {
    	    /* shouldn't ever get to this line, since we checked 
	     * previously that excite_states were contained in total_set 
	     */
	    return -1;
	}
	use_cofactored_set(&set_i, &total_set, bdd_cofactor);
	array_insert(bdd_t *, reached_sets, i, set_i);
	reached_excite_states = bdd_and(excite_states, 
					array_fetch(bdd_t *, reached_sets, i), 1, 1);
	if (!(bdd_is_tautology(reached_excite_states, 0))) {
	    reverse_generate_Sj(info, seq_info, i, reached_excite_states);
	    bdd_free(reached_excite_states);
	    return i;
	}
	bdd_free(reached_excite_states);
    }
    /* shouldn't ever get to this line, since we checked previously that
        excite_states were contained in total_set */
    return -1;
}

static void 
reverse_generate_Sj(info, seq_info, depth, reached_excite_states)
atpg_info_t *info;
seq_info_t *seq_info;
int depth;
bdd_t *reached_excite_states;
{
    range_data_t *range_data = seq_info->range_data;
    verif_options_t *options = seq_info->seq_opt;
    array_t *reached_sets = seq_info->reached_sets;
    array_t *input_trace = seq_info->input_trace;
    bdd_t *minterm;
    bdd_t *reset_states;
    bdd_t *preimage;
    bdd_t *support_reached;
    bdd_t *state_justified;
    bdd_t *input;
    bdd_t *new_justified_states;
    int i, justified_excite_state_key;

    if (info->atpg_opt->use_internal_states) {
	reset_states = seq_info->start_states;
    } else {
    	reset_states = range_data->init_state_fn;
    }
    minterm = seq_get_one_minterm(range_data->manager,
                                     reached_excite_states, seq_info->var_table);
    state_justified = bdd_smooth(minterm, seq_info->real_pi_bdds);
    new_justified_states = bdd_or(seq_info->justified_states, state_justified, 1, 1);
    bdd_free(seq_info->justified_states);
    seq_info->justified_states = new_justified_states;
    bdd_free(minterm);
    justified_excite_state_key = convert_bdd_to_int(state_justified);
    
    for (i = depth; i -- ; ) {
	preimage = product_compute_reverse_image(state_justified, 
						 range_data, options);
	bdd_free(state_justified);
	support_reached = bdd_and(preimage, 
				  array_fetch(bdd_t *, reached_sets, i), 1, 1);
	bdd_free(preimage);
        minterm = seq_get_one_minterm(range_data->manager,
                                         support_reached, seq_info->var_table);
	bdd_free(support_reached);
    	state_justified = bdd_smooth(minterm, seq_info->real_pi_bdds);
	input = bdd_smooth(minterm, range_data->input_vars);
	array_insert(bdd_t *, input_trace, i, input);
    }
    /* state_justified should be a start state at this point */
    assert(bdd_leq(state_justified, reset_states, 1, 1));
    if (info->atpg_opt->use_internal_states) {
	seq_info->start_state_used = state_justified;
    } else {
    	bdd_free(state_justified);
    }

    convert_trace_to_sequence(info, seq_info, seq_info->just_sequence_table, 
			      seq_info->just_sequence, seq_info->input_vars, 
			      depth, justified_excite_state_key);
    for (i = depth; i -- ; ) {
	input = array_fetch(bdd_t *, input_trace, i);
	bdd_free(input);
    }
}

static void 
convert_trace_to_sequence(info, seq_info, table, sequence, input_vars, 
	seq_length, key)
atpg_info_t *info;
seq_info_t *seq_info;
st_table *table;
array_t *sequence;
array_t *input_vars;
int seq_length;
int key;
{
    array_t *input_trace = seq_info->input_trace;
    int i, j, value, npi;
    bdd_t *input, *var;
    unsigned *vector, *vector_for_reuse;
    array_t *sequence_for_reuse;
    bool save_sequence_for_reuse;
    array_t *old_just_sequence;

    npi = info->n_real_pi;
    if (seq_length > array_n(sequence)) {
	for (i = seq_length - array_n(sequence); i --; ) {
	    vector = ALLOC(unsigned, npi);
	    array_insert_last(unsigned *, sequence, vector);
	}
    }
    save_sequence_for_reuse = FALSE;
    if (table != NIL(st_table)) {
    	if (! st_lookup(table, (char *) key, NIL(char *))) {
	    save_sequence_for_reuse = TRUE;
	}
    }
    if (save_sequence_for_reuse) {
    	sequence_for_reuse = array_alloc(unsigned *, seq_length);
    }
    /* for each vector of sequence... */ 
    for (i = 0; i < seq_length; i ++ ) {
	input = array_fetch(bdd_t *, input_trace, i);
	vector = array_fetch(unsigned *, sequence, i);
	if (save_sequence_for_reuse) {
	    vector_for_reuse = ALLOC(unsigned, npi);
	}

	/* fill in the vectors minterm by minterm: 2->0 */
	for (j = 0; j < array_n(input_vars); j++) {
	    var = array_fetch(bdd_t *, input_vars, j);
	    value = (bdd_leq(input, var, 1, 1)) ? ALL_ONE : ALL_ZERO;
	    vector[j] = value;
	    if (save_sequence_for_reuse) {
	    	vector_for_reuse[j] = value;
	    }
	}
	if (save_sequence_for_reuse) {
	    array_insert(unsigned *, sequence_for_reuse, i, vector_for_reuse);
	}
    }
    if (save_sequence_for_reuse) {
    	st_insert(table, (char *) key, (char *) sequence_for_reuse);
    }
}

int
seq_reuse_prop_sequence(npi, seq_info, key)
int npi;
seq_info_t *seq_info;
int key;
{
    array_t *prop_sequence = seq_info->prop_sequence;
    array_t *old_prop_sequence;
    int i, j, seq_length;
    unsigned *vector, *old_vector;

    assert(st_lookup(seq_info->prop_sequence_table, (char *) key,
                     (char **) &old_prop_sequence));
    if (old_prop_sequence == NIL(array_t)) return 0;
    seq_length = array_n(old_prop_sequence);
    for (i = seq_length; i -- ; ) {
	vector = array_fetch(unsigned *, prop_sequence, i);
	old_vector = array_fetch(unsigned *, old_prop_sequence, i);
	for (j = npi; j -- ; ) {
	    vector[j] = old_vector[j];
	}
    }
    return seq_length;
}

int 
seq_fault_free_propagate(info, seq_info)
atpg_info_t *info;
seq_info_t *seq_info;
{
    bdd_t *init_state;
    int n_prop_vectors, key;

    init_state = convert_states_to_product_bdd(seq_info, seq_info->good_state, 
					       seq_info->faulty_state);
    n_prop_vectors = traverse_product_machine(info, seq_info, init_state, TRUE);
    if (n_prop_vectors == 0) {
	key = derive_prop_key(seq_info);
    	st_insert(seq_info->prop_sequence_table, (char *) key, NIL(char));
    }
    bdd_free(init_state);
    return n_prop_vectors;
}

int
traverse_product_machine(info, seq_info, init_state, prop)
atpg_info_t *info;
seq_info_t *seq_info;
bdd_t *init_state;
bool prop;
{
    range_data_t *range_data = seq_info->product_range_data;
    verif_options_t *options = seq_info->seq_product_opt;
    array_t *reached_sets = seq_info->product_reached_sets;
    array_t *input_trace = seq_info->input_trace;
    bdd_t *current_set, *total_set, *new_current_set, *output_fn,
	  *incorrect_support, *incorrect_input, *incorrect_state, *tmp,
	  *minterm;
    int output_index, iter_count, seq_length, i, init_state_key;

    current_set = bdd_dup(init_state);
    total_set = bdd_zero(range_data->manager);

    iter_count = 0;
    for (;;) {
    	if (! (seq_check_output(current_set, range_data, &output_index))) {
            break;
    	}
    	if (bdd_leq(current_set, total_set, 1, 1)) return 0;
    	use_cofactored_set(&current_set, &total_set, bdd_cofactor);
    	array_insert(bdd_t *, reached_sets, iter_count, bdd_dup(total_set));
    	if (bdd_is_tautology(total_set, 1)) return 0;
    	new_current_set = product_compute_next_states(current_set, range_data, 
						      options);
    	bdd_free(current_set);
    	current_set = new_current_set;
    	iter_count++;
    }
    seq_length = iter_count + 1;
    output_fn = array_fetch(bdd_t *, range_data->external_outputs, output_index);
    incorrect_support = bdd_and(current_set, output_fn, 1, 0);
    bdd_free(current_set);
    for (;;) {
        minterm = seq_get_one_minterm(range_data->manager, incorrect_support,
                                      seq_info->product_var_table);
    	bdd_free(incorrect_support);
    	incorrect_state = bdd_smooth(minterm, seq_info->product_real_pi_bdds);
	incorrect_input = bdd_smooth(minterm, range_data->input_vars);
    	array_insert(bdd_t *, input_trace, iter_count, incorrect_input);
    	if (iter_count == 0) {
	    assert(bdd_leq(incorrect_state, init_state,1 , 1));
	    if (!prop && info->atpg_opt->use_internal_states) {
		seq_info->start_state_used = incorrect_state;
	    } else {
    	    	bdd_free(incorrect_state);
	    }
      	    break;
    	}
    	iter_count--;
    	tmp = product_compute_reverse_image(incorrect_state, range_data, options);
    	bdd_free(incorrect_state);
    	total_set = array_fetch(bdd_t *, reached_sets, iter_count);
    	incorrect_support = bdd_and(tmp, total_set, 1, 1);
    	bdd_free(tmp);
    }
    if (prop) {
	init_state_key = derive_prop_key(seq_info);
    	convert_trace_to_sequence(info, seq_info, seq_info->prop_sequence_table, 
				  seq_info->prop_sequence, 
				  seq_info->product_input_vars, seq_length, 
				  init_state_key);
    } else {
    	convert_trace_to_sequence(info, seq_info, NIL(st_table), 
				  seq_info->prop_sequence, 
				  seq_info->product_input_vars, seq_length, 0);
    }
    for (i = seq_length; i -- ; ) {
	tmp = array_fetch(bdd_t *, input_trace, i);
	bdd_free(tmp);
    }
    for (i = seq_length - 1; i -- ; ) {
	tmp = array_fetch(bdd_t *, reached_sets, i);
	bdd_free(tmp);
    }
    return seq_length;
}

static int 
seq_check_output(current_set, data, output_index)
bdd_t *current_set;
range_data_t *data;
int *output_index;
{
    int i;
    bdd_t *output;

    for (i = 0; i < array_n(data->external_outputs); i++) {
    	output = array_fetch(bdd_t *, data->external_outputs, i);
    	if (! bdd_leq(current_set, output, 1, 1)) {
      	    if (output_index != NIL(int)) {
        	*output_index = i;
      	    }
      	    return 0;
    	}
    }
    return 1;
}

/* --insert the fault into the faulty network, and make necessary changes to
     the bdd formulas for the external outputs and transition outputs
   --verify the equivalence of the good and faulty machine (thus proving the 
     fault redundant), or else produce a distinguishing sequence (which is a
     test sequence)
   --put the network and range data back into its original condition
*/
int
good_faulty_PMT(fault, info, seq_info)
fault_t *fault;
atpg_info_t *info;
seq_info_t *seq_info;
{
    range_data_t *range_data = seq_info->product_range_data;
    output_info_t *output_info = seq_info->seq_product_opt->output_info;
    network_t *network = seq_info->product_network;
    array_t *orig_external_outputs = seq_info->orig_external_outputs;
    array_t *orig_transition_outputs = seq_info->orig_transition_outputs;
    saved_nodes_t saved_nodes;
    array_t *node_vec;
    int i, n_prop_vectors;
    node_t *node;
    bdd_t *output, *orig_output;

    insert_fault(network, fault, &saved_nodes);

    ntbdd_free_at_node(saved_nodes.const_node);
    node_vec = network_tfo(saved_nodes.const_node, INFINITY);
    for (i = array_n(node_vec); i --; ) {
        node = array_fetch(node_t *, node_vec, i);
	ntbdd_free_at_node(node);
    }
    array_free(node_vec);

    for (i = 0; i < array_n(output_info->xnor_nodes); i++) {
      	node = array_fetch(node_t *, output_info->xnor_nodes, i);
      	output = ntbdd_node_to_bdd(node, range_data->manager, 
				   output_info->pi_ordering);
      	array_insert(bdd_t *, range_data->external_outputs, i, output);
    }
    for (i = 0; i < array_n(output_info->transition_nodes); i++) {
	node = array_fetch(node_t *, output_info->transition_nodes, i);
      	output = ntbdd_node_to_bdd(node, range_data->manager, 
				   output_info->pi_ordering);
      	array_insert(bdd_t *, range_data->transition_outputs, i, output);
    }

    if (info->atpg_opt->use_internal_states) {
    	n_prop_vectors = traverse_product_machine(info, seq_info, 
				seq_info->product_start_states, FALSE);
    } else {
    	n_prop_vectors = traverse_product_machine(info, seq_info, 
				range_data->init_state_fn, FALSE);
    }

    ntbdd_free_at_node(saved_nodes.const_node);
    node_vec = network_tfo(saved_nodes.const_node, INFINITY);
    for (i = array_n(node_vec); i --; ) {
        node = array_fetch(node_t *, node_vec, i);
	ntbdd_free_at_node(node);
    }
    array_free(node_vec);

    remove_fault(network, &saved_nodes);

    for (i = 0; i < array_n(output_info->xnor_nodes); i++) {
      	node = array_fetch(node_t *, output_info->xnor_nodes, i);
	ntbdd_free_at_node(node);
	orig_output = bdd_dup(array_fetch(bdd_t *, orig_external_outputs, i));
	ntbdd_set_at_node(node, orig_output);
	array_insert(bdd_t *, range_data->external_outputs, i, orig_output);
    }
    for (i = 0; i < array_n(output_info->transition_nodes); i++) {
	node = array_fetch(node_t *, output_info->transition_nodes, i);
	ntbdd_free_at_node(node);
	orig_output = bdd_dup(array_fetch(bdd_t *, orig_transition_outputs, i));
	ntbdd_set_at_node(node, orig_output);
	array_insert(bdd_t *, range_data->transition_outputs, i,
		     bdd_dup(orig_output));
    }

    return n_prop_vectors;
}

static void
insert_fault(network, fault, saved_nodes)
network_t *network;
fault_t *fault;
saved_nodes_t *saved_nodes;
{
    int n_fanout, i;
    node_t *faulty_node, *faulty_fanin, *const_node, *fanout, **fanouts;
    lsGen gen;

    faulty_node = fault->node->copy;
    if ((fault->fanin) == NIL(node_t)) {
	faulty_fanin = NIL(node_t);
    } else {
        faulty_fanin = fault->fanin->copy;
    }
    saved_nodes->faulty_node = faulty_node;
    saved_nodes->faulty_fanin = faulty_fanin;

    if (node_function(fault->node) != NODE_PI) {
	const_node = (fault->value == S_A_0 ? node_constant(0) : node_constant(1));

	if ((faulty_fanin) == NIL(node_t)) {
	    saved_nodes->which_case = 1;
	    saved_nodes->orig_faulty_node = node_dup(faulty_node);
	    node_replace(faulty_node, const_node);
	    saved_nodes->const_node = faulty_node;
	} 
	else if (node_num_fanout(faulty_fanin) == 1) {
	    saved_nodes->which_case = 2;
	    saved_nodes->orig_faulty_fanin = node_dup(faulty_fanin);
	    node_replace(faulty_fanin, const_node);
	    saved_nodes->const_node = faulty_fanin;
	} 
	else {
	    saved_nodes->which_case = 3;
	    network_add_node(network, const_node);
	    saved_nodes->const_node = const_node;
	    node_patch_fanin(faulty_node, faulty_fanin, const_node);
	}
    } 
    else {
	saved_nodes->which_case = 4;
	n_fanout = 0;
	foreach_fanout(faulty_node, gen, fanout) {
	    if (strstr(fanout->name, ":0") != NULL) {
		n_fanout ++;
	    }
	}
	const_node = (fault->value == S_A_0 ? node_constant(0) : node_constant(1));
	network_add_node(network, const_node);
	fanouts = ALLOC(node_t *, n_fanout);
	i = 0;
	foreach_fanout(faulty_node, gen, fanout) {
	    if (strstr(fanout->name, ":0") != NULL) {
	    	fanouts[i ++] = fanout;
	    }
	}
	saved_nodes->const_node = const_node;
	saved_nodes->n_fanout = n_fanout;
	saved_nodes->fanouts = fanouts;
	for (i = 0; i < n_fanout; i ++) {
	    node_patch_fanin(fanouts[i], faulty_node, const_node);
	}
    }
}

static void
remove_fault(network, saved_nodes)
network_t *network;
saved_nodes_t *saved_nodes;
{
    int i;

    switch (saved_nodes->which_case) {
	case 1:
	    node_replace(saved_nodes->faulty_node, saved_nodes->orig_faulty_node);
	    break; 
	case 2:
	    node_replace(saved_nodes->faulty_fanin, saved_nodes->orig_faulty_fanin);
	    break;
	case 3:
	    node_patch_fanin(saved_nodes->faulty_node, saved_nodes->const_node,
							saved_nodes->faulty_fanin);
	    network_delete_node(network, saved_nodes->const_node);
	    break;
	case 4:
	    for (i = 0; i < saved_nodes->n_fanout; i ++) {
	    	node_patch_fanin(saved_nodes->fanouts[i], saved_nodes->const_node, 
							saved_nodes->faulty_node);
	    }
	    network_delete_node(network, saved_nodes->const_node);
	    FREE(saved_nodes->fanouts);
	    break;
	default:
	    fprintf(sisout, "ERROR--saved_nodes->which case was not in [1,4]\n");
	    break;
    }
}


