
/* file @(#)comb.c	1.1                      */
/* last modified on 5/29/91 at 12:35:19   */
#include "genlib_int.h"

combination_t *
gl_init_gen_combination(value, n)
int *value;
int n;
{
    int i;
    combination_t *comb;

    comb = ALLOC(combination_t, 1);
    comb->n = n;
    comb->value = ALLOC(int, n);
    comb->state = ALLOC(int, n);
    for(i = 0; i < n; i++) {
	comb->state[i] = 0;
	comb->value[i] = value[i];
    }
    comb->state[0] = -1;
    return comb;
}


void 
gl_free_gen_combination(comb)
combination_t *comb;
{
    FREE(comb->value);
    FREE(comb->state);
    FREE(comb);
}


int
gl_next_combination(comb, vector)
combination_t *comb;
int **vector;
{
    int k;

    for(k = 0; k < comb->n; k++) {
	/* advance position k */
	comb->state[k]++;

	/* Check for overflow in position k */
	if (comb->state[k] >= comb->value[k]) {
	    comb->state[k] = 0;
	} else {
	    *vector = comb->state;
	    return 1;
	}
    }
    return 0;	/* no more possibilities */
}


int
gl_next_nondecreasing_combination(comb, vector)
combination_t *comb;
int **vector;
{
    int k, nondecreasing;

    do {
	if (! gl_next_combination(comb, vector)) {
	    return 0;
	}

	nondecreasing = 1;
	for(k = 1; k < comb->n; k++) {
	    if (comb->state[k-1] > comb->state[k]) {
		nondecreasing = 0;
		break;
	    }
	}
    } while (! nondecreasing);
    return 1;
}


int
gl_next_nonincreasing_combination(comb, vector)
combination_t *comb;
int **vector;
{
    int k, nonincreasing;

    do {
	if (! gl_next_combination(comb, vector)) {
	    return 0;
	}

	nonincreasing = 1;
	for(k = 1; k < comb->n; k++) {
	    if (comb->state[k-1] < comb->state[k]) {
		nonincreasing = 0;
		break;
	    }
	}
    } while (! nonincreasing);
    return 1;
}

partition_t *
gl_init_gen_partition(s)
int s;
{
    int i;
    partition_t *part;

    part = ALLOC(partition_t, 1);
    part->nvalue = s;
    part->value = ALLOC(int, part->nvalue);
    for(i = 0; i < part->nvalue; i++) {
	part->value[i] = 0;
    }
    part->value[0] = 0;
    if (s > 1) part->value[1] = 1;
    part->non_zero = 1;
    part->sum = 1;
    part->maxsum = s;
    return part;
}


void 
gl_free_gen_partition(part)
partition_t *part;
{
    FREE(part->value);
    FREE(part);
}


int 
gl_next_partition_less_than(part, value, n)
partition_t *part;
int **value;		/* array of values in partition */
int *n;			/* number of values */
{
    int k, l, new_value;

    for(k = 0; k < part->nvalue; k++) {
	/* advance position k */
	if (part->value[k] == 0) part->non_zero++;
	part->value[k]++;
	part->sum++;

	/* check that the sum is <= s */
	if (part->sum <= part->maxsum) {
	    *n = part->non_zero;
	    *value = part->value;
	    return 1;

	} else {
	    if (k == part->nvalue - 1) return 0;

	    /* get set to advance the next counter */
	    for(l = 0; l <= k; l++) {
		new_value = part->value[k+1] + 1;
		part->sum = part->sum - part->value[l] + new_value;
		part->value[l] = new_value;
	    }
	}
    }
    return 0;		/* no more partitions */
}


int 
gl_next_partition(part, value, n)
partition_t *part;
int **value;
int *n;
{
    while (gl_next_partition_less_than(part, value, n)) {
	if (part->sum == part->maxsum) {		/* cheat */
	    return 1;
	}
    }
    return 0;
}

#ifdef TEST
#include <stdio.h>

void exit();

void
usage(s)
char *s;
{
    (void) fprintf(stderr, "usage: %s n l1 l2 ... ln\n", s);
    exit(2);
}


main(argc, argv)
int argc;
char **argv;
{
    int i, n, array[40], *vector;
    combination_t *comb;

    if (argc < 2) usage(argv[0]);
    n = atoi(argv[1]);
    if (argc < n+2) usage(argv[0]);

    for(i = 0; i < n; i++) {
	array[i] = atoi(argv[i+2]);
    }

    (void) printf("Combinations are\n");
    comb = gl_init_gen_combination(array, n);
    while (gl_next_combination(comb, &vector)) {
	for(i = 0; i < n; i++) {
	    (void) printf(" %d", vector[i]);
	}
	(void) printf("\n");
    }
    gl_free_gen_combination(comb);

    (void) printf("Nondecreasing combinations are\n");
    comb = gl_init_gen_combination(array, n);
    while (gl_next_nondecreasing_combination(comb, &vector)) {
	for(i = 0; i < n; i++) {
	    (void) printf(" %d", vector[i]);
	}
	(void) printf("\n");
    }
    gl_free_gen_combination(comb);
}
#endif
