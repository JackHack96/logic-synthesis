
#include "sis.h"
#include "atpg_int.h"

static void atpg_network_simulate();

static void get_tfo();

static void get_tfo_of_array();

static void atpg_sim_tfo_array();

static void atpg_sim_get_po();

static bool sf_record_covered_faults();

static void ss_record_covered_faults();

static void atpg_fault_sequence_sim();

static void fp_record_covered_faults();

static int seq_length_cmp();

static void record_different_states();

static int seq_index_cmp();

/* Takes input vector and present state, and gives back real po values
 * and next state.  The next state is written over the present state!
 */
static void
atpg_network_simulate(info, real_pi_values, state, real_po_values)
        atpg_ss_info_t *info;
        unsigned *real_pi_values;
        unsigned *state;
        unsigned *real_po_values;
{
    int             i, n_latch;
    atpg_sim_node_t *sim_nodes;

    n_latch   = info->n_latch;
    sim_nodes = info->sim_nodes;
    for (i    = info->n_latch; i--;) {
        sim_nodes[info->pi_uid[i]].value = state[i];
    }
    for (i = info->n_real_pi; i--;) {
        sim_nodes[info->pi_uid[i + n_latch]].value = real_pi_values[i];
    }
    for (i = 0; i < info->n_nodes; i++) {
        (*(sim_nodes[i].eval))(sim_nodes, i);
    }
    for (i = n_latch; i--;) {
        state[i] = sim_nodes[info->po_uid[i]].value;
    }
    for (i = info->n_real_po; i--;) {
        real_po_values[i] = sim_nodes[info->po_uid[i + n_latch]].value;
    }
}

lsList
atpg_seq_single_fault_simulate(ss_info, exdc_info, sequences, fault_list,
                               original_sequences, cnt)
        atpg_ss_info_t *ss_info;
        atpg_ss_info_t *exdc_info;
        array_t *sequences;
        lsList fault_list;
        sequence_t **original_sequences;
        int cnt;
{
    int       i, j, prev_atpg_id, pi_index, n_changed_nodes, seq_length;
    unsigned  *vector;
    unsigned  *true_value, *real_po_values, *exdc_value, *true_state;
    fault_t   *fault, *prev_fault;
    lsList    covered_faults;
    lsGen     gen1;
    lsGeneric data;
    lsHandle  handle1;

    true_value     = ss_info->true_value;
    true_state     = ss_info->true_state;
    real_po_values = ss_info->real_po_values;
    exdc_value     = NIL(
    unsigned);
    seq_length = array_n(sequences);

    for (i = ss_info->n_latch; i--;) {
        true_state[i] = ss_info->reset_state[i];
    }
    covered_faults = lsCreate();
    /* for each vector of the sequence... */
    for (i         = 0; i < seq_length; i++) {
        vector = array_fetch(
        unsigned *, sequences, i);

        /* first simulate to get true value and true state */
        atpg_network_simulate(ss_info, vector, true_state, true_value);

        /* simulate don't cares */
        /* Note: the value of the state here doesn't matter, since this only
         * executes when the circuit is combinational */
        if (exdc_info != NIL(atpg_ss_info_t)) {
            exdc_value = exdc_info->true_value;
            atpg_network_simulate(exdc_info, vector, NIL(
            unsigned), exdc_value);
        }

        gen1       = lsStart(fault_list);
        prev_fault = NIL(fault_t);
        while (lsNext(gen1, (lsGeneric * ) & fault, &handle1) == LS_OK) {
            n_changed_nodes = 0;
            atpg_sf_set_sim_masks(ss_info, fault);
            if (prev_fault != NIL(fault_t)) {
                prev_atpg_id = GET_ATPG_ID(prev_fault->node);

                /* if the previous fault was on a pi, we need to reset the value */
                if (node_function(prev_fault->node) == NODE_PI) {
                    st_lookup_int(ss_info->pi_po_table, (char *) prev_fault->node, &pi_index);
                    if (network_is_real_pi(ss_info->network, prev_fault->node)) {
                        ss_info->sim_nodes[prev_atpg_id].value
                                = vector[pi_index - ss_info->n_latch];
                    } else {
                        if (i) {
                            ss_info->sim_nodes[prev_atpg_id].value
                                    = fault->current_state[pi_index];
                        } else {
                            ss_info->sim_nodes[prev_atpg_id].value
                                    = ss_info->reset_state[pi_index];
                        }
                    }
                }
                ss_info->changed_node_indices[n_changed_nodes] = prev_atpg_id;
                n_changed_nodes += 1;
            }
            /* if latch values are different for this fault than for the previous... */
            if (i) {
                for (j = ss_info->n_latch; j--;) {
                    if (ss_info->sim_nodes[ss_info->pi_uid[j]].value !=
                        fault->current_state[j]) {
                        ss_info->sim_nodes[ss_info->pi_uid[j]].value =
                                fault->current_state[j];
                        ss_info->changed_node_indices[n_changed_nodes] =
                                ss_info->pi_uid[j];
                        n_changed_nodes += 1;
                    }
                }
            }
            ss_info->changed_node_indices[n_changed_nodes] = GET_ATPG_ID(fault->node);
            n_changed_nodes += 1;
            atpg_sim_tfo_array(ss_info, n_changed_nodes);
            atpg_sim_get_po(ss_info, real_po_values, fault->current_state);
            atpg_sf_reset_sim_masks(ss_info, fault);
            prev_fault = fault;
            if (sf_record_covered_faults(fault, true_value, real_po_values,
                                         exdc_value, ss_info->n_real_po, i,
                                         original_sequences, cnt)) {
                lsRemoveItem(handle1, &data);
                lsNewEnd(covered_faults, (lsGeneric) data, 0);
            }
        }
        lsFinish(gen1);
    }
    return covered_faults;
}

/* use parallel sequence, single fault simulation */
lsList
atpg_random_cover(info, ss_info, exdc_ss_info, save_sequences, n_tests_ptr)
        atpg_info_t *info;
        atpg_ss_info_t *ss_info;
        atpg_ss_info_t *exdc_ss_info;
        bool save_sequences;
        int *n_tests_ptr;
{
    atpg_options_t *atpg_opt = info->atpg_opt;
    int            i, j, niter, npi;
    lsList         local_covered, covered_faults;
    array_t        *sequences;
    unsigned       *vector;

    npi       = info->n_real_pi;
    sequences = array_alloc(
    unsigned *, atpg_opt->rtg_depth);
    for (i = atpg_opt->rtg_depth; i--;) {
        vector = ALLOC(
        unsigned, npi);
        array_insert(
        unsigned *, sequences, i, vector);
    }

    covered_faults = lsCreate();
    niter          = 0;
    do {
        for (i = atpg_opt->rtg_depth; i--;) {
            vector = array_fetch(
            unsigned *, sequences, i);
            for (j = npi; j--;) {
                vector[j] = (random() << 16) | (random() & 0xffff);
            }
        }
        local_covered = atpg_seq_single_fault_simulate(ss_info, exdc_ss_info,
                                                       sequences, info->faults, NIL(sequence_t * ), 0);
        concat_lists(local_covered, atpg_seq_single_fault_simulate(ss_info,
                                                                   exdc_ss_info, sequences, info->untested_faults,
                                                                   NIL(sequence_t * ), 0));
        if (save_sequences && (lsLength(local_covered) > 0)) {
            extract_test_sequences(info, ss_info, local_covered, sequences,
                                   n_tests_ptr, info->n_real_pi);
            if (atpg_opt->verbosity > 0) {
                fprintf(sisout, "RTG: covered %d remaining %d\n",
                        lsLength(local_covered), lsLength(info->faults));
            }
        }
        if (lsLength(local_covered) < 10) {
            niter++;
            if (niter > 1) {
                concat_lists(covered_faults, local_covered);
                for (i = atpg_opt->rtg_depth; i--;) {
                    vector = array_fetch(
                    unsigned *, sequences, i);
                    FREE(vector);
                }
                array_free(sequences);
                return covered_faults;
            }
        } else {
            niter = 0;
        }
        concat_lists(covered_faults, local_covered);
    } while (lsLength(info->faults) != 0);
    for (i         = atpg_opt->rtg_depth; i--;) {
        vector = array_fetch(
        unsigned *, sequences, i);
        FREE(vector);
    }
    array_free(sequences);
    return covered_faults;
}

int
random_propagate(fault, info, ss_info, seq_info)
        fault_t *fault;
        atpg_info_t *info;
        atpg_ss_info_t *ss_info;
        seq_info_t *seq_info;
{
    array_t  *word_vectors       = ss_info->prop_word_vectors;
    array_t  *good_start_state   = seq_info->good_state;
    array_t  *faulty_start_state = seq_info->faulty_state;
    array_t  *prop_sequence      = seq_info->prop_sequence;
    int      h, i, j, k, c, npi, npo, seq_length, good_value, faulty_value,
             n_changed_nodes;
    unsigned *vector, *prop_vector, *word_vector;
    unsigned word;
    unsigned *true_value         = ss_info->true_value;
    unsigned *true_state         = ss_info->true_state;
    unsigned *real_po_values     = ss_info->real_po_values;
    unsigned *faulty_state       = fault->current_state;
    unsigned *exdc_value         = NIL(
    unsigned);

    seq_length = array_n(word_vectors);
    npi        = info->n_real_pi;
    npo        = info->n_real_po;

    for (h = info->atpg_opt->n_random_prop_iter; h--;) {
        /* set values of random prop vectors */
        for (i = seq_length; i--;) {
            vector = array_fetch(
            unsigned *, word_vectors, i);
            for (j = npi; j--;) {
                vector[j] = (random() << 16) | (random() & 0xffff);
            }
        }

        /* set states to good and faulty latch values */
        for (i = ss_info->n_latch; i--;) {
            good_value = array_fetch(
            int, good_start_state, i);
            if (good_value == 0) {
                true_state[i] = ALL_ZERO;
            } else {
                true_state[i] = ALL_ONE;
            }
            faulty_value = array_fetch(
            int, faulty_start_state, i);
            if (faulty_value == 0) {
                faulty_state[i] = ALL_ZERO;
            } else {
                faulty_state[i] = ALL_ONE;
            }
        }
        /* for each vector of the sequence... */
        for (i = 0; i < seq_length; i++) {
            vector = array_fetch(
            unsigned *, word_vectors, i);

            /* first simulate to get true value and true state */
            atpg_network_simulate(ss_info, vector, true_state, true_value);

            n_changed_nodes = 0;
            atpg_sf_set_sim_masks(ss_info, fault);
            /* if latch values are different for this simulation... */
            for (j = ss_info->n_latch; j--;) {
                if (ss_info->sim_nodes[ss_info->pi_uid[j]].value !=
                    faulty_state[j]) {
                    ss_info->sim_nodes[ss_info->pi_uid[j]].value =
                            faulty_state[j];
                    ss_info->changed_node_indices[n_changed_nodes] =
                            ss_info->pi_uid[j];
                    n_changed_nodes += 1;
                }
            }
            ss_info->changed_node_indices[n_changed_nodes] = GET_ATPG_ID(fault->node);
            n_changed_nodes += 1;
            atpg_sim_tfo_array(ss_info, n_changed_nodes);
            atpg_sim_get_po(ss_info, real_po_values, fault->current_state);
            atpg_sf_reset_sim_masks(ss_info, fault);
            if (sf_record_covered_faults(fault, true_value, real_po_values,
                                         exdc_value, npo, NIL(sequence_t * ), 0)) {
                c      = fault->sequence_index;
                if (seq_length > array_n(prop_sequence)) {
                    for (j = seq_length - array_n(prop_sequence); j--;) {
                        prop_vector = ALLOC(
                        unsigned, npi);
                        array_insert_last(
                        unsigned *, prop_sequence, prop_vector);
                    }
                }
                for (j = seq_length; j--;) {
                    prop_vector = array_fetch(
                    unsigned *, prop_sequence, j);
                    word_vector = array_fetch(
                    unsigned *, word_vectors, j);
                    for (k = npi; k--;) {
                        word = word_vector[k];
                        prop_vector[k] = (EXTRACT_BIT(word, c) == 0) ? ALL_ZERO : ALL_ONE;
                    }
                    array_insert(
                    unsigned *, prop_sequence, j, prop_vector);
                }
                return (i + 1);
            }
        }
    }
    return 0;
}

void
fault_simulate(info, ss_info, exdc_info, cnt)
        atpg_info_t *info;
        atpg_ss_info_t *ss_info, *exdc_info;
        int cnt;
{
    lsList    covered_faults;
    array_t   *sequences = ss_info->word_vectors;
    fault_t   *f;
    lsHandle  handle;
    lsGeneric data;

    covered_faults = atpg_seq_single_fault_simulate(ss_info, exdc_info,
                                                    sequences, info->faults, ss_info->alloc_sequences, cnt);
    concat_lists(covered_faults, atpg_seq_single_fault_simulate(ss_info,
                                                                exdc_info, sequences, info->untested_faults,
                                                                ss_info->alloc_sequences, cnt));
    if (info->atpg_opt->verbosity > 0) {
        fprintf(sisout, "fault simulation covered %d\n", lsLength(covered_faults));
    }
    while (lsFirstItem(covered_faults, (lsGeneric * ) & f, &handle) == LS_OK) {
        f->sequence = ss_info->alloc_sequences[f->sequence_index];
        if (ATPG_DEBUG) {
            assert(atpg_verify_test(ss_info, f, f->sequence));
        }
        f->sequence->n_covers++;
        f->is_covered = TRUE;
        lsNewEnd(info->tested_faults, (lsGeneric) f, 0);
        lsRemoveItem(handle, &data);
    }
    lsDestroy(covered_faults, 0);
}

void
atpg_sf_set_sim_masks(info, fault)
        atpg_ss_info_t *info;
        fault_t *fault;
{
    atpg_sim_node_t *node;

    node = &(info->sim_nodes[GET_ATPG_ID(fault->node)]);
    if (fault->fanin == NIL(node_t)) {
        if (fault->value == S_A_1) {
            node->or_output_mask = ALL_ONE;
        } else {
            node->and_output_mask = ALL_ZERO;
        }
    } else {
        if (fault->value == S_A_1) {
            node->or_input_masks[fault->index] = ALL_ONE;
        } else {
            node->and_input_masks[fault->index] = ALL_ZERO;
        }
    }
}

void
atpg_sf_reset_sim_masks(info, fault)
        atpg_ss_info_t *info;
        fault_t *fault;
{
    atpg_sim_node_t *node;

    node = &(info->sim_nodes[GET_ATPG_ID(fault->node)]);
    if (fault->fanin == NIL(node_t)) {
        node->or_output_mask  = ALL_ZERO;
        node->and_output_mask = ALL_ONE;
    } else {
        node->or_input_masks[fault->index]  = ALL_ZERO;
        node->and_input_masks[fault->index] = ALL_ONE;
    }
}

static void
get_tfo(info, id, index)
        atpg_ss_info_t *info;
        int id;
        int *index;
{
    int i;

    if (!info->sim_nodes[id].visited) {
        info->sim_nodes[id].visited = TRUE;
        for (i = 0; i < info->sim_nodes[id].nfanout; i++) {
            get_tfo(info, (info->sim_nodes[id].fanout)[i], index);
        }
        info->tfo[*index] = id;
        (*index)++;
    }
}

static void
get_tfo_of_array(info, index, array_size)
        atpg_ss_info_t *info;
        int *index;
        int array_size;
{
    int id;

    for (; array_size--;) {
        id = info->changed_node_indices[array_size];
        get_tfo(info, id, index);
    }
}

/* assumes pi values are set */
static void
atpg_sim_tfo_array(info, array_size)
        atpg_ss_info_t *info;
        int array_size;
{
    int f, index;

    index = 0;
    get_tfo_of_array(info, &index, array_size);
    for (; index--;) {
        f = info->tfo[index];
        (*(info->sim_nodes[f].eval))(info->sim_nodes, f);
        info->sim_nodes[f].visited = FALSE;
    }
}

static void
atpg_sim_get_po(info, real_po_values, next_state)
        atpg_ss_info_t *info;
        unsigned *real_po_values;
        unsigned *next_state;
{
    int i, n_latch;

    n_latch = info->n_latch;
    for (i  = n_latch; i--;) {
        next_state[i] = info->sim_nodes[info->po_uid[i]].value;
    }
    for (i = info->n_real_po; i--;) {
        real_po_values[i] = info->sim_nodes[info->po_uid[i + n_latch]].value;
    }
}

static bool
sf_record_covered_faults(fault, true_value, po_values, exdc_value, npo,
                         vector_number, original_sequences, cnt)
        fault_t *fault;
        unsigned *true_value;
        unsigned *po_values;
        unsigned *exdc_value;
        int npo;
        int vector_number;
        sequence_t **original_sequences;
        int cnt;
{
    int      i, j;
    unsigned tval, fval, dcval;

    if (original_sequences == NIL(sequence_t * )) {
        for (i = npo; i--;) {
            tval  = true_value[i];
            fval  = po_values[i];
            dcval = (exdc_value == NIL(
            unsigned) ? 0 : exdc_value[i]);
            if ((tval ^ fval) & ~dcval) {
                for (j = 0; j < WORD_LENGTH; j++) {
                    if ((EXTRACT_BIT(dcval, j) == 0) &&
                        (EXTRACT_BIT(tval, j) != EXTRACT_BIT(fval, j))) {
                        fault->sequence_index = j;
                        return TRUE;
                    }
                }
            }
        }
    } else {
        for (i = npo; i--;) {
            tval  = true_value[i];
            fval  = po_values[i];
            dcval = (exdc_value == NIL(
            unsigned) ? 0 : exdc_value[i]);
            if ((tval ^ fval) & ~dcval) {
                for (j = 0; j < cnt; j++) {
                    if ((EXTRACT_BIT(dcval, j) == 0) &&
                        (EXTRACT_BIT(tval, j) != EXTRACT_BIT(fval, j))) {
                        if (vector_number < array_n(original_sequences[j]->vectors)) {
                            fault->sequence_index = j;
                            return TRUE;
                        }
                    }
                }
            }
        }
    }
    return FALSE;
}

static void
ss_record_covered_faults(po_values, faults_ptr, npo)
        unsigned *po_values;
        fault_t **faults_ptr;
        int npo;
{
    fault_t  *f;
    int      i, j, *true_value;
    unsigned val;

    true_value = ALLOC(
    int, npo);
    for (i = npo; i--;) {
        val = po_values[i];
        true_value[i] = EXTRACT_BIT(val, WORD_LENGTH - 1);
    }

    for (i = 0; i < WORD_LENGTH - 1; i++) {
        f = faults_ptr[i];
        if (f != NIL(fault_t)) {
            for (j = 0; j < npo; j++) {
                val = po_values[j];
                /* fault detected at some PO? */
                if (EXTRACT_BIT(val, i) != true_value[j]) {
                    f->is_covered = TRUE;
                    break;
                }
            }
        }
    }
    FREE(true_value);
}

static void
record_different_states(seq_info, state, n_latch)
        seq_info_t *seq_info;
        unsigned *state;
        int n_latch;
{
    array_t  *good_state   = seq_info->good_state;
    array_t  *faulty_state = seq_info->faulty_state;
    unsigned latch_value_i;
    int      i, tval, fval;

    for (i = n_latch; i--;) {
        latch_value_i = state[i];
        tval          = (EXTRACT_BIT(latch_value_i, 1) ? 1 : 0);
        array_insert(
        int, good_state, i, tval);
        fval = (EXTRACT_BIT(latch_value_i, 0) ? 1 : 0);
        array_insert(
        int, faulty_state, i, fval);
    }

}

/* create sequences that are tests for faults */
void
extract_test_sequences(info, ss_info, faults, sequences, n_tests_ptr, npi)
        atpg_info_t *info;
        atpg_ss_info_t *ss_info;
        lsList faults;
        array_t *sequences;
        int *n_tests_ptr;
        int npi;
{
    int        i, j, k, word, seq_length;
    sequence_t *sequence;
    fault_t    *f;
    lsGen      gen;
    unsigned   *word_vector, *vector;

    seq_length = array_n(sequences);
    for (i     = WORD_LENGTH; i--;) {
        ss_info->used[i] = FALSE;
    }
    foreach_fault(faults, gen, f) {
        ss_info->used[f->sequence_index] = TRUE;
    }
    for (i = WORD_LENGTH; i--;) {
        if (ss_info->used[i]) {
            sequence = ALLOC(sequence_t, 1);
            sequence->vectors = array_alloc(
            unsigned *, seq_length);
            sequence->n_covers = 0;
            if (n_tests_ptr != NIL(int)) {
                *n_tests_ptr += 1;
                sequence->index = *n_tests_ptr;
            }
            for (j                      = seq_length; j--;) {
                vector = ALLOC(
                unsigned, npi);
                word_vector = array_fetch(
                unsigned *, sequences, j);
                for (k = npi; k--;) {
                    word = word_vector[k];
                    vector[k] = (EXTRACT_BIT(word, i) == 0) ? ALL_ZERO : ALL_ONE;
                }
                array_insert(
                unsigned *, sequence->vectors, j, vector);
            }
            ss_info->alloc_sequences[i] = sequence;
            st_insert(info->sequence_table, (char *) sequence, (char *) 0);
        }
    }
    foreach_fault(faults, gen, f) {
        f->sequence = ss_info->alloc_sequences[f->sequence_index];
        if (ATPG_DEBUG) {
            assert(atpg_verify_test(ss_info, f, f->sequence));
        }
        f->sequence->n_covers++;
        f->is_covered = TRUE;
    }
}

void
atpg_set_sim_masks(info)
        atpg_ss_info_t *info;
{
    int             i;
    fault_t         *fault;
    atpg_sim_node_t *node;

    for (i = 0; i < WORD_LENGTH; i++) {
        if ((fault = info->faults_ptr[i]) != NIL(fault_t)) {
            node = &(info->sim_nodes[GET_ATPG_ID(fault->node)]);
            if (fault->fanin == NIL(node_t)) {
                if (fault->value == S_A_1) {
                    node->or_output_mask |= (1 << i);
                } else {
                    node->and_output_mask &= ~(1 << i);
                }
            } else {
                if (fault->value == S_A_1) {
                    node->or_input_masks[fault->index] |= (1 << i);
                } else {
                    node->and_input_masks[fault->index] &= ~(1 << i);
                }
            }
        }
    }
}

void
atpg_reset_sim_masks(info)
        atpg_ss_info_t *info;
{
    int             i, id;
    fault_t         *fault;
    atpg_sim_node_t *node;

    for (i = 0; i < WORD_LENGTH; i++) {
        if ((fault = info->faults_ptr[i]) != NIL(fault_t)) {
            id   = GET_ATPG_ID(fault->node);
            node = &(info->sim_nodes[id]);
            if (fault->fanin == NIL(node_t)) {
                node->or_output_mask  = ALL_ZERO;
                node->and_output_mask = ALL_ONE;
            } else {
                node->or_input_masks[fault->index]  = ALL_ZERO;
                node->and_input_masks[fault->index] = ALL_ONE;
            }
        }
    }
}

int
get_min_just_sequence(ss_info, fault, seq_info)
        atpg_ss_info_t *ss_info;
        fault_t *fault;
        seq_info_t *seq_info;
{
    array_t  *just_sequence = seq_info->just_sequence;
    unsigned *po_values, *state, *vector, po_j;
    int      seq_length, i, j;
    fault_t  **faults_ptr   = ss_info->faults_ptr;

    po_values  = ss_info->true_value;
    state      = ss_info->true_state;
    seq_length = array_n(just_sequence);

    for (i        = WORD_LENGTH; i--;) {
        faults_ptr[i] = NIL(fault_t);
    }
    faults_ptr[0] = fault;

    for (i = ss_info->n_latch; i--;) {
        state[i] = ss_info->reset_state[i];
    }
    atpg_set_sim_masks(ss_info);

    /* for each vector of the sequence... */
    for (i = 0; i < seq_length; i++) {
        vector = array_fetch(
        unsigned *, just_sequence, i);

        atpg_network_simulate(ss_info, vector, state, po_values);

        /* is fault effect propagated to a real primary output? */
        for (j = ss_info->n_real_po; j--;) {
            po_j = po_values[j];
            if (EXTRACT_BIT(po_j, 1) != EXTRACT_BIT(po_j, 0)) {
                fault->is_covered = TRUE;
                atpg_reset_sim_masks(ss_info);
                return (i + 1);
            }
        }
        /* is fault effect propagated to a next state line? */
        for (j = ss_info->n_latch; j--;) {
            po_j = state[j];
            if (EXTRACT_BIT(po_j, 1) != EXTRACT_BIT(po_j, 0)) {
                atpg_reset_sim_masks(ss_info);
                record_different_states(seq_info, state, ss_info->n_latch);
                return (i + 1);
            }
        }
    }
    atpg_reset_sim_masks(ss_info);
    return -1;
}

void
simulate_entire_sequence(ss_info, fault, seq_info, vectors)
        atpg_ss_info_t *ss_info;
        fault_t *fault;
        seq_info_t *seq_info;
        array_t *vectors;
{
    unsigned *po_values, *state, *vector;
    int      seq_length, i;
    fault_t  **faults_ptr = ss_info->faults_ptr;

    po_values  = ss_info->true_value;
    state      = ss_info->true_state;
    seq_length = array_n(vectors);

    for (i        = WORD_LENGTH; i--;) {
        faults_ptr[i] = NIL(fault_t);
    }
    faults_ptr[0] = fault;

    for (i = ss_info->n_latch; i--;) {
        state[i] = ss_info->reset_state[i];
    }
    atpg_set_sim_masks(ss_info);

    /* for each vector of the sequence... */
    for (i = 0; i < seq_length; i++) {
        vector = array_fetch(
        unsigned *, vectors, i);
        atpg_network_simulate(ss_info, vector, state, po_values);
    }
    record_different_states(seq_info, state, ss_info->n_latch);
    atpg_reset_sim_masks(ss_info);
}

bool
atpg_verify_test(ss_info, fault, test_sequence)
        atpg_ss_info_t *ss_info;
        fault_t *fault;
        sequence_t *test_sequence;
{
    array_t  *test_vectors = test_sequence->vectors;
    unsigned *po_values, *state, *vector, po_j;
    int      seq_length, i, j;
    fault_t  **faults_ptr  = ss_info->faults_ptr;

    po_values  = ss_info->true_value;
    state      = ss_info->true_state;
    seq_length = array_n(test_vectors);

    for (i        = WORD_LENGTH; i--;) {
        faults_ptr[i] = NIL(fault_t);
    }
    faults_ptr[0] = fault;

    for (i = ss_info->n_latch; i--;) {
        state[i] = ss_info->reset_state[i];
    }

    atpg_set_sim_masks(ss_info);

    /* for each vector of the sequence... */
    for (i = 0; i < seq_length; i++) {
        vector = array_fetch(
        unsigned *, test_vectors, i);

        atpg_network_simulate(ss_info, vector, state, po_values);

        /* is fault effect propagated to a real primary output? */
        for (j = ss_info->n_real_po; j--;) {
            po_j = po_values[j];
            if (EXTRACT_BIT(po_j, 1) != EXTRACT_BIT(po_j, 0)) {
                atpg_reset_sim_masks(ss_info);
                return TRUE;
            }
        }
    }
    atpg_reset_sim_masks(ss_info);
    return FALSE;
}

/* single sequence/parallel fault simulation */
lsList
seq_single_sequence_simulate(ss_info, test_sequence, fault_list)
        atpg_ss_info_t *ss_info;
        sequence_t *test_sequence;
        lsList fault_list;
{
    unsigned  *real_outputs = ss_info->true_value;
    unsigned  *states       = ss_info->true_state;
    array_t   *vectors      = test_sequence->vectors;
    unsigned  *vector;
    int       j, k, done, seq_length;
    fault_t   *fault;
    lsGeneric data;
    lsHandle  handle;
    lsGen     gen;
    lsList    covered_faults;

    seq_length = array_n(vectors);
    gen        = lsStart(fault_list);
    j          = 0;
    done       = FALSE;
    ss_info->faults_ptr[WORD_LENGTH - 1] = NIL(fault_t);
    do {
        /* Fill faults_ptr with next WORD_LENGTH - 1 faults.  (Last
         * position must contain NIL(fault_t)--the good machine.)
         */
        if (lsNext(gen, (lsGeneric * ) & fault, &handle) == LS_OK) {
            ss_info->faults_ptr[j++] = fault;
        } else {
            done = TRUE;
            for (; j < WORD_LENGTH - 1; j++) {
                ss_info->faults_ptr[j] = NIL(fault_t);
            }
        }
        if (j == (WORD_LENGTH - 1)) {
            atpg_set_sim_masks(ss_info);
            for (k = ss_info->n_latch; k--;) {
                states[k] = ss_info->reset_state[k];
            }
            for (k = 0; k < seq_length; k++) {
                vector = array_fetch(
                unsigned *, vectors, k);
                atpg_network_simulate(ss_info, vector, states, real_outputs);
                ss_record_covered_faults(real_outputs, ss_info->faults_ptr,
                                         ss_info->n_real_po);
            }
            atpg_reset_sim_masks(ss_info);
            j = 0;
        }
    } while (!done);
    lsFinish(gen);

    covered_faults = lsCreate();
    gen            = lsStart(fault_list);
    while (lsNext(gen, (lsGeneric * ) & fault, &handle) == LS_OK) {
        if (fault->is_covered) {
            fault->status   = TESTED;
            fault->sequence = test_sequence;
            if (ATPG_DEBUG) {
                atpg_verify_test(ss_info, fault, test_sequence);
            }
            test_sequence->n_covers += 1;
            lsRemoveItem(handle, &data);
            lsNewEnd(covered_faults, data, 0);
        }
    }
    lsFinish(gen);
    return covered_faults;
}

void
fault_simulate_to_get_final_state(ss_info, test_sequence, seq_info)
        atpg_ss_info_t *ss_info;
        sequence_t *test_sequence;
        seq_info_t *seq_info;
{
    array_t  *test_vectors = test_sequence->vectors;
    unsigned *po_values, *state, *vector;
    int      seq_length, i;
    unsigned latch_value;
    array_t  *good_state   = seq_info->good_state;

    po_values  = ss_info->true_value;
    state      = ss_info->true_state;
    seq_length = array_n(test_vectors);

    for (i = ss_info->n_latch; i--;) {
        state[i] = ss_info->reset_state[i];
    }

    /* for each vector of the sequence... */
    for (i = 0; i < seq_length; i++) {
        vector = array_fetch(
        unsigned *, test_vectors, i);
        atpg_network_simulate(ss_info, vector, state, po_values);
    }

    for (i = ss_info->n_latch; i--;) {
        latch_value = state[i];
        if (latch_value == ALL_ZERO) {
            array_insert(
            int, good_state, i, 0);
        } else {
            assert(latch_value == ALL_ONE);
            array_insert(
            int, good_state, i, 1);
        }
    }
}

static int
seq_length_cmp(obj1, obj2)
        char *obj1;
        char *obj2;
{
    sequence_t **s1, **s2;

    s1 = (sequence_t **) obj1;
    s2 = (sequence_t **) obj2;
    return (array_n((*s2)->vectors) - array_n((*s1)->vectors));
}

void
atpg_simulate_old_sequences(info, ss_info, exdc_info, fp_table, untested_faults)
        atpg_info_t *info;
        atpg_ss_info_t *ss_info, *exdc_info;
        st_table *fp_table;
        lsList untested_faults;
{
    int             i, n_seq, max_seq_length;
    fault_t         *f;
    fault_pattern_t fp;
    sequence_t      **sequences_ptr, *sequence;
    array_t         *sequences = ss_info->word_vectors;
    lsList          local_covered;
    lsGen           gen;
    st_generator    *sgen;

    foreach_fault(info->faults, gen, f) {
        fp.node  = f->node;
        fp.fanin = f->fanin;
        fp.value = (f->value == S_A_0) ? 0 : 1;
        if (st_lookup(fp_table, (char *) &fp, (char **) &sequence)) {
            f->sequence = sequence;
        }
    }
    atpg_fault_sequence_sim(info, ss_info);

    /* try all patterns on remaining faults */
    n_seq          = st_count(info->sequence_table);
    sequences_ptr  = ALLOC(sequence_t * , n_seq);
    i              = 0;
    max_seq_length = 0;
    st_foreach_item(info->sequence_table, sgen, (char **) &sequence, NIL(
    char *)) {
        sequences_ptr[i++] = sequence;
        if (array_n(sequence->vectors) > max_seq_length)
            max_seq_length = array_n(sequence->vectors);
    }

    /* parallel pattern, single fault */
    for (i = 0; i < n_seq; i += WORD_LENGTH) {
        extract_sequences(ss_info, sequences_ptr, i, MIN(i + WORD_LENGTH, n_seq),
                          max_seq_length, ss_info->n_real_pi);
        local_covered = atpg_seq_single_fault_simulate(ss_info, exdc_info,
                                                       sequences, info->faults, NIL(sequence_t * ), 0);
        concat_lists(local_covered, atpg_seq_single_fault_simulate(ss_info,
                                                                   exdc_info, sequences, untested_faults,
                                                                   NIL(sequence_t * ), 0));
        lsDestroy(local_covered, free_fault);
        reset_word_vectors(ss_info);
    }
    FREE(sequences_ptr);
}

static void
atpg_fault_sequence_sim(atpg_info, ss_info)
        atpg_info_t *atpg_info;
        atpg_ss_info_t *ss_info;
{
    unsigned   *true_outputs   = ss_info->true_value;
    unsigned   *faulty_outputs = ss_info->real_po_values;
    unsigned   *true_states    = ss_info->true_state;
    unsigned   *faulty_states  = ss_info->faulty_state;
    unsigned   *vectors;
    array_t    *word_vectors   = ss_info->word_vectors;
    int        j, k, done, max_seq_length;
    fault_t    *fault;
    lsGeneric  data;
    lsHandle   handle;
    lsGen      gen;
    sequence_t *sequence;

    gen            = lsStart(atpg_info->faults);
    j              = 0;
    max_seq_length = 0;
    done           = FALSE;
    do {
        if (lsNext(gen, (lsGeneric * ) & fault, &handle) == LS_OK) {
            sequence = fault->sequence;
            if (sequence != NIL(sequence_t)) {
                if (array_n(sequence->vectors) > max_seq_length)
                    max_seq_length = array_n(sequence->vectors);
                ss_info->faults_ptr[j++] = fault;
            }
        } else {
            done = TRUE;
            for (; j < WORD_LENGTH; j++) {
                ss_info->faults_ptr[j] = NIL(fault_t);
            }
        }
        if (j == WORD_LENGTH) {
            fillin_word_vectors(ss_info, max_seq_length, ss_info->n_real_pi);
            for (k = ss_info->n_latch; k--;) {
                faulty_states[k] = true_states[k] = ss_info->reset_state[k];
            }
            for (k         = 0; k < max_seq_length; k++) {
                vectors = array_fetch(
                unsigned *, word_vectors, k);
                atpg_network_simulate(ss_info, vectors, true_states, true_outputs);
                atpg_set_sim_masks(ss_info);
                atpg_network_simulate(ss_info, vectors, faulty_states, faulty_outputs);
                atpg_reset_sim_masks(ss_info);
                fp_record_covered_faults(ss_info, true_outputs, faulty_outputs);
            }
            j              = 0;
            max_seq_length = 0;
            reset_word_vectors(ss_info);
        }
    } while (!done);
    lsFinish(gen);

    gen = lsStart(atpg_info->faults);
    while (lsNext(gen, (lsGeneric * ) & fault, &handle) == LS_OK) {
        if (fault->is_covered) {
            lsRemoveItem(handle, &data);
            free_fault((fault_t *) data);
        }
    }
    lsFinish(gen);
}

void
extract_sequences(info, sequences_ptr, from, to, seq_length, npi)
        atpg_ss_info_t *info;
        sequence_t **sequences_ptr;
        int from;
        int to;
        int seq_length;
        int npi;
{
    int      h, i, j;
    array_t  *vectors;
    array_t  *word_vectors = info->word_vectors;
    unsigned *vector, *word_vector;

    if (array_n(word_vectors) < seq_length) {
        lengthen_word_vectors(info, seq_length - array_n(word_vectors), npi);
    }

    for (h = from; h < to; h++) {
        vectors = sequences_ptr[h]->vectors;
        for (i  = array_n(vectors); i--;) {
            vector = array_fetch(
            unsigned *, vectors, i);
            word_vector = array_fetch(
            unsigned *, word_vectors, i);
            for (j = npi; j--;) {
                if (vector[j] == ALL_ZERO) {
                    word_vector[j] &= ~(1 << h);
                } else {
                    assert(vector[j] == ALL_ONE);
                    word_vector[j] |= (1 << h);
                }
            }
        }
    }
}

static void
fp_record_covered_faults(info, true_outputs, faulty_outputs)
        atpg_ss_info_t *info;
        unsigned *true_outputs;
        unsigned *faulty_outputs;
{
    int     i, j;
    fault_t *f;

    for (i = 0; i < WORD_LENGTH; i++) {
        f = info->faults_ptr[i];
        if (f != NIL(fault_t)) {
            for (j = 0; j < info->n_real_po; j++) {
                if (EXTRACT_BIT(true_outputs[j], i) !=
                    EXTRACT_BIT(faulty_outputs[j], i)) {
                    f->is_covered = TRUE;
                    break;
                }
            }
        }
    }
}

static int
seq_index_cmp(obj1, obj2)
        char *obj1;
        char *obj2;
{
    sequence_t **s1, **s2;

    s1 = (sequence_t **) obj1;
    s2 = (sequence_t **) obj2;
    return ((*s2)->index - (*s1)->index);
}

void
reverse_fault_simulate(info, ss_info)
        atpg_info_t *info;
        atpg_ss_info_t *ss_info;
{
    long            start_time;
    st_table        *table = info->sequence_table;
    st_generator    *sgen;
    sequence_t      *sequence;
    sequence_t      **sequences_ptr;
    int             i, j, n_seq, count;
    lsList          covered;
    fault_t         *fault;
    lsHandle        handle;
    fault_pattern_t fault_info;
    lsGeneric       data;
    array_t         *vectors;
    unsigned        *vector;

    start_time = util_cpu_time();
    assert(lsLength(info->faults) == 0);
    lsDestroy(info->faults, free_fault);
    atpg_gen_faults(info);
    /* retain only testable faults */
    count = lsLength(info->faults);
    while (count-- && lsFirstItem(info->faults, (lsGeneric * ) & fault, &handle) == LS_OK) {
        fault_info.node  = fault->node;
        fault_info.fanin = fault->fanin;
        fault_info.value = (fault->value == S_A_0) ? 0 : 1;
        lsRemoveItem(handle, &data);
        if (st_lookup(info->redund_table, (char *) &fault_info, NIL(char *))) {
            free_fault((fault_t *) data);
        } else {
            lsNewEnd(info->faults, (lsGeneric) data, 0);
        }
    }

    n_seq         = st_count(table);
    sequences_ptr = ALLOC(sequence_t * , n_seq);
    i             = 0;
    st_foreach_item(table, sgen, (char **) &sequence, NIL(
    char *)) {
        sequences_ptr[i++] = sequence;
        st_delete(table, (char **) &sequence, NIL(
        char *));
    }
    qsort((void *) sequences_ptr, n_seq, sizeof(sequence_t *), seq_index_cmp);

    for (i = 0; i < n_seq; i++) {
        covered = seq_single_sequence_simulate(ss_info, sequences_ptr[i],
                                               info->faults);
        if (lsLength(covered) > 0) {
            st_insert(table, (char *) sequences_ptr[i], (char *) 0);
        } else {
            vectors = sequences_ptr[i]->vectors;
            for (j  = array_n(vectors); j--;) {
                vector = array_fetch(
                unsigned *, vectors, j);
                FREE(vector);
            }
            array_free(vectors);
            FREE(sequences_ptr[i]);
        }
        lsDestroy(covered, free_fault);
        if (lsLength(info->faults) == 0) {
            i++;
            break;
        }
    }
    if (info->atpg_opt->build_product_machines) {
        assert(lsLength(info->faults) == 0);
    } else {
        lsDestroy(info->faults, free_fault);
        info->faults = lsCreate();
    }
    for (; i < n_seq; i++) {
        vectors = sequences_ptr[i]->vectors;
        for (j  = array_n(vectors); j--;) {
            vector = array_fetch(
            unsigned *, vectors, j);
            FREE(vector);
        }
        array_free(vectors);
        FREE(sequences_ptr[i]);
    }
    FREE(sequences_ptr);
    info->time_info->reverse_fault_sim += (util_cpu_time() - start_time);
}

