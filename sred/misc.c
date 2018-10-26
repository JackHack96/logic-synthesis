
#include "espresso.h"

/* sf_insert_set -- add a set in position "i" of a set family */
pset_family sf_insert_set(A, i, s)
pset_family A;
int i;
pset s;
{
    register pset p;
	register int j, old_count;

    while (i >= A->capacity) {
	A->capacity = A->capacity + A->capacity/2 + 1;
	A->data = REALLOC(unsigned int, A->data, (long) A->capacity * A->wsize);
    }
	if (i >= A->count) {
	old_count = A->count;
	A->count = i + 1;
	for (j = old_count; j < i; j++) {
		set_clear (GETSET (A, j), A->sf_size);
	}
	}
    p = GETSET(A, i);
    INLINEset_copy(p, s);
    return A;
}

VOVoutput ()
{
}
