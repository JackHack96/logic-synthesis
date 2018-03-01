/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/atpg/atpg_util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:17 $
 *
 */
#include "sis.h"
#include "atpg_int.h"

static int order_cmp();
static void print_time_info();

/* the following are only here to let me link to
the cmu bdd package! */
/*bool bdd_is_cube(){}
double tmg_compute_optimal_clock(){}
*/

static int
order_cmp(obj1, obj2)
char *obj1;
char *obj2;
{
    node_index_t *n1, *n2;

    n1 = (node_index_t *) obj1;
    n2 = (node_index_t *) obj2;
    return (ATPG_CLAUSE_GET(n1->node))->order - 
	   (ATPG_CLAUSE_GET(n2->node))->order;
}

int *
fanin_dfs_sort(node, fanin_ptr)
node_t *node;
node_index_t *fanin_ptr;
{
    node_t *fanin;
    int *fanin_list, j, n;

    n = node_num_fanin(node);
    foreach_fanin(node, j, fanin) {
	fanin_ptr[j].node = fanin;
	fanin_ptr[j].index = j;
    }
    qsort((void *) fanin_ptr, n, sizeof(node_index_t), order_cmp);
    fanin_list = ALLOC(int, n);
    for (j = 0; j < n; j ++) {
	fanin_list[j] = fanin_ptr[j].index;
    }
    return fanin_list;
}

node_t ** 
sat_fanout_dfs_sort(node)
node_t *node;
{
    node_t *fanout, **fanout_ptr;
    lsGen gen;
    int j;

    fanout_ptr = ALLOC(node_t *, node_num_fanout(node));
    j = 0;
    foreach_fanout(node, gen, fanout) {
	fanout_ptr[j ++] = fanout;
    }
    qsort((void *) fanout_ptr, node_num_fanout(node), sizeof(node_t *), 
	  order_cmp);
    return fanout_ptr;
}

void
concat_lists(l1, l2)
lsList l1;
lsList l2;
{
    lsGeneric data;

    while (lsDelBegin(l2, &data) != LS_NOMORE) {
	lsNewEnd(l1, (lsGeneric) data, 0);
    }
    lsDestroy(l2, 0);
}

void
create_just_sequence(seq_info, n_new_vectors, old_vectors, npi)
seq_info_t *seq_info;
int n_new_vectors;
array_t *old_vectors;
int npi;
{
    array_t *just_sequence = seq_info->just_sequence;
    array_t *tmp_sequence = seq_info->prop_sequence;
    int i, j, n_old_vectors;
    unsigned *just_vector, *tmp_vector, *old_vector;

    n_old_vectors = array_n(old_vectors);
    if ((n_old_vectors + n_new_vectors) > array_n(just_sequence)) {
	for (i = n_old_vectors + n_new_vectors - array_n(just_sequence); i --; ) {
	    just_vector = ALLOC(unsigned, npi);
	    array_insert_last(unsigned *, just_sequence, just_vector);
	}
    }
    for (i = n_new_vectors; i --; ) {
	just_vector = array_fetch(unsigned *, just_sequence, i);
	tmp_vector = array_fetch(unsigned *, tmp_sequence, i);
	for (j = npi; j -- ; ) {
	    tmp_vector[j] = just_vector[j];
	}
    }
    for (i = n_old_vectors; i --; ) {
	just_vector = array_fetch(unsigned *, just_sequence, i);
	old_vector = array_fetch(unsigned *, old_vectors, i);
	for (j = npi; j -- ; ) {
	    just_vector[j] = old_vector[j];
	}
    }
    for (i = n_new_vectors; i --; ) {
	just_vector = array_fetch(unsigned *, just_sequence, i + n_old_vectors);
	tmp_vector = array_fetch(unsigned *, tmp_sequence, i);
	for (j = npi; j -- ; ) {
	    just_vector[j] = tmp_vector[j];
	}
    }
}

/* assign test values to each real PI of network */
void
atpg_derive_excitation_vector(info, n_pi_vars, vector)
atpg_ss_info_t *info;
int n_pi_vars;
unsigned *vector;
{
    int i, v, ind;
    sat_input_t sv;

    /* give unassigned values the value 1 */
    for (i = 0; i < info->n_real_pi; i ++) {
	vector[i] = ALL_ONE;
    }
    for (i = 0; i < n_pi_vars; i ++) {
	sv = array_fetch(sat_input_t, info->sat_input_vars, i);
	v = sat_get_value(info->atpg_sat, sv.sat_id);
	assert(st_lookup_int(info->pi_po_table, sv.info, &ind));
	if ((v == 0) && (ind >= info->n_latch)) {
	    vector[ind - info->n_latch] = ALL_ZERO;
	}
    }
}

void
atpg_print_fault(fp, fault)
FILE *fp;
fault_t *fault;
{
    if (fault->value == S_A_0) {
	fprintf(fp, "S_A_0: NODE: %s ", node_name(fault->node));
    }
    else {
	fprintf(fp, "S_A_1: NODE: %s ", node_name(fault->node));
    }
    if (fault->fanin == NIL(node_t)) {
	fprintf(fp, "\tOUTPUT\n");
    }
    else {
	fprintf(fp, "\tINPUT: %s\n", node_name(fault->fanin));
    }
}

void
atpg_print_vectors(fp, vectors, n_inputs)
FILE *fp;
array_t *vectors;
int n_inputs;
{
    int i, j, seq_length;
    unsigned *vector;

    seq_length = array_n(vectors);
    for (i = 0; i < seq_length; i ++) {
	vector = array_fetch(unsigned *, vectors, i);
	for (j = 0; j < n_inputs; j ++) {
            fprintf(fp, "%c ", (vector[j] == ALL_ZERO) ? '0':'1');
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
}

void
atpg_print_some_vectors(fp, vectors, n_inputs, seq_length)
FILE *fp;
array_t *vectors;
int n_inputs;
int seq_length;
{
    int i, j;
    unsigned *vector;

    for (i = 0; i < seq_length; i ++) {
	vector = array_fetch(unsigned *, vectors, i);
	for (j = 0; j < n_inputs; j ++) {
            fprintf(fp, "%c ", (vector[j] == ALL_ZERO) ? '0':'1');
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
}

void 
atpg_print_bdd(bdd_fn, message)
bdd_t *bdd_fn;
char *message;
{
    fprintf(sisout, "%s: \n", message);
    bdd_print(bdd_fn);
}



void 
atpg_print_results(info, seq_info)
atpg_info_t *info;
seq_info_t *seq_info;
{
    statistics_t *stats = info->statistics;
    time_info_t *time_info = info->time_info;

    fprintf(sisout, "faults: %d\ttested: %d\tredundant: %d\tuntested: %d\n",
	      stats->initial_faults, lsLength(info->tested_faults), 
	      lsLength(info->redundant_faults), 
	      lsLength(info->untested_faults) + lsLength(info->final_untested_faults));
    if (info->atpg_opt->verbosity > 1) {
    	fprintf(sisout, "tested by RTG: %d\n", stats->n_RTG_tested);
    	fprintf(sisout, 
	      	"comb. red. or all excite states unreachable: %d\tverified red.: %d\n",
	      	stats->sat_red, stats->verified_red);
    	fprintf(sisout, "reused justification sequences: %d\n", stats->n_just_reused);
    	fprintf(sisout, "random propagations: %d\tsuccessful: %d\n", 
		stats->n_random_propagations, stats->n_random_propagated);
    	fprintf(sisout, "deterministic propagations: %d\n", stats->n_det_propagations);
    	fprintf(sisout, "reused propagation sequences: %d\n", stats->n_prop_reused);
    	fprintf(sisout, 
		"ff prop. sequence was test in faulty machine: %d\twasn't test: %d\n",
		 stats->n_ff_propagated, stats->n_not_ff_propagated);
    	fprintf(sisout, "untested by main loop: %d\tverifications performed: %d\n",
				stats->n_untested_by_main_loop, stats->n_verifications);
	if (info->seq) {
    	    fprintf(sisout, "DFS depth: %d\tstates reached in DFS: %1.0f\n", 
		    array_n(seq_info->reached_sets),  
		    bdd_count_onset(seq_info->range_data->total_set, 
					seq_info->range_data->pi_inputs));
    	    fprintf(sisout, "justified states: %1.0f\n", 
		    bdd_count_onset(seq_info->justified_states, 
					seq_info->range_data->pi_inputs));
	}
    	print_time_info(time_info);
    	fprintf(sisout, "number of tests in test set: %d\n", stats->n_sequences);
	if (info->seq) {
    	    fprintf(sisout, "number of vectors in test set: %d\n", stats->n_vectors);
	}
    }
}

static void
print_time_info(time_info)
time_info_t *time_info;
{
    long total_time = time_info->total_time;

    fprintf(sisout, "setup time:\t\t\t%.2f\t%.2f%%\n", 
					(time_info->setup)/1000.0, 
					100.0 * time_info->setup/total_time);
    fprintf(sisout, "STG traversal time:\t\t%.2f\t%.2f%%\n", 
					(time_info->traverse_stg)/1000.0,
					100.0 * time_info->traverse_stg/total_time);
    fprintf(sisout, "RTG time:\t\t\t%.2f\t%.2f%%\n", 
					(time_info->RTG)/1000.0,
					100.0 * time_info->RTG/total_time);
    fprintf(sisout, "SAT clause setup time:\t\t%.2f\t%.2f%%\n", 
					(time_info->SAT_clauses)/1000.0,
					100.0 * time_info->SAT_clauses/total_time);
    fprintf(sisout, "SAT solve time:\t\t\t%.2f\t%.2f%%\n", 
					(time_info->SAT_solve)/1000.0,
					100.0 * time_info->SAT_solve/total_time);
    fprintf(sisout, "justification time:\t\t%.2f\t%.2f%%\n", 
					(time_info->justify)/1000.0,
					100.0 * time_info->justify/total_time);
    fprintf(sisout, "random propagation time:\t%.2f\t%.2f%%\n", 
					(time_info->random_propagate)/1000.0,
					100.0 * time_info->random_propagate/total_time);
    fprintf(sisout, "fault-free propagation time:\t%.2f\t%.2f%%\n", 
					(time_info->ff_propagate)/1000.0,
					100.0 * time_info->ff_propagate/total_time);
    fprintf(sisout, "fault simulation time:\t\t%.2f\t%.2f%%\n",
					(time_info->fault_simulate)/1000.0,
					100.0 * time_info->fault_simulate/total_time);
    fprintf(sisout, "product machine verification:\t%.2f\t%.2f%%\n",
				(time_info->product_machine_verify)/1000.0,
				100.0 * time_info->product_machine_verify/total_time);
    fprintf(sisout, "reverse fault simulation:\t%.2f\t%.2f%%\n",
				(time_info->reverse_fault_sim)/1000.0,
				100.0 * time_info->reverse_fault_sim/total_time);
    fprintf(sisout, "total time:\t\t\t%.2f\n", total_time/1000.0);
}

sequence_t *
derive_test_sequence(ss_info, seq_info, n_just_vectors, n_prop_vectors, npi, cnt)
atpg_ss_info_t *ss_info;
seq_info_t *seq_info;
int n_just_vectors, n_prop_vectors;
int npi;
int cnt;
{
    sequence_t *sequence;
    array_t *just_sequence = seq_info->just_sequence;
    array_t *prop_sequence = seq_info->prop_sequence;
    array_t *word_vectors = ss_info->word_vectors;
    array_t *vectors;
    unsigned *vector, *just_vector, *prop_vector, *word_vector;
    int i, j;

    if (array_n(word_vectors) < (n_just_vectors + n_prop_vectors)) {
	lengthen_word_vectors(ss_info, 
		n_just_vectors + n_prop_vectors - array_n(word_vectors),
		ss_info->n_real_pi);
    }

    sequence = ALLOC(sequence_t, 1);
    sequence->vectors = array_alloc(unsigned *, n_just_vectors + n_prop_vectors);
    vectors = sequence->vectors;
    for (i = n_just_vectors; i -- ; ) {
	just_vector = array_fetch(unsigned *, just_sequence, i);
	word_vector = array_fetch(unsigned *, word_vectors, i);
	vector = ALLOC(unsigned, npi);
	for (j = npi; j -- ; ) {
	    vector[j] = just_vector[j];
	    if (just_vector[j] == ALL_ZERO) {
            	word_vector[j] &= ~ (1 << cnt);
	    } else {
		assert(just_vector[j] == ALL_ONE);
            	word_vector[j] |= (1 << cnt);
	    }
	}
	array_insert(unsigned *, vectors, i, vector);
    }
    for (i = n_prop_vectors; i --; ) {
	prop_vector = array_fetch(unsigned *, prop_sequence, i);
	word_vector = array_fetch(unsigned *, word_vectors, i + n_just_vectors);
	vector = ALLOC(unsigned, npi);
	for (j = npi; j -- ; ) {
	    vector[j] = prop_vector[j];
	    if (prop_vector[j] == ALL_ZERO) {
            	word_vector[j] &= ~ (1 << cnt);
	    } else {
		assert(prop_vector[j] == ALL_ONE);
            	word_vector[j] |= (1 << cnt);
	    }
	}
	array_insert(unsigned *, vectors, i + n_just_vectors, vector);
    }

    return sequence;
}

void 
reset_word_vectors(info)
atpg_ss_info_t *info;
{
    array_t *word_vectors = info->word_vectors;
    unsigned *vector;
    int i, j, npi;

    npi = info->n_real_pi;
    for (i = array_n(word_vectors); i --; ) {
	vector = array_fetch(unsigned *, word_vectors, i);
	for (j = npi; j --; ) {
	    vector[j] = ALL_ONE;
	}
    }
}

void
lengthen_word_vectors(info, n_vectors, npi)
atpg_ss_info_t *info;
int n_vectors;
int npi;
{
    array_t *word_vectors = info->word_vectors;
    int i, j;
    unsigned *new_vector;

    for (i = n_vectors; i --; ) {
	new_vector = ALLOC(unsigned, npi);
	for (j = npi; j --; ) {
	    new_vector[j] = ALL_ONE;
	}
	array_insert_last(unsigned *, word_vectors, new_vector);
    }
}

void
fillin_word_vectors(info, seq_length, npi)
atpg_ss_info_t *info;
int seq_length;
int npi;
{
    int h, i, j;
    array_t *word_vectors = info->word_vectors;
    array_t *vectors;
    unsigned *vector, *word_vector;

    if (array_n(word_vectors) < seq_length) {
	lengthen_word_vectors(info, seq_length - array_n(word_vectors), npi);
    }

    for (h = WORD_LENGTH; h --; ) {
        if (info->faults_ptr[h] != NIL(fault_t)) {
	    vectors = info->faults_ptr[h]->sequence->vectors;
	    for (i = array_n(vectors); i -- ; ) {
		vector = array_fetch(unsigned *, vectors, i);
		word_vector = array_fetch(unsigned *, word_vectors, i);
		for (j = npi; j -- ; ) {
		    if (vector[j] == ALL_ZERO) {
			word_vector[j] &= ~ (1 << h);
		    } else {
			assert(vector[j] == ALL_ONE);
			word_vector[j] |= (1 << h);
		    }
		}
	    }
	}
    }
}
