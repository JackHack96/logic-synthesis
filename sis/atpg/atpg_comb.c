
#include "atpg_int.h"
#include "sis.h"

static void atpg_comb_network_simulate();
static void atpg_sim_set_pi();
static void get_tfo();
static void atpg_sim_tfo();
static void atpg_sim_get_po();
static bool sf_comb_record_covered_faults();
static void atpg_fault_pattern_sim();
static void fp_comb_record_covered_faults();

sequence_t *derive_comb_test(info, n_var, pos) atpg_ss_info_t *info;
int n_var;
int pos;
{
  array_t *word_vectors = info->word_vectors;
  sequence_t *sequence;
  unsigned *vector, *word_vector;
  int i, v, ind;
  sat_input_t sv;

  if (array_n(word_vectors) == 0) {
    lengthen_word_vectors(info, 1, info->n_pi);
  }

  sequence = ALLOC(sequence_t, 1);
  sequence->vectors = array_alloc(unsigned *, 1);
  vector = ALLOC(unsigned, info->n_pi);
  array_insert(unsigned *, sequence->vectors, 0, vector);
  word_vector = array_fetch(unsigned *, word_vectors, 0);
  for (i = 0; i < info->n_pi; i++) {
    vector[i] = ALL_ONE;
  }
  for (i = 0; i < n_var; i++) {
    sv = array_fetch(sat_input_t, info->sat_input_vars, i);
    v = sat_get_value(info->atpg_sat, sv.sat_id);
    assert(st_lookup_int(info->pi_po_table, sv.info, &ind));
    if (v == 0) {
      vector[ind] = ALL_ZERO;
      word_vector[ind] &= ~(1 << pos);
    } else {
      vector[ind] = ALL_ONE;
      word_vector[ind] |= (1 << pos);
    }
  }
  return sequence;
}

static void atpg_comb_network_simulate(info, pi_values,
                                       po_values) atpg_ss_info_t *info;
unsigned *pi_values;
unsigned *po_values;
{
  int i;
  atpg_sim_node_t *sim_nodes;

  sim_nodes = info->sim_nodes;
  for (i = info->n_pi; i--;) {
    sim_nodes[info->pi_uid[i]].value = pi_values[i];
  }
  for (i = 0; i < info->n_nodes; i++) {
    (*(sim_nodes[i].eval))(sim_nodes, i);
  }
  for (i = info->n_po; i--;) {
    po_values[i] = sim_nodes[info->po_uid[i]].value;
  }
}

lsList atpg_comb_single_fault_simulate(info, exdc_info, word_vectors,
                                       fault_list) atpg_ss_info_t *info;
atpg_ss_info_t *exdc_info;
array_t *word_vectors;
lsList fault_list;
{
  unsigned *true_value, *po_values, *exdc_value, *pattern;
  fault_t *fault, *prev_fault;
  lsList covered_faults;
  lsGen gen1;
  lsGeneric data;
  lsHandle handle1;

  true_value = info->all_true_value;
  po_values = info->all_po_values;
  exdc_value = NIL(unsigned);
  pattern = array_fetch(unsigned *, word_vectors, 0);

  /* first simulate to get true values */
  atpg_comb_network_simulate(info, pattern, true_value);

  /* simulate don't cares */
  if (exdc_info != NIL(atpg_ss_info_t)) {
    exdc_value = exdc_info->all_true_value;
    atpg_comb_network_simulate(exdc_info, pattern, exdc_value);
  }

  gen1 = lsStart(fault_list);
  covered_faults = lsCreate();
  prev_fault = NIL(fault_t);
  while (lsNext(gen1, (lsGeneric *)&fault, &handle1) == LS_OK) {
    atpg_sf_set_sim_masks(info, fault);
    if (prev_fault != NIL(fault_t)) {
      if (node_function(prev_fault->node) == NODE_PI) {
        atpg_sim_set_pi(info, pattern);
      }
      atpg_sim_tfo(info, GET_ATPG_ID(prev_fault->node));
    }
    atpg_sim_tfo(info, GET_ATPG_ID(fault->node));
    atpg_sim_get_po(info, po_values);
    atpg_sf_reset_sim_masks(info, fault);
    prev_fault = fault;
    if (sf_comb_record_covered_faults(fault, true_value, po_values, exdc_value,
                                      info->n_po)) {
      lsRemoveItem(handle1, &data);
      lsNewEnd(covered_faults, (lsGeneric)data, 0);
    }
  }
  lsFinish(gen1);
  return covered_faults;
}

static void atpg_sim_set_pi(info, pi_values) atpg_ss_info_t *info;
unsigned *pi_values;
{
  int i;

  for (i = info->n_pi; i--;) {
    info->sim_nodes[info->pi_uid[i]].value = pi_values[i];
  }
}

static void get_tfo(info, id, index) atpg_ss_info_t *info;
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

/* assumes pi values are set */
static void atpg_sim_tfo(info, id) atpg_ss_info_t *info;
int id;
{
  int f, index;

  index = 0;
  get_tfo(info, id, &index);
  for (; index--;) {
    f = info->tfo[index];
    (*(info->sim_nodes[f].eval))(info->sim_nodes, f);
    info->sim_nodes[f].visited = FALSE;
  }
}

static void atpg_sim_get_po(info, po_values) atpg_ss_info_t *info;
unsigned *po_values;
{
  int i;

  for (i = info->n_po; i--;) {
    po_values[i] = info->sim_nodes[info->po_uid[i]].value;
  }
}

static bool sf_comb_record_covered_faults(fault, true_value, po_values,
                                          exdc_value, npo) fault_t *fault;
unsigned *true_value;
unsigned *po_values;
unsigned *exdc_value;
int npo;
{
  int i, j;
  unsigned tval, fval, dcval;

  for (i = npo; i--;) {
    tval = true_value[i];
    fval = po_values[i];
    dcval = (exdc_value == NIL(unsigned) ? 0 : exdc_value[i]);
    if ((tval ^ fval) & ~dcval) {
      for (j = 0; j < WORD_LENGTH; j++) {
        if ((EXTRACT_BIT(dcval, j) == 0) &&
            (EXTRACT_BIT(tval, j) != EXTRACT_BIT(fval, j))) {
          break;
        }
      }
      fault->sequence_index = j;
      return TRUE;
    }
  }
  return FALSE;
}

void atpg_comb_simulate_old_sequences(info, ss_info, exdc_info,
                                      fault_pattern_table,
                                      untested_faults) atpg_info_t *info;
atpg_ss_info_t *ss_info;
atpg_ss_info_t *exdc_info;
st_table *fault_pattern_table;
lsList untested_faults;
{
  int i, n_seq;
  fault_t *f;
  fault_pattern_t fp;
  sequence_t **sequences_ptr, *sequence;
  lsList local_covered;
  lsGen gen;
  st_generator *sgen;

  foreach_fault(info->faults, gen, f) {
    fp.node = f->node;
    fp.fanin = f->fanin;
    fp.value = (f->value == S_A_0) ? 0 : 1;
    if (st_lookup(fault_pattern_table, (char *)&fp, (char **)&sequence)) {
      f->sequence = sequence;
    }
  }
  atpg_fault_pattern_sim(info, ss_info);

  n_seq = st_count(info->sequence_table);
  sequences_ptr = ALLOC(sequence_t *, n_seq);
  i = 0;
  st_foreach_item(info->sequence_table, sgen, (char **)&sequence, NIL(char *)) {
    sequences_ptr[i++] = sequence;
  }

  /* parallel pattern, single fault */
  for (i = 0; i < n_seq; i += WORD_LENGTH) {
    extract_sequences(ss_info, sequences_ptr, i, MIN(i + WORD_LENGTH, n_seq), 1,
                      ss_info->n_pi);
    local_covered = atpg_comb_single_fault_simulate(
        ss_info, exdc_info, ss_info->word_vectors, info->faults,
        NIL(sequence_t *), 0);
    concat_lists(local_covered, atpg_seq_single_fault_simulate(
                                    ss_info, exdc_info, ss_info->word_vectors,
                                    untested_faults, NIL(sequence_t *), 0));
    lsDestroy(local_covered, free_fault);
    reset_word_vectors(ss_info);
  }
  FREE(sequences_ptr);
}

static void atpg_fault_pattern_sim(info, ss_info) atpg_info_t *info;
atpg_ss_info_t *ss_info;
{
  unsigned *true_outputs, *faulty_outputs, *pattern;
  int j, done;
  fault_t *fault;
  lsGeneric data;
  lsHandle handle;
  lsGen gen;

  true_outputs = ss_info->all_true_value;
  faulty_outputs = ss_info->all_po_values;
  gen = lsStart(info->faults);
  j = 0;
  done = FALSE;
  do {
    if (lsNext(gen, (lsGeneric *)&fault, &handle) == LS_OK) {
      if (fault->sequence != NIL(sequence_t)) {
        ss_info->faults_ptr[j++] = fault;
      }
    } else {
      done = TRUE;
      for (; j < WORD_LENGTH; j++) {
        ss_info->faults_ptr[j] = NIL(fault_t);
      }
    }
    if (j == WORD_LENGTH) {
      fillin_word_vectors(ss_info, 1, info->n_pi);
      pattern = array_fetch(unsigned *, ss_info->word_vectors, 0);
      atpg_comb_network_simulate(ss_info, pattern, true_outputs);
      atpg_set_sim_masks(ss_info);
      atpg_comb_network_simulate(ss_info, pattern, faulty_outputs);
      atpg_reset_sim_masks(ss_info);
      fp_comb_record_covered_faults(ss_info, true_outputs, faulty_outputs);
      j = 0;
    }
  } while (!done);
  lsFinish(gen);

  gen = lsStart(info->faults);
  while (lsNext(gen, (lsGeneric *)&fault, &handle) == LS_OK) {
    if (fault->is_covered) {
      lsRemoveItem(handle, &data);
      FREE(data);
    }
  }
  lsFinish(gen);
}

static void fp_comb_record_covered_faults(info, true_outputs,
                                          faulty_outputs) atpg_ss_info_t *info;
unsigned *true_outputs;
unsigned *faulty_outputs;
{
  int i, j;
  fault_t *f;

  for (i = 0; i < WORD_LENGTH; i++) {
    f = info->faults_ptr[i];
    if (f != NIL(fault_t)) {
      for (j = 0; j < info->n_po; j++) {
        if (EXTRACT_BIT(true_outputs[j], i) !=
            EXTRACT_BIT(faulty_outputs[j], i)) {
          f->is_covered = TRUE;
          break;
        }
      }
    }
  }
}
