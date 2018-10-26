
#include "sis.h"
#include "atpg_int.h"

static sequence_t *justify_and_ff_propagate();
static sequence_t *internal_states_justify_and_ff_propagate();

/* This function generates a test sequence using a 3-step process of combinational
 * testing, fault-free justification, and fault-free propagation.  
 * The returned test sequence is ALLOCed only if a test is found.  
 */
sequence_t *
generate_test(fault, info, ss_info, seq_info, exdc_info, cnt)
fault_t *fault;
atpg_info_t *info;
atpg_ss_info_t *ss_info;
seq_info_t *seq_info;
atpg_ss_info_t *exdc_info;
int cnt;
{
    long time, last_time;
    sat_t *atpg_sat = ss_info->atpg_sat;
    time_info_t *time_info = info->time_info;
    atpg_options_t *atpg_opt = info->atpg_opt;
    int n_pi_vars;
    sat_result_t sat_value;
    sequence_t *test_sequence;
    unsigned *vector;

    last_time = util_cpu_time();
    sat_reset(atpg_sat);
    n_pi_vars = atpg_network_fault_clauses(ss_info, exdc_info, seq_info, fault);
    time = util_cpu_time();
    time_info->SAT_clauses += (time - last_time);
    last_time = time;

    sat_value = sat_solve(atpg_sat, atpg_opt->fast_sat, atpg_opt->verbosity);
    time = util_cpu_time();
    time_info->SAT_solve += (time - last_time);
    switch (sat_value) {
	case SAT_ABSURD:
	    fault->status = REDUNDANT;
	    fault->redund_type = CONTROL;
	    info->statistics->sat_red += 1;
	break;
	case SAT_GAVEUP:
	    fault->status = ABORTED;
	break;
	case SAT_SOLVED:
	    if (info->seq) {
		if (atpg_opt->use_internal_states) {
	    	    test_sequence = internal_states_justify_and_ff_propagate(fault, 
					info, ss_info, seq_info, n_pi_vars, cnt);
		} else {
	    	    test_sequence = justify_and_ff_propagate(fault, info, 
					ss_info, seq_info, n_pi_vars, cnt);
		}
	    } else {
		fault->is_covered = TRUE;
		test_sequence = ALLOC(sequence_t, 1);
		test_sequence->vectors = array_alloc(unsigned *, 1);
		vector = ALLOC(unsigned, info->n_real_pi);
		atpg_derive_excitation_vector(ss_info, n_pi_vars, vector);
		array_insert(unsigned *, test_sequence->vectors, 0, vector);
	    }
	    if (fault->is_covered) {
	    	fault->status = TESTED;
	    }
	break;
	default:
	    fail("bad result returned by SAT");
	break;
    }

    return test_sequence;
}

/* The returned test sequence is ALLOCed only if a test is found.  
 */ 
static sequence_t *
justify_and_ff_propagate(fault, info, ss_info, seq_info, n_pi_vars, cnt)
fault_t *fault;
atpg_info_t *info;
atpg_ss_info_t *ss_info;
seq_info_t *seq_info;
int n_pi_vars;
int cnt;
{
    statistics_t *stats = info->statistics;
    time_info_t *time_info = info->time_info;
    atpg_options_t *atpg_opt = info->atpg_opt;
    sequence_t *test_sequence;
    long time, last_time;
    bdd_t *excite_states, *already_justified;
    int i, n_just_vectors, actual_n_just_vectors, n_prop_vectors, 
	key, inverted_key;
    unsigned *vector, *excitation_vector;
    bool guaranteed_test;

    last_time = util_cpu_time();
    /* generate excitation states bdd from SAT solution */
    excite_states = seq_derive_excitation_states(ss_info, seq_info, n_pi_vars);
    if (ATPG_DEBUG) {
	if (!bdd_leq(excite_states, seq_info->range_data->total_set, 1, 1)) {
	    fail("SAT returned some unreachable tests");
	}
    }
    already_justified = bdd_and(excite_states, 
				  seq_info->justified_states, 1, 1);
    if (!(bdd_is_tautology(already_justified, 0))) {
	stats->n_just_reused += 1;
	n_just_vectors = seq_reuse_just_sequence(info->n_real_pi, info->n_latch, 
					seq_info, already_justified);
    	excitation_vector = array_fetch(unsigned *, seq_info->just_sequence, 
				    n_just_vectors);
    	atpg_derive_excitation_vector(ss_info, n_pi_vars, excitation_vector);
    	actual_n_just_vectors = get_min_just_sequence(ss_info, fault, seq_info);
	if (actual_n_just_vectors < 1) {
	    n_just_vectors = seq_state_justify(info, seq_info, excite_states);
    	    excitation_vector = array_fetch(unsigned *, seq_info->just_sequence, 
				    n_just_vectors);
    	    atpg_derive_excitation_vector(ss_info, n_pi_vars, excitation_vector);
    	    actual_n_just_vectors = get_min_just_sequence(ss_info, fault, seq_info);
	}
    } else {
	n_just_vectors = seq_state_justify(info, seq_info, excite_states);
    	excitation_vector = array_fetch(unsigned *, seq_info->just_sequence, 
				    n_just_vectors);
    	atpg_derive_excitation_vector(ss_info, n_pi_vars, excitation_vector);
    	actual_n_just_vectors = get_min_just_sequence(ss_info, fault, seq_info);
    }
    bdd_free(excite_states);
    bdd_free(already_justified);
    assert(actual_n_just_vectors > 0);
    time = util_cpu_time();
    time_info->justify += (time - last_time);
    if (fault->is_covered) {
	test_sequence = derive_test_sequence(ss_info, seq_info, 
				actual_n_just_vectors, 0, info->n_real_pi, cnt);
	return test_sequence;
    } else {				/* find propagation sequence */
	guaranteed_test = FALSE;
	n_prop_vectors = 0;
	if (atpg_opt->random_prop) {
	    stats->n_random_propagations += 1;
	    last_time = util_cpu_time();
	    n_prop_vectors = random_propagate(fault, info, ss_info, seq_info);
	    time = util_cpu_time();
	    time_info->random_propagate += (time - last_time);
	    if (n_prop_vectors) {
		guaranteed_test = TRUE;
		fault->is_covered = TRUE;
		stats->n_random_propagated += 1;
	    }
	}
	if (!n_prop_vectors && atpg_opt->deterministic_prop 
		&& atpg_opt->build_product_machines && seq_info->product_machine_built) {
	    last_time = util_cpu_time();
	    stats->n_det_propagations += 1;
	    key = derive_prop_key(seq_info);
	    if (st_lookup(seq_info->prop_sequence_table, (char *) key, NIL(char *))) {
		stats->n_prop_reused += 1;
		n_prop_vectors = seq_reuse_prop_sequence(info->n_real_pi, 
						     seq_info, key);
	    } else {
		inverted_key = derive_inverted_prop_key(seq_info);
		if (st_lookup(seq_info->prop_sequence_table, 
				 (char *) inverted_key, NIL(char *))) {
		    stats->n_prop_reused += 1;
		    n_prop_vectors = seq_reuse_prop_sequence(info->n_real_pi, 
					    seq_info, inverted_key);
		} else {
		    n_prop_vectors = seq_fault_free_propagate(info, seq_info);
		}
	    }
	    time = util_cpu_time();
	    time_info->ff_propagate += (time - last_time);
	}
	if (n_prop_vectors) {
	    test_sequence = derive_test_sequence(ss_info, seq_info, 
				    actual_n_just_vectors, n_prop_vectors, 
				    info->n_real_pi, cnt);
	    if (!guaranteed_test) {
		last_time = util_cpu_time();
		if (atpg_verify_test(ss_info, fault, test_sequence)) {
		    fault->is_covered = TRUE;
		    stats->n_ff_propagated += 1;
		} else {
		    stats->n_not_ff_propagated += 1;
		    for (i = array_n(test_sequence->vectors); i -- ; ) {
			vector = array_fetch(unsigned *, test_sequence->vectors, i);
			FREE(vector);
		    }
		    array_free(test_sequence->vectors);
		    FREE(test_sequence);
		}
		time = util_cpu_time();
		time_info->ff_propagate += (time - last_time);
	    }
	}
    }
    return test_sequence;
}

/* The returned test sequence is ALLOCed only if a test is found.  
 */ 
static sequence_t *
internal_states_justify_and_ff_propagate(fault, info, ss_info, seq_info, n_pi_vars, cnt)
fault_t *fault;
atpg_info_t *info;
atpg_ss_info_t *ss_info;
seq_info_t *seq_info;
int n_pi_vars;
int cnt;
{
    statistics_t *stats = info->statistics;
    time_info_t *time_info = info->time_info;
    atpg_options_t *atpg_opt = info->atpg_opt;
    sequence_t *test_sequence;
    long time, last_time;
    bdd_t *excite_states;
    int i, n_just_vectors, actual_n_just_vectors, n_prop_vectors, key, 
	inverted_key, bdd_key, n_old_vectors;
    unsigned *vector, *excitation_vector;
    bool guaranteed_test;
    bdd_t *new_start_states;
    lsList sequence_list;
    sequence_t *sequence_start;
    array_t *old_vectors;
    unsigned *old_vector;

    last_time = util_cpu_time();
    /* generate excitation states bdd from SAT solution */
    excite_states = seq_derive_excitation_states(ss_info, seq_info, n_pi_vars);
    if (ATPG_DEBUG) {
	if (!bdd_leq(excite_states, seq_info->range_data->total_set, 1, 1)) {
	    fail("SAT returned some unreachable tests");
	}
    }
    n_just_vectors = internal_states_seq_state_justify(info, seq_info, excite_states);
    bdd_free(excite_states);
    excitation_vector = array_fetch(unsigned *, seq_info->just_sequence, 
				    n_just_vectors);
    atpg_derive_excitation_vector(ss_info, n_pi_vars, excitation_vector);
    if (bdd_equal(seq_info->start_state_used, seq_info->range_data->init_state_fn)) {
	n_old_vectors = 0;
    } else {
	bdd_key = convert_bdd_to_int(seq_info->start_state_used);
	assert(st_lookup(seq_info->state_sequence_table, (char *) bdd_key, (char **) &sequence_list));
	assert(lsLength(sequence_list) > 0);
	(void) lsFirstItem(sequence_list, (lsGeneric *) &sequence_start, 0);
	old_vectors = sequence_start->vectors;
	n_old_vectors = array_n(old_vectors);
	create_just_sequence(seq_info, n_just_vectors + 1, old_vectors, info->n_real_pi);
    }
    actual_n_just_vectors = get_min_just_sequence(ss_info, fault, seq_info);
    if (actual_n_just_vectors < n_old_vectors) {
	fault->is_covered = FALSE;
	simulate_entire_sequence(ss_info, fault, seq_info, old_vectors);
	actual_n_just_vectors = n_old_vectors;
    }
    assert(actual_n_just_vectors > 0);
    time = util_cpu_time();
    time_info->justify += (time - last_time);
    if (fault->is_covered) {
	test_sequence = derive_test_sequence(ss_info, seq_info, 
				actual_n_just_vectors, 0, info->n_real_pi, cnt);

    } else {				/* find propagation sequence */
	guaranteed_test = FALSE;
	n_prop_vectors = 0;
	if (atpg_opt->random_prop) {
	    stats->n_random_propagations += 1;
	    last_time = util_cpu_time();
	    n_prop_vectors = random_propagate(fault, info, ss_info, seq_info);
	    time = util_cpu_time();
	    time_info->random_propagate += (time - last_time);
	    if (n_prop_vectors) {
		guaranteed_test = TRUE;
		fault->is_covered = TRUE;
		stats->n_random_propagated += 1;
	    }
	}
	if (!n_prop_vectors && atpg_opt->deterministic_prop 
		&& atpg_opt->build_product_machines && seq_info->product_machine_built) {
	    last_time = util_cpu_time();
	    stats->n_det_propagations += 1;
	    key = derive_prop_key(seq_info);
	    if (st_lookup(seq_info->prop_sequence_table, (char *) key, NIL(char *))) {
		stats->n_prop_reused += 1;
		n_prop_vectors = seq_reuse_prop_sequence(info->n_real_pi, 
						     seq_info, key);
	    } else {
		inverted_key = derive_inverted_prop_key(seq_info);
		if (st_lookup(seq_info->prop_sequence_table, 
				 (char *) inverted_key, NIL(char *))) {
		    stats->n_prop_reused += 1;
		    n_prop_vectors = seq_reuse_prop_sequence(info->n_real_pi, 
					    seq_info, inverted_key);
		} else {
		    n_prop_vectors = seq_fault_free_propagate(info, seq_info);
		}
	    }
	    time = util_cpu_time();
	    time_info->ff_propagate += (time - last_time);
	}
	if (n_prop_vectors) {
	    test_sequence = derive_test_sequence(ss_info, seq_info, 
				    actual_n_just_vectors, n_prop_vectors, 
				    info->n_real_pi, cnt);
	    if (!guaranteed_test) {
		last_time = util_cpu_time();
		if (atpg_verify_test(ss_info, fault, test_sequence)) {
		    fault->is_covered = TRUE;
		    stats->n_ff_propagated += 1;
		} else {
		    stats->n_not_ff_propagated += 1;
		    for (i = array_n(test_sequence->vectors); i -- ; ) {
			vector = array_fetch(unsigned *, test_sequence->vectors, i);
			FREE(vector);
		    }
		    array_free(test_sequence->vectors);
		    FREE(test_sequence);
		}
		time = util_cpu_time();
		time_info->ff_propagate += (time - last_time);
	    }
	}
    }
    if (n_old_vectors && fault->is_covered) {
	assert(st_delete_int(seq_info->state_sequence_table, &bdd_key, (char **) &sequence_list));
	(void) lsDelBegin(sequence_list, (lsGeneric *) &sequence_start);
	assert(st_delete(info->sequence_table, (char **) &sequence_start, NIL(char *)));
	if (lsLength(sequence_list) == 0) {
	    new_start_states = bdd_and(seq_info->start_states, 
				       seq_info->start_state_used, 1, 0);
	    bdd_free(seq_info->start_states);
	    seq_info->start_states = new_start_states;
	    lsDestroy(sequence_list, 0);
	} else {
	    st_insert(seq_info->state_sequence_table, 
		      (char *) bdd_key, (char *) sequence_list);
	}
	for (i = n_old_vectors; i -- ; ) {
	    old_vector = array_fetch(unsigned *, old_vectors, i);
	    FREE(old_vector);
	}
	array_free(old_vectors);
	FREE(sequence_start);
    }
    bdd_free(seq_info->start_state_used);
    return test_sequence;
}

/* The returned test sequence is ALLOCed only if a test is found.  
 */ 
sequence_t *
generate_test_using_verification(fault, info, ss_info, seq_info, cnt)
fault_t *fault;
atpg_info_t *info;
atpg_ss_info_t *ss_info;
seq_info_t *seq_info;
int cnt;
{
    sequence_t *test_sequence;
    int n_test_vectors, n_old_vectors, actual_n_just_vectors, i, j, key;
    bdd_t *new_start_states;
    lsList sequence_list;
    sequence_t *sequence_start;
    array_t *old_vectors;
    unsigned *vector, *old_vector;
    array_t *just_sequence = seq_info->just_sequence;
    int npi = info->n_real_pi;
    bdd_t *start_state_used;
    bool internal_state_used, tested;

    assert(seq_info->product_machine_built);
    info->statistics->n_verifications += 1;
    n_test_vectors = good_faulty_PMT(fault, info, seq_info);
    start_state_used = seq_info->start_state_used;
    if (n_test_vectors) {
	tested = TRUE;
    	internal_state_used = FALSE;
        n_old_vectors = 0;
	if (info->atpg_opt->use_internal_states) {
	    if (!bdd_equal(start_state_used, 
			   seq_info->product_range_data->init_state_fn)) {
		internal_state_used = TRUE;
		key = convert_product_bdd_to_key(info, seq_info, start_state_used);
		assert(st_lookup(seq_info->state_sequence_table, (char *) key, (char **) &sequence_list));
		assert(lsLength(sequence_list) > 0);
		(void) lsFirstItem(sequence_list, (lsGeneric *) &sequence_start, 0);
		old_vectors = sequence_start->vectors;
		n_old_vectors = array_n(old_vectors);
    		if (n_old_vectors > array_n(just_sequence)) {
		    for (i = n_old_vectors - array_n(just_sequence); i --; ) {
	    		vector = ALLOC(unsigned, npi);
	    		array_insert_last(unsigned *, just_sequence, vector);
		    }
    		}
		for (i = n_old_vectors; i -- ; ) {
	    	    vector = array_fetch(unsigned *, just_sequence, i);
	    	    old_vector = array_fetch(unsigned *, old_vectors, i);
	    	    for (j = npi; j -- ; ) {
			vector[j] = old_vector[j];
	    	    }
		}
	    }
	}
	test_sequence = derive_test_sequence(ss_info, seq_info, n_old_vectors, 
					     n_test_vectors, info->n_real_pi, cnt);
	if (internal_state_used) {
	    tested = atpg_verify_test(ss_info, fault, test_sequence);
	    if (!tested && ATPG_DEBUG) {
		create_just_sequence(seq_info, 0, test_sequence->vectors, 
				     info->n_real_pi);
    		actual_n_just_vectors = get_min_just_sequence(ss_info, fault, seq_info);
		assert(actual_n_just_vectors <= n_old_vectors);
	    }
	}
	if (tested) {
	    fault->is_covered = TRUE;
	    fault->status = TESTED;
	    if (internal_state_used) {
		assert(st_delete_int(seq_info->state_sequence_table, &key, (char **) &sequence_list));
		(void) lsDelBegin(sequence_list, (lsGeneric *) &sequence_start);
		assert(st_delete(info->sequence_table, (char **) &sequence_start, NIL(char *)));
		if (lsLength(sequence_list) == 0) {
		    new_start_states = bdd_and(seq_info->product_start_states, 
					       start_state_used, 1, 0);
		    bdd_free(seq_info->product_start_states);
		    seq_info->product_start_states = new_start_states;
		    lsDestroy(sequence_list, 0);
		} else {
		    st_insert(seq_info->state_sequence_table, 
			      (char *) key, (char *) sequence_list);
		}
		for (i = n_old_vectors; i -- ; ) {
		    old_vector = array_fetch(unsigned *, old_vectors, i);
		    FREE(old_vector);
		}
		array_free(old_vectors);
		FREE(sequence_start);
	    }
	} else {
	    for (i = array_n(test_sequence->vectors); i -- ; ) {
		vector = array_fetch(unsigned *, test_sequence->vectors, i);
		FREE(vector);
	    }
	    array_free(test_sequence->vectors);
	    FREE(test_sequence);
	}
	if (info->atpg_opt->use_internal_states) {
	    bdd_free(start_state_used);
	}
    } else {
    	fault->status = REDUNDANT;
	fault->redund_type = OBSERVE;
	info->statistics->verified_red += 1;
	test_sequence = NIL(sequence_t);
    }
    return test_sequence;
}
