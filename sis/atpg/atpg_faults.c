#include "atpg_int.h"
#include "sis.h"

static void atpg_node_faults();
static void atpg_collapse_node_faults();
static void pi_fault_collapse();
static void collapse_fanin_fault();

void atpg_gen_faults(info) atpg_info_t *info;
{
  array_t *nodevec;
  int i;
  node_t *node;

  info->faults = lsCreate();
  nodevec = network_dfs(info->network);
  /* faults closer to primary outputs first */
  for (i = array_n(nodevec); i--;) {
    node = array_fetch(node_t *, nodevec, i);
    if (!st_lookup(info->control_node_table, (char *)node, NIL(char *))) {
      atpg_collapse_node_faults(info->faults, node, info);
    }
  }
  array_free(nodevec);
}

void atpg_gen_node_faults(info, node_vec) atpg_info_t *info;
array_t *node_vec;
{
  node_t *node;
  int i;

  info->faults = lsCreate();
  for (i = array_n(node_vec); i--;) {
    node = array_fetch(node_t *, node_vec, i);
    atpg_node_faults(info->faults, node, info);
  }
}

/* all transitive fanin of node must have been processed already
 * for equivalent faults, */
static void atpg_collapse_node_faults(faults, node, info) lsList faults;
node_t *node;
atpg_info_t *info;
{
  node_function_t node_func;
  fault_t *fault;
  node_t *fanin;
  int i;

  node_func = node_function(node);

  /* don't collapse faults on complex gates */
  if (node_func == NODE_COMPLEX) {
    atpg_node_faults(faults, node, info);
    return;
  }

  if (node_func == NODE_PO || node_func == NODE_0 || node_func == NODE_1) {
    return;
  }

  /* if PI choose equivalent fault */
  if (node_func == NODE_PI) {
    switch (node_num_fanout(node)) {
    case 0:
      return;
    case 1:
      pi_fault_collapse(faults, node, info);
      return;
    default:
      /* no collapsing possible */
      fault = new_fault(node, NIL(node_t), S_A_0, info);
      lsNewEnd(faults, (lsGeneric)fault, 0);
      fault = new_fault(node, NIL(node_t), S_A_1, info);
      lsNewEnd(faults, (lsGeneric)fault, 0);
      return;
    }
  }

  /* process each fanin */
  foreach_fanin(node, i, fanin) {
    /* only handle branches of fanout-points */
    if (node_num_fanout(fanin) > 1) {
      collapse_fanin_fault(faults, node, fanin, info);
    }
  }
}

/* node is a PI with single fanout. For AND gate, if it is the first fanin
 * then don't collapse s-a-0. If OR gate, then don't collapse s-a-1 on first
 * fanin. For both AND gate and OR gate choose all s-a-1 and s-a-0
 * respectively */
static void pi_fault_collapse(faults, node, info) lsList faults;
node_t *node;
atpg_info_t *info;
{
  node_t *fanout;
  stuck_value_t value, opp_value;
  input_phase_t phase;
  node_function_t fanout_func;
  fault_t *fault;

  assert(node_num_fanout(node) == 1);
  fanout = node_get_fanout(node, 0);
  fanout_func = node_function(fanout);

  /* equivalence exists only for AND and OR gates (this includes NOR
   * and NAND and either phases of inputs */
  if (fanout_func == NODE_AND || fanout_func == NODE_OR) {
    phase = node_input_phase(fanout, node);
    if (node_get_fanin_index(fanout, node) == 0) {
      /* AND gate S_A_0 are equivalent */
      if (fanout_func == NODE_AND) {
        value = (phase == POS_UNATE ? S_A_0 : S_A_1);
      }
      /* OR gate S_A_1 are equivalent */
      else {
        value = (phase == POS_UNATE ? S_A_1 : S_A_0);
      }
      fault = new_fault(node, NIL(node_t), value, info);
      lsNewEnd(faults, (lsGeneric)fault, 0);
    }
    if (fanout_func == NODE_AND) {
      opp_value = (phase == POS_UNATE ? S_A_1 : S_A_0);
    } else {
      opp_value = (phase == POS_UNATE ? S_A_0 : S_A_1);
    }
    fault = new_fault(node, NIL(node_t), opp_value, info);
    lsNewEnd(faults, (lsGeneric)fault, 0);
  }
  /* no equivalence possible */
  else {
    fault = new_fault(node, NIL(node_t), S_A_0, info);
    lsNewEnd(faults, (lsGeneric)fault, 0);
    fault = new_fault(node, NIL(node_t), S_A_1, info);
    lsNewEnd(faults, (lsGeneric)fault, 0);
  }
}

/* fanin must be non-null */
static void collapse_fanin_fault(faults, node, fanin, info) lsList faults;
node_t *node;
node_t *fanin;
atpg_info_t *info;
{
  stuck_value_t value, opp_value;
  input_phase_t phase;
  node_function_t node_func;
  fault_t *fault;

  assert(fanin != NIL(node_t));
  assert(node_num_fanout(fanin) > 1);

  node_func = node_function(node);

  if (node_func == NODE_AND || node_func == NODE_OR) {
    phase = node_input_phase(node, fanin);
    if (node_get_fanin_index(node, fanin) == 0) {
      /* AND gate S_A_0 are equivalent */
      if (node_func == NODE_AND) {
        value = (phase == POS_UNATE ? S_A_0 : S_A_1);
      }
      /* OR gate S_A_1 are equivalent */
      else {
        value = (phase == POS_UNATE ? S_A_1 : S_A_0);
      }
      fault = new_fault(node, fanin, value, info);
      lsNewEnd(faults, (lsGeneric)fault, 0);
    }
    if (node_func == NODE_AND) {
      opp_value = (phase == POS_UNATE ? S_A_1 : S_A_0);
    } else {
      opp_value = (phase == POS_UNATE ? S_A_0 : S_A_1);
    }
    fault = new_fault(node, fanin, opp_value, info);
    lsNewEnd(faults, (lsGeneric)fault, 0);
  }
  /* no equivalence possible */
  else {
    fault = new_fault(node, fanin, S_A_0, info);
    lsNewEnd(faults, (lsGeneric)fault, 0);
    fault = new_fault(node, fanin, S_A_1, info);
    lsNewEnd(faults, (lsGeneric)fault, 0);
  }
}

static void atpg_node_faults(faults, node, info) lsList faults;
node_t *node;
atpg_info_t *info;
{
  int i;
  node_t *fanin;
  fault_t *fault;

  /* faults on inputs */
  foreach_fanin(node, i, fanin) {
    fault = new_fault(node, fanin, S_A_0, info);
    lsNewEnd(faults, (lsGeneric)fault, 0);
    fault = new_fault(node, fanin, S_A_1, info);
    lsNewEnd(faults, (lsGeneric)fault, 0);
  }
  /* faults on outputs */
  fault = new_fault(node, NIL(node_t), S_A_0, info);
  lsNewEnd(faults, (lsGeneric)fault, 0);
  fault = new_fault(node, NIL(node_t), S_A_1, info);
  lsNewEnd(faults, (lsGeneric)fault, 0);
}

fault_t *new_fault(node, fanin, value, info) node_t *node;
node_t *fanin;
stuck_value_t value;
atpg_info_t *info;
{
  fault_t *f;

  f = ALLOC(fault_t, 1);
  f->node = node;
  f->fanin = fanin;
  if (fanin == NIL(node_t)) {
    f->index = -1;
  } else {
    f->index = node_get_fanin_index(node, fanin);
  }
  f->value = value;
  f->status = UNTESTED;
  f->is_covered = FALSE;
  f->sequence = NIL(sequence_t);
  f->current_state = ALLOC(unsigned, info->n_latch);
  return f;
}

void free_fault(fault) fault_t *fault;
{
  FREE(fault->current_state);
  FREE(fault);
}
