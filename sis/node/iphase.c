
#include "sis.h"

input_phase_t node_input_phase(node, fanin) node_t *node, *fanin;
{
  register int i;
  register pset last, p;
  register bool pos_used, neg_used;

  if (node_function(node) == NODE_PO) {
    fail("node_input_phase: primary output node does not have a function");
  }

  pos_used = neg_used = FALSE;

  i = node_get_fanin_index(node, fanin);
  if (i == -1) {
    return PHASE_UNKNOWN;
  } else {
    foreach_set(node->F, last, p) {
      switch (GETINPUT(p, i)) {
      case ONE:
        pos_used = TRUE;
        break;
      case ZERO:
        neg_used = TRUE;
        break;
      case TWO:
        break;
      default:
        fail("node_input_phase: bad cube");
        break;
      }
    }
  }

  if (pos_used) {
    if (neg_used) {
      return BINATE;
    } else {
      return POS_UNATE;
    }
  } else {
    if (neg_used) {
      return NEG_UNATE;
    } else {
      return PHASE_UNKNOWN;
    }
  }
}
