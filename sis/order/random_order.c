#include "sis.h"

/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/order/random_order.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:52 $
 * $Log: random_order.c,v $
 * Revision 1.1.1.1  2004/02/07 10:14:52  pchong
 * imported
 *
 * Revision 1.5  1993/02/25  01:02:46  sis
 * Shiple updates; 2/24/93.  See Shiple's RCS message.
 *
 * Revision 1.5  1993/02/25  01:02:46  sis
 * Shiple updates; 2/24/93.  See Shiple's RCS message.
 *
 * Revision 1.2  1993/01/11  23:28:20  shiple
 * Made change to call to st_foreach_item for DEC Alpha compatibility.
 *
 * Revision 1.1  1991/04/01  00:53:36  shiple
 * Initial revision
 *
 *
 */

static int *random_permutation();

array_t *order_random(roots, leaves, dummy)
array_t *roots;
st_table *leaves;
int dummy;  /* to give it the same interface as order_dfs */
{
    int seed;
    int counter;
    st_generator *gen;
    char *key;
    int value;
    int *permutation;

    seed = (int) leaves;
    permutation = random_permutation(seed, st_count(leaves));
    if (permutation == NIL(int)) return NIL(array_t);
    counter = 0;
    st_foreach_item_int(leaves, gen, &key, &value) {
      if (!st_insert(leaves, (char *)key, (char *)permutation[counter++])){
        fail ("order_random: expecting value to be initialized to -1");
      }
    }
    FREE(permutation);
    return NIL(array_t);
}

/* 
 * Computes a random permutation; returned as an array of integers from 0 to n_elts-1.
 */
static int *random_permutation(seed, n_elts)
int seed;
int n_elts;
{
    int i, j;
    int *permutation;
    int *remaining;
    int next_entry;
    int next_value;
    int n_entries;
    /* Declaration for random shouldn't be necessary (pick up from util.h). */
    /* Declaring it here breaks hpux. */
    /* extern long random(); */

    if (n_elts <= 0) return NIL(int);

    n_entries = n_elts;
    permutation = ALLOC(int, n_entries);
    remaining = ALLOC(int, n_entries);
    for (i = 0; i < n_entries; i++) {
	remaining[i] = i;
    }

    srandom(seed);

    next_entry = 0;
    for (; n_entries > 0; n_entries--) {
	next_value = random() % n_entries;
	permutation[next_entry++] = remaining[next_value];
	for (j = next_value; j < n_entries - 1; j++) {
	    remaining[j] = remaining[j + 1];
	}
    }
    FREE(remaining);
    return permutation;
}
