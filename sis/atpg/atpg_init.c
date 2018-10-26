
#include "sis.h"
#include "atpg_int.h"

static void atpg_extract_sim_node();
static void atpg_extract_sim_function();
static int *sim_fanout_dfs_sort();
static int id_cmp();
static void atpg_eval_sim_po();
static void atpg_eval_sim_pi();
static void atpg_eval_sim_node();
static void free_sim_node();

static time_info_t INIT_TIME_INFO = {0,0,0,0,0,0,0,0,0,0,0,0};
static statistics_t INIT_STATS = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

int
set_atpg_options(info, argc, argv)
atpg_info_t *info;
int argc;
char **argv;
{
    atpg_options_t *atpg_opt;
    int c;
    char *fname, *real_filename;

    atpg_opt = info->atpg_opt;

    /* default values */
    atpg_opt->quick_redund = FALSE;
    atpg_opt->reverse_fault_sim = TRUE;
    atpg_opt->PMT_only = FALSE;
    atpg_opt->use_internal_states = FALSE;
    atpg_opt->n_sim_sequences = WORD_LENGTH;
    atpg_opt->deterministic_prop = TRUE;
    atpg_opt->random_prop = TRUE;
    atpg_opt->rtg_depth = -1;
    atpg_opt->fault_simulate = TRUE;
    atpg_opt->fast_sat = FALSE;
    atpg_opt->rtg = TRUE;
    atpg_opt->build_product_machines = TRUE;
    atpg_opt->tech_decomp = FALSE;
    atpg_opt->timeout = 0;
    atpg_opt->verbosity = 0;
    atpg_opt->prop_rtg_depth = 20;
    atpg_opt->n_random_prop_iter = 1;
    atpg_opt->print_sequences = FALSE;
    atpg_opt->force_comb = FALSE;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "cd:DfFhn:qrRptT:v:y:z:")) != EOF) {
	switch (c) {
	    case 'c':
		atpg_opt->force_comb = TRUE;
		break;
	    case 'd':
		atpg_opt->rtg_depth = atoi(util_optarg);
		break;
	    case 'D':
		atpg_opt->deterministic_prop = FALSE;
		break;
	    case 'f':
		atpg_opt->fault_simulate = FALSE;
		break;
	    case 'F':
		atpg_opt->reverse_fault_sim = FALSE;
		break;
	    case 'h':
		atpg_opt->fast_sat = TRUE;
		break;
	    case 'n':
		atpg_opt->n_sim_sequences = atoi(util_optarg);
		if (atpg_opt->n_sim_sequences > WORD_LENGTH) {
		    return 0;
		}
		break;
	    case 'q':
		atpg_opt->quick_redund = TRUE;
		atpg_opt->build_product_machines = FALSE;
		break;
	    case 'r':
		atpg_opt->rtg = FALSE;
		break;
	    case 'R':
		atpg_opt->random_prop = FALSE;
		break;
	    case 'p':
		atpg_opt->build_product_machines = FALSE;
		break;
	    case 't':
		atpg_opt->tech_decomp = TRUE;
		break;
	    case 'T':
		atpg_opt->timeout = atoi(util_optarg);
		if (atpg_opt->timeout < 0 || atpg_opt->timeout > 3600 * 24 * 365) {
		    return 0;
		}
		break;
	    case 'v':
		atpg_opt->verbosity = atoi(util_optarg);
		break;
	    case 'y':
		atpg_opt->prop_rtg_depth = atoi(util_optarg);
		break;
	    case 'z':
		atpg_opt->n_random_prop_iter = atoi(util_optarg);
		break;
	    default:
		return 0;
	}
    }
    if (argc - util_optind == 1) {
	atpg_opt->print_sequences = TRUE;
	fname = argv[util_optind];
	atpg_opt->fp = com_open_file(fname, "w", &real_filename, /* silent */ 0);
	atpg_opt->real_filename = real_filename;
	if (atpg_opt->fp == NULL) {
	    FREE(real_filename);
	    return 0;
	}
    }
    else if (argc - util_optind != 0) {
	return 0;
    }
    return 1;	/* everything's OK */
}

atpg_info_t *
atpg_info_init(network)
network_t *network;
{
    atpg_info_t *info;
    statistics_t *stats;
    time_info_t *time_info;
    lsGen gen;
    node_t *pi, *po, *node;
    array_t *nodevec;
    int i;
    
    info = ALLOC(atpg_info_t, 1);

    info->network = network;
    info->n_pi = network_num_pi(network);
    info->n_po = network_num_po(network);
    info->n_latch = network_num_latch(network);
    info->n_real_pi = 0;
    info->n_real_po = 0;
    foreach_primary_input(network, gen, pi) {
	if (network_is_real_pi(network, pi)) {
	    info->n_real_pi ++;
	}
    }
    info->control_node_table = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_primary_output(network, gen, po) {
	if (network_is_real_po(network, po)) {
	    info->n_real_po ++;
	} else {
	    if (network_is_control(info->network, po)) {
		nodevec = network_tfi(po, INFINITY);
		for (i = array_n(nodevec); i --; ) {
		    node = array_fetch(node_t *, nodevec, i);
		    st_insert(info->control_node_table, (char *) node, (char *) 0);
		}
		array_free(nodevec);
	    }
	}
    }
    if (st_count(info->control_node_table) != 0) {
	fprintf(sisout, "NOTE: faults on clock lines and other latch control lines will not be tested.\n");
    }

    info->atpg_opt = ALLOC(atpg_options_t, 1);

    stats = ALLOC(statistics_t, 1);
    *stats = INIT_STATS;
    info->statistics = stats;

    time_info = ALLOC(time_info_t, 1);
    *time_info = INIT_TIME_INFO;
    info->time_info = time_info;

    info->sequence_table = st_init_table(st_ptrcmp, st_ptrhash);
    info->redund_table = st_init_table(st_fpcmp, st_fphash);

    info->redundant_faults = lsCreate();
    info->untested_faults = lsCreate();
    info->final_untested_faults = lsCreate();
    /* info->tested_faults is created during RTG */

    return info;
}

atpg_ss_info_t *
atpg_sim_sat_info_init(network, atpg_info)
network_t *network;
atpg_info_t *atpg_info;
{
    atpg_ss_info_t *ss_info;
    int i;
    unsigned *vector;
    lsGen gen;
    node_t *pi, *po;

    ss_info = ALLOC(atpg_ss_info_t, 1);

    ss_info->network = network;

    ss_info->n_real_pi = 0;
    ss_info->n_real_po = 0;
    foreach_primary_input(network, gen, pi) {
	if (network_is_real_pi(network, pi)) {
	    ss_info->n_real_pi ++;
	}
    }
    foreach_primary_output(network, gen, po) {
	if (network_is_real_po(network, po)) {
	    ss_info->n_real_po ++;
	}
    }

    ss_info->used = ALLOC(int, WORD_LENGTH);
    ss_info->alloc_sequences = ALLOC(sequence_t *, WORD_LENGTH);
    ss_info->word_vectors = array_alloc(unsigned *, 0);
    ss_info->prop_word_vectors = array_alloc(unsigned *, 0);
    for (i = atpg_info->atpg_opt->prop_rtg_depth; i --; ) {
	vector = ALLOC(unsigned, ss_info->n_real_pi);
	array_insert_last(unsigned *, ss_info->prop_word_vectors, vector);
    }
    ss_info->real_po_values = ALLOC(unsigned, ss_info->n_real_po);
    ss_info->true_value = ALLOC(unsigned, ss_info->n_real_po);
    ss_info->faults_ptr = ALLOC(fault_t *, WORD_LENGTH);

    return ss_info;
}

seq_info_t *
atpg_seq_info_init(info)
atpg_info_t *info;
{
    seq_info_t *seq_info;

    seq_info = ALLOC(seq_info_t, 1);
    seq_info->seq_opt = ALLOC(verif_options_t, 1);
    seq_info->seq_opt->output_info = ALLOC(output_info_t, 1);
    seq_info->seq_product_opt = ALLOC(verif_options_t, 1);
    seq_info->seq_product_opt->output_info = ALLOC(output_info_t, 1);
    seq_info->start_states = NIL(bdd_t);
    seq_info->product_start_states = NIL(bdd_t);
    seq_info->reached_sets = array_alloc(bdd_t *, 0);
    if (info->atpg_opt->build_product_machines) {
    	seq_info->product_reached_sets = array_alloc(bdd_t *, 0);
    }
    seq_info->input_trace = array_alloc(bdd_t *, 0);
    seq_info->product_machine_built = FALSE;

    return seq_info;
}

void
atpg_setup_seq_info(info, seq_info, stg_depth)
atpg_info_t *info;
seq_info_t *seq_info;
int stg_depth;
{
    range_data_t *data = seq_info->range_data;
    output_info_t *output_info = seq_info->seq_opt->output_info;
    st_table *pi_to_var_table;
    node_t *pi;
    bdd_t *var;
    int i;
    unsigned *vector;

    seq_info->just_sequence = array_alloc(unsigned *, stg_depth + 1);
    seq_info->prop_sequence = array_alloc(unsigned *, stg_depth + 1);
    for (i = stg_depth + 1; i -- ; ) {
	vector = ALLOC(unsigned, info->n_real_pi);
	array_insert(unsigned *, seq_info->just_sequence, i, vector);
	vector = ALLOC(unsigned, info->n_real_pi);
	array_insert(unsigned *, seq_info->prop_sequence, i, vector);
    }

    seq_info->real_pi_bdds = bdd_extract_var_array(data->manager, 
				  output_info->extern_pi, output_info->pi_ordering);
    seq_info->var_table = st_init_table(st_numcmp, st_numhash);
    bdd_add_varids_to_table(data->input_vars, seq_info->var_table);
    bdd_add_varids_to_table(seq_info->real_pi_bdds, seq_info->var_table);
    pi_to_var_table = get_pi_to_var_table(data->manager, output_info);
    seq_info->input_vars = array_alloc(bdd_t *, 0);
    for(i = 0; i < array_n(output_info->extern_pi); i++) {
    	pi = array_fetch(node_t *, output_info->extern_pi, i);
    	assert(st_lookup(pi_to_var_table, (char *) pi, (char **) &var));
    	array_insert_last(bdd_t *, seq_info->input_vars, var);
    }
    st_free_table(pi_to_var_table);
}

void
atpg_product_setup_seq_info(seq_info)
seq_info_t *seq_info;
{
    range_data_t *product_data = seq_info->product_range_data;
    output_info_t *product_output_info = seq_info->seq_product_opt->output_info;
    st_table *pi_to_var_table;
    node_t *pi;
    bdd_t *var;
    int i;

    seq_info->product_real_pi_bdds = 
		    bdd_extract_var_array(product_data->manager, 
					  product_output_info->extern_pi, 
					  product_output_info->pi_ordering);
    seq_info->product_var_table = st_init_table(st_numcmp, st_numhash);
    bdd_add_varids_to_table(product_data->input_vars, 
			    seq_info->product_var_table);
    bdd_add_varids_to_table(seq_info->product_real_pi_bdds, 
			    seq_info->product_var_table);
    pi_to_var_table = get_pi_to_var_table(product_data->manager, 
					  product_output_info);
    seq_info->product_input_vars = array_alloc(bdd_t *, 0);
    for(i = 0; i < array_n(product_output_info->extern_pi); i++) {
	pi = array_fetch(node_t *, product_output_info->extern_pi, i);
	assert(st_lookup(pi_to_var_table, (char *) pi, (char **) &var));
	array_insert_last(bdd_t *, seq_info->product_input_vars, var);
    }
    st_free_table(pi_to_var_table);
    seq_info->product_machine_built = TRUE;
}

void
atpg_sat_init(info)
atpg_ss_info_t *info;
{
    if (info->network == NIL(network_t)) {
	return;
    }
    atpg_sat_node_info_setup(info->network);
    atpg_sat_clause_begin();
    info->atpg_sat = sat_new();
    info->sat_input_vars = array_alloc(sat_input_t, info->n_pi);
}

void 
atpg_sat_node_info_setup(network)
network_t *network;
{
    array_t *nodevec;
    int i, max_fanin;
    node_t *node;
    node_index_t *fanin_ptr;
    atpg_clause_t *node_info;

    nodevec = network_dfs(network);
    max_fanin = 0;
    for (i = 0; i < array_n(nodevec); i ++) {
	node = array_fetch(node_t *, nodevec, i);
	node_info = ALLOC(atpg_clause_t, 1);
	ATPG_CLAUSE_SET(node, node_info);
	node_info->true_id = -1;
	node_info->fault_id = -1;
	node_info->active_id = -1;
	node_info->current_id = -1;
	node_info->visited = FALSE;
	node_info->order = i;
	max_fanin = MAX(max_fanin, node_num_fanin(node));
    }
    fanin_ptr = ALLOC(node_index_t, max_fanin);
    for (i = 0; i < array_n(nodevec); i ++) {
	node = array_fetch(node_t *, nodevec, i);
	node_info = ATPG_CLAUSE_GET(node);
	node_info->nfanin = node_num_fanin(node);
	node_info->nfanout = node_num_fanout(node);
	node_info->fanin = fanin_dfs_sort(node, fanin_ptr);
	node_info->fanout = sat_fanout_dfs_sort(node);
    }
    FREE(fanin_ptr);
    array_free(nodevec);
}

void
atpg_sim_setup(ss_info)
atpg_ss_info_t *ss_info;
{
    node_t *pi, *po;
    int id;
    lsGen gen, latch_gen;
    latch_t *l;
    unsigned value;

    ss_info->n_latch = network_num_latch(ss_info->network);
    ss_info->n_pi = network_num_pi(ss_info->network);
    ss_info->n_po = network_num_po(ss_info->network);
    ss_info->pi_po_table = st_init_table(st_ptrcmp, st_ptrhash);
    id = 0;
    /* give latches the first n_latch id's, and match latch pi's and po's by id */
    foreach_latch(ss_info->network, gen, l) {
	pi = latch_get_output(l);
	st_insert(ss_info->pi_po_table, (char *) pi, (char *) id);
	po = latch_get_input(l);
	st_insert(ss_info->pi_po_table, (char *) po, (char *) id++);
    }
    assert(id == ss_info->n_latch);
    foreach_primary_input(ss_info->network, gen, pi) {
	if (network_is_real_pi(ss_info->network, pi)) {
	    st_insert(ss_info->pi_po_table, (char *) pi, (char *) id++);
	} else {
	    if (!network_is_control(ss_info->network, pi)) {
	    	assert(st_is_member(ss_info->pi_po_table, (char *) pi));
	    }
	}
    }
    assert(id == (ss_info->n_latch + ss_info->n_real_pi));
    id = ss_info->n_latch;
    foreach_primary_output(ss_info->network, gen, po) {
	if (network_is_real_po(ss_info->network, po)) {
	    st_insert(ss_info->pi_po_table, (char *) po, (char *) id++);
	} else {
	    if (!network_is_control(ss_info->network, po)) {
	    	assert(st_is_member(ss_info->pi_po_table, (char *) po));
	    }
	}
    }
    assert(id == (ss_info->n_latch + ss_info->n_real_po));
    ss_info->pi_uid = ALLOC(int, ss_info->n_real_pi + ss_info->n_latch);
    ss_info->po_uid = ALLOC(int, ss_info->n_real_po + ss_info->n_latch);

    /* create the reset state array used in fault simulation */
    ss_info->reset_state = ALLOC(unsigned, ss_info->n_latch);
    foreach_latch(ss_info->network, latch_gen, l) {
	value = ((latch_get_initial_value(l) == 0) ? ALL_ZERO : ALL_ONE);
	pi = latch_get_output(l);
	st_lookup_int(ss_info->pi_po_table, (char *) pi, &id);
	ss_info->reset_state[id] = value;
    }
    ss_info->true_state = ALLOC(unsigned, ss_info->n_latch);
    ss_info->faulty_state = ALLOC(unsigned, ss_info->n_latch);
    ss_info->changed_node_indices = ALLOC(int, ss_info->n_latch + 2);
    ss_info->all_true_value = ALLOC(unsigned, ss_info->n_po);
    ss_info->all_po_values = ALLOC(unsigned, ss_info->n_po);
}

void
atpg_comb_sim_setup(ss_info)
atpg_ss_info_t *ss_info;
{
    array_t *nodevec;
    node_t *node, **fanout_ptr;
    int id, max_fanout;
    lsGen gen;
    node_t *pi, *po;

    ss_info->n_nodes = ss_info->n_pi + ss_info->n_po + 
                    network_num_internal(ss_info->network);
    ss_info->tfo = ALLOC(int, ss_info->n_nodes);

    /* create simulation nodes */
    max_fanout = 0;
    nodevec = network_dfs(ss_info->network);
    ss_info->sim_nodes = ALLOC(atpg_sim_node_t, ss_info->n_nodes);

    /* allocate all id's before extraction */
    for (id = 0; id < array_n(nodevec); id ++) {
	node = array_fetch(node_t *, nodevec, id);
	SET_ATPG_ID(node, id);
	max_fanout = MAX(max_fanout, node_num_fanout(node));
    }
    fanout_ptr = ALLOC(node_t *, max_fanout);
    for (id = 0; id < array_n(nodevec); id ++) {
	node = array_fetch(node_t *, nodevec, id);
	atpg_extract_sim_node(node, ss_info->sim_nodes, fanout_ptr);
    }
    FREE(fanout_ptr);
    array_free(nodevec);

    /* match pi's with an index for consistent reference */
    /* Because the latches were given the first n_latch id's they will appear 
       first in the uid arrays, and a latch's pi and po will have the same index
    */
    foreach_primary_input(ss_info->network, gen, pi) {
	if (!network_is_control(ss_info->network, pi)) { 
	    assert(st_lookup_int(ss_info->pi_po_table, (char *) pi, &id));
	    ss_info->pi_uid[id] = GET_ATPG_ID(pi);
	}
    }
    foreach_primary_output(ss_info->network, gen, po) {
	if (!network_is_control(ss_info->network, po)) { 
	    assert(st_lookup_int(ss_info->pi_po_table, (char *) po, &id));
	    ss_info->po_uid[id] = GET_ATPG_ID(po);
	}
    }
}

void
atpg_add_pi_and_po(dc_network, info)
network_t *dc_network;
atpg_info_t *info;
{
    node_t *orig_pi, *orig_po, *exdc_pi, *exdc_po, *new_pi, *new_po;
    int id;
    lsGen gen;
    bool network_changed;

    network_changed = FALSE;
    foreach_primary_input(info->network, gen, orig_pi) {
	exdc_pi = network_find_node(dc_network, orig_pi->name);
	if (exdc_pi == NIL(node_t)) {
	    new_pi = node_alloc();
	    new_pi->name = ALLOC(char, strlen (node_long_name(orig_pi)) + 1);
	    strcpy(new_pi->name, node_long_name(orig_pi));
	    network_add_primary_input(dc_network, new_pi);
	    network_changed = TRUE;
	}
    }
    foreach_primary_output(info->network, gen, orig_po) {
	exdc_po = network_find_node(dc_network, orig_po->name);
	if (exdc_po == NIL(node_t)) {
	    new_po = node_constant(0);
	    new_po->name = ALLOC(char, strlen (node_long_name(orig_po)) + 1);
	    strcpy(new_po->name, node_long_name(orig_po));
	    network_add_node(dc_network, new_po);
	    network_add_primary_output(dc_network, new_po);
	    network_changed = TRUE;
	}
    }
    if (network_changed) {
	fprintf(sisout, 
"\nNOTE: When using an external don't care network, the primary inputs and\noutputs of the external don't care network must match exactly with the\nprimary inputs and outputs of the network.  Your external don't care network\nwas missing some inputs or outputs, so dummy primary inputs or outputs\nhave been added to the exdc network.  This will not change the behavior\nof the circuit.\n\n");
    }
}


void
atpg_exdc_sim_link(exdc_info, info)
atpg_ss_info_t *exdc_info;
atpg_ss_info_t *info;
{
    node_t *pi, *po, *orig_pi, *orig_po;
    int id;
    lsGen gen;

    /* must have same pi's and po's as original network */
    /* Dummy pi's and po's should have been added (if they were needed) 
	in the procedure atpg_add_pi_and_po */
    assert(exdc_info->n_real_pi == info->n_pi);
    assert(exdc_info->n_real_po == info->n_po);

    /* Match pi's with index in info->network for consistent reference
     * same order of pi's and po's as original network. */
    foreach_primary_input(exdc_info->network, gen, pi) {
	orig_pi = network_find_node(info->network, pi->name);
	assert(orig_pi != NIL(node_t));
	assert(st_lookup_int(info->pi_po_table, (char *) orig_pi, &id));
	exdc_info->pi_uid[id] = GET_ATPG_ID(pi);
    }
    foreach_primary_output(exdc_info->network, gen, po) {
	orig_po = network_find_node(info->network, po->name);
	assert(orig_po != NIL(node_t));
	assert(st_lookup_int(info->pi_po_table, (char *) orig_po, &id));
	exdc_info->po_uid[id] = GET_ATPG_ID(po);
    }
}

void
print_and_destroy_sequences(info)
atpg_info_t *info;
{
    st_table *sequence_table = info->sequence_table;
    atpg_options_t *atpg_opt = info->atpg_opt;
    statistics_t *stats = info->statistics;
    st_generator *sgen;
    sequence_t *sequence;
    array_t *vectors;
    unsigned *vector;
    int i;
    lsGen gen;
    node_t *pi;

    if (atpg_opt->print_sequences) {
        fprintf(atpg_opt->fp,
                    "atpg test sequences for %s\n", network_name(info->network));
	if (st_count(info->control_node_table) != 0) {
	     fprintf(atpg_opt->fp, "\nNOTE: Ignore setting of clock and other latch control inputs.\n\n");
	}
        fprintf(atpg_opt->fp, "inputs:\n");
        foreach_primary_input(info->network, gen, pi) {
            if (network_is_real_pi(info->network, pi)) {
                fprintf(atpg_opt->fp, "%s ", node_name(pi));
            }
        }
        fprintf(atpg_opt->fp, "\n\n");
    }
    st_foreach_item(sequence_table, sgen, (char **) &sequence, NIL(char *)) {
	vectors = sequence->vectors;
	if (atpg_opt->print_sequences) {
	    atpg_print_vectors(atpg_opt->fp, vectors, info->n_real_pi);
	}
	stats->n_vectors += array_n(vectors);
	stats->n_sequences += 1;
	for (i = array_n(vectors); i -- ; ) {
	    vector = array_fetch(unsigned *, vectors, i);
	    FREE(vector);
	}
	array_free(vectors);
	FREE(sequence);
    }
    if (atpg_opt->print_sequences) {
	FREE(atpg_opt->real_filename);
	if (atpg_opt->fp != sisout) (void) fclose(atpg_opt->fp);
    }
}

void
atpg_sat_free(info)
atpg_ss_info_t *info;
{
    node_t *node;
    lsGen gen;
    atpg_clause_t *node_info;

    sat_delete(info->atpg_sat);
    foreach_node(info->network, gen, node) {
	node_info = ATPG_CLAUSE_GET(node);
	if (node_info) {
	    FREE(node_info->fanin);
	    FREE(node_info->fanout);
	    FREE(node_info);
	    ATPG_CLAUSE_SET(node, 0);
	}
    }
    atpg_sat_clause_end();
    array_free(info->sat_input_vars);
}

void atpg_sim_unsetup(info)
atpg_ss_info_t *info;
{
    st_free_table(info->pi_po_table);
    FREE(info->pi_uid);
    FREE(info->po_uid);
    FREE(info->reset_state);
    FREE(info->true_state);
    FREE(info->faulty_state);
    FREE(info->all_true_value);
    FREE(info->all_po_values);
    FREE(info->changed_node_indices);
}

void atpg_comb_sim_unsetup(info)
atpg_ss_info_t *info;
{
    int i;

    for (i = info->n_nodes; i --; ) {
	free_sim_node(&((info->sim_nodes)[i]));
    }
    FREE(info->sim_nodes);
    FREE(info->tfo);
}

void
atpg_sim_free(info)
atpg_ss_info_t *info;
{
    int i;
    unsigned *vector;
    array_t *word_vectors = info->word_vectors;
    array_t *prop_word_vectors = info->prop_word_vectors;

    FREE(info->real_po_values);
    FREE(info->true_value);
    FREE(info->used);
    FREE(info->alloc_sequences);
    for (i = array_n(word_vectors); i -- ; ) {
	vector = array_fetch(unsigned *, word_vectors, i);
	FREE(vector);
    }
    array_free(word_vectors);
    for (i = array_n(prop_word_vectors); i -- ; ) {
	vector = array_fetch(unsigned *, prop_word_vectors, i);
	FREE(vector);
    }
    array_free(prop_word_vectors);
    FREE(info->faults_ptr);
    FREE(info->tfo);
}

void
seq_info_product_free(seq_info)
seq_info_t *seq_info;
{
    seq_info->product_machine_built = FALSE;
    product_free_range_data(seq_info->product_range_data, 
			    seq_info->seq_product_opt);
    network_free(seq_info->product_network);
    output_info_free(seq_info->seq_product_opt->output_info);	     
    free_bdds_in_array(seq_info->orig_external_outputs);
    array_free(seq_info->orig_external_outputs);
    free_bdds_in_array(seq_info->orig_transition_outputs);
    array_free(seq_info->orig_transition_outputs);
    array_free(seq_info->product_real_pi_bdds);
    array_free(seq_info->product_input_vars);
    st_free_table(seq_info->product_var_table);
}

void
seq_info_free(info, seq_info)
atpg_info_t *info;
seq_info_t *seq_info;
{
    int i;
    unsigned *vector;
    st_table *just_table;
    st_table *prop_table;
    atpg_options_t *atpg_opt = info->atpg_opt;
    st_generator *sgen;
    array_t *vectors;
    char *key;
    lsGen gen;
    node_t *node;
    atpg_clause_t *node_info;
    lsList seq_list;

    product_free_range_data(seq_info->range_data, seq_info->seq_opt);
    network_free(seq_info->network_copy);
    output_info_free(seq_info->seq_opt->output_info);
    free_bdds_in_array(seq_info->reached_sets);
    bdd_free(seq_info->justified_states);
    array_free(seq_info->good_state);
    array_free(seq_info->faulty_state);

    just_table = seq_info->just_sequence_table;
    st_foreach_item(just_table, sgen, &key, (char **) &vectors) {
	for (i = array_n(vectors); i -- ; ) {
	    vector = array_fetch(unsigned *, vectors, i);
	    FREE(vector);
	}
	array_free(vectors);
    }
    st_free_table(just_table);
    prop_table = seq_info->prop_sequence_table;
    st_foreach_item(prop_table, sgen, &key, (char **) &vectors) {
	if (vectors != NIL(array_t)) {
	    for (i = array_n(vectors); i -- ; ) {
	    	vector = array_fetch(unsigned *, vectors, i);
	    	FREE(vector);
	    }
	    array_free(vectors);
	}
    }
    st_free_table(prop_table);
    array_free(seq_info->latch_to_pi_ordering);
    if (atpg_opt->use_internal_states) {
	bdd_free(seq_info->start_states);
    	if (atpg_opt->build_product_machines) {
	    if (seq_info->product_start_states != NIL(bdd_t))
	    		bdd_free(seq_info->product_start_states);
	    array_free(seq_info->latch_to_product_pi_ordering);
	}
   	st_foreach_item(seq_info->state_sequence_table, sgen, 
					&key, (char **) &seq_list) {
	    lsDestroy(seq_list, 0);
    	}
    	st_free_table(seq_info->state_sequence_table);
    }

    foreach_node(seq_info->valid_states_network, gen, node) {
	node_info = ATPG_CLAUSE_GET(node);
	if (node_info) {
	    FREE(node_info->fanin);
	    FREE(node_info->fanout);
	    FREE(node_info);
	    ATPG_CLAUSE_SET(node, 0);
	}
    }
    network_free(seq_info->valid_states_network);
    array_free(seq_info->real_pi_bdds);
    array_free(seq_info->input_vars);
    st_free_table(seq_info->var_table);

    if (info->atpg_opt->build_product_machines) {
    	array_free(seq_info->product_reached_sets);
    }
    FREE(seq_info->seq_opt->output_info);
    FREE(seq_info->seq_product_opt->output_info);
    FREE(seq_info->seq_opt);
    FREE(seq_info->seq_product_opt);
    array_free(seq_info->reached_sets);

    for (i = array_n(seq_info->just_sequence); i -- ; ) {
	vector = array_fetch(unsigned *, seq_info->just_sequence, i);
	FREE(vector);
    }
    array_free(seq_info->just_sequence);
    for (i = array_n(seq_info->prop_sequence); i -- ; ) {
	vector = array_fetch(unsigned *, seq_info->prop_sequence, i);
	FREE(vector);
    }
    array_free(seq_info->prop_sequence);
    array_free(seq_info->input_trace);
    FREE(seq_info);
}

void
atpg_free_info(info)
atpg_info_t *info;
{
    st_generator *sgen;
    fault_pattern_t *info_p;

    FREE(info->atpg_opt);
    FREE(info->time_info);
    FREE(info->statistics);
    lsDestroy(info->redundant_faults, free_fault);
    lsDestroy(info->untested_faults, free_fault);
    lsDestroy(info->final_untested_faults, free_fault);
    st_free_table(info->sequence_table);
    st_foreach_item(info->redund_table, sgen, (char **) &info_p, NIL(char *)) {
        FREE(info_p);
    }
    st_free_table(info->redund_table);
    st_free_table(info->control_node_table);
    FREE(info);
}

static void
atpg_extract_sim_node(node, sim_nodes, fanout_ptr)
node_t *node;
atpg_sim_node_t *sim_nodes;
node_t **fanout_ptr;
{
    int i;
    node_t *fanin;
    atpg_sim_node_t *sm;

    sm = &(sim_nodes[GET_ATPG_ID(node)]);
    sm->node = node;
    sm->nfanout = node_num_fanout(node);
    sm->fanout = sim_fanout_dfs_sort(node, fanout_ptr);
    sm->uid = GET_ATPG_ID(node);
    sm->type = node_function(node);
    sm->visited = FALSE;
    sm->n_inputs = node_num_fanin(node);
    sm->or_output_mask = ALL_ZERO;
    sm->and_output_mask = ALL_ONE;
    sm->or_input_masks = ALLOC(unsigned, sm->n_inputs);
    sm->and_input_masks = ALLOC(unsigned, sm->n_inputs);
    for (i = sm->n_inputs; i --; ) {
	sm->or_input_masks[i] = ALL_ZERO;
	sm->and_input_masks[i] = ALL_ONE;
    }
    switch (sm->type) {
	case NODE_0:
	    sm->value = ALL_ZERO;
	    sm->eval = atpg_eval_sim_pi;
	    return;
	case NODE_1:
	    sm->value = ALL_ONE;
	    sm->eval = atpg_eval_sim_pi;
	    return;
	case NODE_PI:
	    sm->eval = atpg_eval_sim_pi;
	    return;
	case NODE_PO:
	    sm->fanin_values = ALLOC(unsigned *, sm->n_inputs);
	    for (i = sm->n_inputs; i --; ) {
		fanin = node_get_fanin(node, i);
		sm->fanin_values[i] = 
		    &(sim_nodes[GET_ATPG_ID(fanin)].value);
	    }
	    sm->eval = atpg_eval_sim_po;
	    return;
	case NODE_BUF:
	case NODE_INV:
	case NODE_AND:
	case NODE_OR:
	case NODE_COMPLEX:
	    sm->fanin_values = ALLOC(unsigned *, sm->n_inputs);
	    for (i = sm->n_inputs; i --; ) {
		fanin = node_get_fanin(node, i);
		sm->fanin_values[i] = 
		    &(sim_nodes[GET_ATPG_ID(fanin)].value);
	    }
	    sm->n_cubes = node_num_cube(node);
	    sm->function = ALLOC(int *, sm->n_cubes);
	    for (i = sm->n_cubes; i --; ) {
		 sm->function[i] = ALLOC(int, sm->n_inputs);
	    }
	    sm->inputs = ALLOC(unsigned *, 3);
	    for (i = 3; i --; ) {
		sm->inputs[i] = ALLOC(unsigned, sm->n_inputs);
	    }
	    for (i = sm->n_inputs; i --; ) {
		 sm->inputs[2][i] = ALL_ONE;
	    }
	    atpg_extract_sim_function(sm);
	    sm->eval = atpg_eval_sim_node;
	    return;
	case NODE_UNDEFINED:
	default:
	    fail("unexpected node type");
    }
}

static void 
atpg_extract_sim_function(sim_node)
atpg_sim_node_t *sim_node;
{
    int c, l, literal;
    node_t *node;
    node_cube_t cube;

    node = sim_node->node;
    for (c = sim_node->n_cubes; c --; ) {
	cube = node_get_cube(node, c);
	for (l = node_num_fanin(node); l --; ) {
	    literal = node_get_literal(cube, l);
	    switch (literal) {
		case ZERO:
		    sim_node->function[c][l] = 0;
		    break;
		case ONE:
		    sim_node->function[c][l] = 1;
		    break;
		case TWO:
		    sim_node->function[c][l] = 2;
		    break;
		default:
		    fail("bad literal");
	    }
	}
    }
}

static int *
sim_fanout_dfs_sort(node, fanout_ptr)
node_t *node;
node_t **fanout_ptr;
{
    node_t *fanout;
    lsGen gen;
    int j, n, *fanout_list;

    n = node_num_fanout(node);
    j = 0;
    foreach_fanout(node, gen, fanout) {
	fanout_ptr[j ++] = fanout;
    }
    qsort((void *) fanout_ptr, n, sizeof(node_t *), id_cmp);
    fanout_list = ALLOC(int, n);
    for (j = 0; j < n ; j ++) {
	fanout_list[j] = GET_ATPG_ID(fanout_ptr[j]);
    }
    return fanout_list;
}

static int
id_cmp(obj1, obj2)
char *obj1;
char *obj2;
{
    node_t **n1, **n2;

    n1 = (node_t **) obj1;
    n2 = (node_t **) obj2;
    return GET_ATPG_ID((*n1)) - GET_ATPG_ID((*n2));
}

static void 
atpg_eval_sim_po(sim_nodes, uid)
atpg_sim_node_t *sim_nodes;
int uid;
{
    int result;
    atpg_sim_node_t *sim_node = &sim_nodes[uid];

    result = *(sim_node->fanin_values[0]);
    result &= (sim_node->and_input_masks[0] & sim_node->and_output_mask);
    result |= (sim_node->or_input_masks[0] | sim_node->or_output_mask);
    sim_node->value = result;
}

static void 
atpg_eval_sim_pi(sim_nodes, uid)
atpg_sim_node_t *sim_nodes;
int uid;
{
    int result;
    atpg_sim_node_t *sim_node = &sim_nodes[uid];

    result = sim_node->value;
    result &= sim_node->and_output_mask;
    result |= sim_node->or_output_mask;
    sim_node->value = result;
}

static void 
atpg_eval_sim_node(sim_nodes, uid)
atpg_sim_node_t *sim_nodes;
int uid;
{
    int i, j;
    int result, and_result;
    atpg_sim_node_t *sim_node = &sim_nodes[uid];

    for (j = sim_node->n_inputs; j --; ) {
        result = *(sim_node->fanin_values[j]);
        result &= sim_node->and_input_masks[j];
        result |= sim_node->or_input_masks[j];
        sim_node->inputs[1][j] = result;
        sim_node->inputs[0][j] = ~result;
    }
    result = sim_node->or_output_mask;
    for (i = sim_node->n_cubes; i --; ) {
        and_result = sim_node->and_output_mask;
        for (j = sim_node->n_inputs; j --; ) {
            and_result &= sim_node->inputs[sim_node->function[i][j]][j];
        }
        result |= and_result;
    }
    sim_node->value = result;
}

static void 
free_sim_node(sim_node)
atpg_sim_node_t *sim_node;
{
    int i;

    FREE(sim_node->and_input_masks);
    FREE(sim_node->or_input_masks);
    FREE(sim_node->fanout);

    switch (sim_node->type) {
      case NODE_PI:
      case NODE_0:
      case NODE_1:
	break;
      case NODE_PO:
	FREE(sim_node->fanin_values);
	break;
      case NODE_BUF:
      case NODE_INV:
      case NODE_AND:
      case NODE_OR:
      case NODE_COMPLEX:
	FREE(sim_node->fanin_values);
	for (i = 3; i --; ) {
	    FREE(sim_node->inputs[i]);
	}
	FREE(sim_node->inputs);
	for (i = sim_node->n_cubes; i --; ) {
	    FREE(sim_node->function[i]);
	}
	FREE(sim_node->function);
	break;
    }
}
