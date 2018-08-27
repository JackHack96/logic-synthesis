
/* SCCSID %W%  */
#include "max.h"
#include "global.h"
#include "merge.h"
#include "sis/util/util.h"
#include "stack.h"
#include "struct.h"
#include "user.h"

PRIMES max;
extern struct isomor *iso;
extern int *b_cover;

maximal_compatibles() {
  register i, j, k, m, l;
  int stop_i;

  max = (PRIMES)0;
  alloc_max_block(0);
  k = 0;

  user.stat.n_max = 0;

  stop_i = -1;
  for (i = num_st - 2; i > stop_i; i--) { /* Column indexing */

    for (j = num_st - 1; j > i; j--) { /* Row indexing */
      if (!iso[i].uvoid && !iso[j].uvoid) {
        if (xstack[k].status & COMPATIBLE) {
          /* 0 level compatible */
          max[user.stat.n_max]->state[0] = i;
          max[user.stat.n_max]->state[1] = j;
          max[user.stat.n_max]->num = 2;
          increase_max_num();
          stop_i = i;
        }
      }
      k++;
    }
  }
  for (/*i*/; i > -1; i--) { /* Column indexing */

    if (!iso[i].uvoid) {

      enlarge_or_add(i);

      for (l = 0; l < user.stat.n_max; l++)
        for (j = l + 1; j < user.stat.n_max; j++) {
          PRIME *temp_max;

          if ((k = included(l, j)) != -1) {
            temp_max = max[k];
            user.stat.n_max--;
            max[k] = max[user.stat.n_max];
            max[user.stat.n_max] = temp_max;
            if (k == l) {
              l--;
              break;
            }
            j--;
          }
        }
    }
  }

  free_stack_head();

  /* High bound */

  if (user.stat.n_iso && (user.opt.solution < 2))
    iso_generate();
  user.stat.high = user.stat.n_max > num_st ? num_st : user.stat.n_max;
}

bound() {
  register i, k;

  get_implied();

  switch (user.opt.solution) {
  case 2: /* From the base maximal compatibles */
  case 3: /* Entirely for jac4 */
    if (user.stat.n_iso) {
      iso_close(user.stat.n_iso);
      if (user.opt.solution == 2)
        break;
    }
  case 1: /* binate covering to maximal */
    user.stat.low = max_cover(0);
    if (user.stat.low == (user.stat.high = max_cover(1))) {
      prime = max;
      p_num = user.stat.n_max;
      make_null(5);
      make_null(6);
    } else {
      i = 0;
      while (b_cover[i] != -1) {
        max[i] = max[b_cover[i]];
        i++;
      }
      user.stat.n_max = i;
    }
  default:
    if (user.level > 6) {
      user.stat.low = max_cover(0);
      if (is_it_closed()) {
        prime = max;
        p_num = user.stat.n_max;
        make_null(5);
        make_null(6);
      }
    }
    break;
  }

  if (user.opt.verbose > 1)
    say_max();
}

included(i, j) {
  register k, l;
  int small, big, not_match;

  if (max[i]->num > max[j]->num) {
    small = j;
    big = i;
  } else {
    small = i;
    big = j;
  }

  for (k = 0; k < max[small]->num; k++) {
    not_match = 1;
    for (l = 0; l < max[big]->num; l++) {
      if (max[small]->state[k] == max[big]->state[l]) {
        not_match = 0;
        break;
      }
    }
    if (not_match)
      return -1;
  }
  return small;
}

enlarge_or_add(column) {
  register i, j, pair;
  int stop, id;
  int work;
  int state1, state2;

#ifdef DEBUG
  (void)printf("enlarge_or_add %d max_com %d\n", column, user.stat.n_max);
#endif
  stop = user.stat.n_max;
  for (i = 0; i < stop; i++) {
    pair = 0;
    work = user.stat.n_max;
    for (j = 0; j < max[i]->num; j++) {
      state1 = column;
      state2 = max[i]->state[j];
      id = (num_st - state1 - 2) * (num_st - state1 - 1) / 2 - state2 + num_st -
           1;
      if (xstack[id].status & COMPATIBLE) {
        max[user.stat.n_max]->state[pair++] = max[i]->state[j];
      }
    }
    if (pair > 1) {
      max[user.stat.n_max]->state[pair] = column;
      if (pair == max[i]->num) {
        max[i]->state[pair] = column;
        max[i]->num++;
      } else {
        max[user.stat.n_max]->num = pair + 1;
        increase_max_num();
      }
      for (j = 0; j < pair; j++) {
        state1 = max[work]->state[j]; /* temp_max[j]; */
        state2 = column;
        id = (num_st - state2 - 2) * (num_st - state2 - 1) / 2 - state1 +
             num_st - 1;
        xstack[id].status |= USED;
      }
    }
  }
  for (i = num_st - 1; i > column; i--) {
    if (!iso[i].uvoid) {
      state2 = i;
      state1 = column;
      id = (num_st - state1 - 2) * (num_st - state1 - 1) / 2 - state2 + num_st -
           1;
      if (!(xstack[id].status & USED) && (xstack[id].status & COMPATIBLE)) {
        max[user.stat.n_max]->state[0] = i;
        max[user.stat.n_max]->state[1] = column;
        max[user.stat.n_max]->num = 2;
        increase_max_num();
      }
    }
  }
}

alloc_max_block(max_size) {
  register i, xsize;

  xsize = sizeof(PRIME) + sizeof(int) * (num_st - 1);
  if (max_size) {
    if (!(max = REALLOC(PRIME *, max, max_size + N_MAX)))
      panic("alloc20");
  } else {
    if (!(max = ALLOC(PRIME *, N_MAX)))
      panic("alloc21");
  }

  /*
          if (!(max[max_size] = (PRIME *) ALLOC(char,
                  (sizeof(PRIME) + sizeof(int)*(num_st - 1))* N_MAX)))
                  panic("alloc22");

          for (i=0 ; i< N_MAX; i++) {
                  max[i+max_size] = (PRIME *)((int)max[max_size] + xsize*i);
                  max[i+max_size]->state = (int *)((int)max[i+max_size]
                          + sizeof(PRIME));
          }
  */
  for (i = 0; i < N_MAX; i++) {
    if (!(max[i + max_size] = ALLOC(PRIME, 1)))
      panic("alloc_max");
    if (!(max[i + max_size]->state = ALLOC(int, num_st - 1)))
      panic("alloc_max");
  }
}

increase_max_num() {
  static limit = N_MAX;

  user.stat.n_max++;
  if (user.stat.n_max == limit) {
    alloc_max_block(limit);
    limit += N_MAX;
  }
}

is_it_closed() {
  register i, j, k, id;
  int *max_base;

  for (i = 0; i < user.stat.nstates; i++) {
    id = b_cover[i];

    for (j = 0; j < max[id]->class.many; j++) {
      int not_closed;

      not_closed = 1;
      for (k = 0; k < user.stat.nstates; k++) {
        if (contained(max[id]->class.imply[j], max[b_cover[k]])) {
          not_closed = 0;
          break;
        }
      }

      if (not_closed) {
        return 0;
      }
    }
  }
  return 1;
}
