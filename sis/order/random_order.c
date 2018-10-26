#include "sis.h"

static int *random_permutation();

array_t *order_random(array_t *roots, st_table *leaves, int dummy) {
    int          seed;
    int          counter;
    st_generator *gen;
    char         *key;
    int          value;
    int          *permutation;

    seed        = (int) leaves;
    permutation = random_permutation(seed, st_count(leaves));
    if (permutation == NIL(int))
        return NIL(array_t);
    counter = 0;
    st_foreach_item_int(leaves, gen, &key, &value) {
        if (!st_insert(leaves, (char *) key, (char *) permutation[counter++])) {
            fail("order_random: expecting value to be initialized to -1");
        }
    }
    FREE(permutation);
    return NIL(array_t);
}

/*
 * Computes a random permutation; returned as an array of integers from 0 to
 * n_elts-1.
 */
static int *random_permutation(int seed, int n_elts) {
    int i, j;
    int *permutation;
    int *remaining;
    int next_entry;
    int next_value;
    int n_entries;
    /* Declaration for random shouldn't be necessary (pick up from util.h). */
    /* Declaring it here breaks hpux. */
    /* long random(); */

    if (n_elts <= 0)
        return NIL(int);

    n_entries   = n_elts;
    permutation = ALLOC(int, n_entries);
    remaining   = ALLOC(int, n_entries);
    for (i      = 0; i < n_entries; i++) {
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
