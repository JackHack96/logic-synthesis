/**CFile****************************************************************

  FileName    [simpArray.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    [Two-level node minimization using don't cares.]

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: simpArray.c,v 1.7 2003/05/27 23:16:11 alanmi Exp $]

***********************************************************************/


#include "util.h"
#include "string.h"
#include "simpArray.h"

#define INIT_SIZE	3


sarray_t *
sarray_do_alloc(int number)
{
    sarray_t *pA;

    pA = ALLOC(sarray_t, 1);
    pA->num   = 0;
    pA->size  = MAX(number, INIT_SIZE);
    pA->space = ALLOC(void *, pA->size);
    (void) memset(pA->space, 0, pA->size * sizeof(void *));
    return pA;
}


void
sarray_free(sarray_t *pA)
{
    if (pA == NIL(sarray_t)) return;
    FREE(pA->space);
    FREE(pA);
}


sarray_t *
sarray_dup(sarray_t *pOld)
{
    sarray_t *pNew;

    pNew = ALLOC(sarray_t, 1);
    if (pNew == NIL(sarray_t)) {
        return NIL(sarray_t);
    }
    pNew->num  = pOld->num;
    pNew->size = pOld->num;
    pNew->space = ALLOC(void *, pNew->size);
    if (pNew->space == NIL(void *)) {
        FREE(pNew);
        return NIL(sarray_t);
    }
    (void) memcpy(pNew->space, pOld->space, pOld->num * sizeof(void *));
    return pNew;
}


int
sarray_append(sarray_t *pA1, sarray_t *pA2)
{
    void **pos;

    /* make sure pA1 has enough room */
    if (pA1->size < pA1->num + pA2->num) {
	if (sarray_resize(pA1, pA1->num + pA2->num) == SARRAY_OUT_OF_MEM) {
	    return SARRAY_OUT_OF_MEM;
	}
    }
    pos = pA1->space + pA1->num;
    (void) memcpy(pos, pA2->space, pA2->num * sizeof(void *));
    pA1->num += pA2->num;

    return 1;
}


sarray_t *
sarray_join(sarray_t *pA1, sarray_t *pA2)
{
    sarray_t *pA;
    void **pos;

    pA = ALLOC(sarray_t, 1);
    if (pA == NIL(sarray_t)) {
        return NIL(sarray_t);
    }
    pA->num   = pA1->num + pA2->num;
    pA->size  = pA->num;
    pA->space = ALLOC(void *, pA->size * sizeof(void *));
    if (pA->space == NIL(void *)) {
        FREE(pA);
        return NIL(sarray_t);
    }
    (void) memcpy(pA->space, pA1->space, pA1->num * sizeof(void *));
    pos = pA->space + pA1->num;
    (void) memcpy(pos, pA2->space, pA2->num * sizeof(void *));
    return pA;
}


int
sarray_resize(sarray_t *pA, int new_size)
{
    int old_size;
    void **pos, **new_space;
    
    /* Note that this is not an exported function, and does not check if
       the pA is locked since that is already done by the caller. */
    old_size  = pA->size;
    pA->size  = MAX( old_size * 2, new_size );
    new_space = REALLOC(void *, pA->space, pA->size);
    if (new_space == NIL(void *)) {
        return SARRAY_OUT_OF_MEM;
    }
    pA->space = new_space;
    pos = (pA->space + old_size);
    (void) memset(pos, 0, (pA->size - old_size)*sizeof(void *));
    return 1;
}


void
sarray_sort(sarray_t *pA, int (*compare)(const void *, const void *))
{
    qsort((void *)pA->space, pA->num, sizeof(void *), compare);
}

/* remove NULL entries; reset num to the number of non-NULL entries */
void
sarray_compact (sarray_t *pA)
{
    int i, nCount;
    nCount = 0;
    for ( i=0; i<pA->num; ++i ) {
        if ( pA->space[i] ) {
            if ( i!=nCount ) {
                pA->space[nCount] = pA->space[i];
            }
            nCount ++;
        }
    }
    pA->num = nCount;
}
