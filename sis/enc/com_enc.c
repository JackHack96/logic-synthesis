#include "enc_int.h"
#include "sis.h"

#define EQN 5
#define ITER 6

static void infeasible_cnt();

bool enc_debug = 0;

static void infeasible_cnt(list, bound) dic_family_t *list;
int bound;
{
  pset lastp, p;
  int less, cnt1, cnt2, i, base;
  unsigned int val;

  less = 0;
  lastp = list->dic + list->dcount * list->dic_size;
  for (p = list->dic; p < lastp; p += list->dic_size) {
    cnt1 = 0;
    cnt2 = 0;
    foreach_set_element(lhs_dic(p), i, val, base) { cnt1++; }
    foreach_set_element(rhs_dic(p), i, val, base) { cnt2++; }
    if (cnt1 > bound || cnt2 > bound) {
      less++;
    }
  }
  (void)fprintf(sisout, "Reduce list by %-1d\n", less);
}
