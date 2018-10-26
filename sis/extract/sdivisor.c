
#include "sis.h"
#include "extract_int.h"
#include "heap.h"
#include "fast_extract_int.h"


/* Memory allocation and free of sdivisor_t */

static sdivisor_t *
sdivisor_alloc()
{
    register sdivisor_t *sdivisor;

    sdivisor = ALLOC(sdivisor_t, 1);
    sdivisor->col1 = sdivisor->col2 = -1;
    sdivisor->coin  = 0;

    return sdivisor;

}

static void
sdivisor_free(sdivisor)
char *sdivisor;
{
    sdivisor_t *sd = (sdivisor_t *) sdivisor;
 
    FREE(sd);
}

/* Memory allocation and free of sdset_t */

sdset_t *
sdset_alloc()
{
    register sdset_t *sdset;
   
    sdset = ALLOC(sdset_t, 1);
    sdset->index = -1;
    sdset->columns = lsCreate();
    sdset->col_set = lsCreate();
    sdset->heap = heap_alloc();
   
    return sdset;
}

void
sdset_free(sdset)
sdset_t *sdset;
{
    void sdivisor_free();

    lsDestroy(sdset->columns, free);
    lsDestroy(sdset->col_set, free);
    (void) heap_free(sdset->heap,sdivisor_free);
    FREE(sdset);
}

/* Memory allocation and free of col_cell_t */

static col_cell_t *
col_cell_alloc()
{
    register col_cell_t *col_cell;

    col_cell = ALLOC(col_cell_t, 1);
    col_cell->num = col_cell->length = 0;
    col_cell->handle = NULL;

    return col_cell;
}


static void
col_cell_free(col_cell)
col_cell_t *col_cell;
{
     FREE(col_cell);
}

/* compare function which will sort array elements in increasing order */

static int
compare(obj1,obj2)
lsGeneric obj1, obj2;
{
    register sm_col *col1 = (sm_col *)obj1;
    register sm_col *col2 = (sm_col *)obj2;

    return (col1->length - col2->length);
}


/* Find the unconsidered columns in the cube-literal matrix. */
static void
find_unconsidered_cols(B, sdset)
sm_matrix *B;
sdset_t *sdset;
{
    int index = sdset->index;
    lsGen gen;
    lsHandle handle;
    col_cell_t *col_cell;
    sm_col *pcol;
    char *data;
    int i, compare();
 
    /* If previous unconsidered cols set is not empty, then clean unavailable
     * cols, else if there is no new cols, then return.
     */

    if (lsLength(sdset->columns)){ 
        lsForeachItem(sdset->columns, gen, col_cell) {
            if (B->cols[col_cell->num] == 0) {
                lsRemoveItem(col_cell->handle, &data);
                FREE(col_cell);
            }
        }
    } else {
        if (index == B->last_col->col_num) return ;
    }

    if (index == B->last_col->col_num) {
        lsSort(sdset->columns, compare);
	return ;
    }

    /* Find the first column number not considered so far */

    if (index < 0) {
        pcol = B->first_col;
    } else {
        pcol =  sm_get_col(B,index);
        if (pcol != 0) {
            pcol = pcol->next_col;
        } else {
            for (i = index; (pcol = B->cols[i]) == 0; i++);
        }
    } 

    /* Append new columns to sdset->columns. */

    for (; pcol != 0; pcol = pcol->next_col) {
        col_cell = col_cell_alloc();
        col_cell->num = pcol->col_num;
        col_cell->length = pcol->length;
        lsNewEnd(sdset->columns, (lsGeneric) col_cell,&handle);  
        col_cell->handle = handle;
    }
    sdset->index = B->last_col->col_num;
    lsSort(sdset->columns, compare);
}


/* Extract the columns which has coincidence k. */
static array_t *
cols_extract(columns, k)
lsList columns;
int k;
{
    int flag = 1;
    array_t *set;
    col_cell_t *col_cell;
    lsHandle handle;
    char *data;

    set = array_alloc(col_cell_t *, 0);

    do {
       lsLastItem(columns, (lsGeneric *) &col_cell, &handle);
       if (col_cell) {
           if (col_cell->length == k) {
               (void) array_insert_last(col_cell_t *, set, col_cell);
               lsRemoveItem(handle,&data); 
           } else {
              flag = 0;
           }
       } else {
           flag = 0;
       }
    } while (flag);
   
    return set;
}


/* Find all possible pairs in set, and compute coincidence, then store in
 * the sdset->heap.
 */
static void
coin1_compute(B, set, sdset)
sm_matrix *B;
array_t *set;
sdset_t *sdset;
{
    int i, j;
    sdivisor_t *sd;
    sm_col *col, *col1, *col2;
    heap_entry_t *entry;
    col_cell_t *col_cell1, *col_cell2;
    
    if (array_n(set) == 1) return ;
    
    for (i=0; i < array_n(set)-1; i++) {
        col_cell1 = array_fetch(col_cell_t *, set, i);
        col1 = sm_get_col(B, col_cell1->num);
        for (j=i+1; j < array_n(set); j++) {
            sd = sdivisor_alloc();
            sd->col1 = col_cell1->num;
            col_cell2 = array_fetch(col_cell_t *, set, j);
            col2 = sm_get_col(B, col_cell2->num);
            sd->col2 = col_cell2->num;
            col = sm_col_and(col1, col2);
            if ((sd->coin = col->length) > 2) {
                entry = heap_entry_alloc();
                entry->key = sd->coin;
                entry->item = (char *) sd;
                (void) insert_heap(sdset->heap, entry);
            } else {
                (void) sdivisor_free((char *)sd);
            }
            (void) sm_col_free(col);
        }
    }
}


/* Find all possible pairs of set and sdset->col_set, compute coincidence,
 * then store in sdset->heap.
 */
static void 
coin2_compute(B, set, sdset)
sm_matrix *B;
array_t *set;
sdset_t *sdset;
{
    int i;
    sdivisor_t *sd;
    sm_col *col, *col1, *col2;
    col_cell_t *col_cell1, *col_cell2;
    heap_entry_t *entry;
    lsGen gen;
    char *data;

    if ((array_n(set)== 0) || (lsLength(sdset->col_set) == 0)) return ;

    for (i = 0; i < array_n(set); i++) {
        col_cell1 = array_fetch(col_cell_t *, set, i);
        col1 = sm_get_col(B, col_cell1->num);
        lsForeachItem(sdset->col_set, gen, col_cell2) {
            col2 = sm_get_col(B, col_cell2->num);
            if (col2) {
                sd = sdivisor_alloc();
                sd->col1 = col_cell1->num;
                sd->col2 = col_cell2->num;
                col = sm_col_and(col1, col2);
                if ((sd->coin = col->length) > 2) {
                    entry = heap_entry_alloc();
                    entry->key = sd->coin;
                    entry->item = (char *) sd;
                    (void) insert_heap(sdset->heap, entry);
                } else {
		    (void) sdivisor_free((char *) sd); 
                }
                (void) sm_col_free(col);
            } else {
                lsRemoveItem(col_cell2->handle, &data);
                (void) col_cell_free((col_cell_t *) data);
            }
        }
    }
}

/* Append T into sdset->col_set. */

static void
sdset_append(sdset, T)
sdset_t *sdset;
array_t *T;
{
    col_cell_t *col_cell;
    lsHandle handle;
    int i;

    for (i = 0; i < array_n(T); i++) {
        col_cell = array_fetch(col_cell_t *, T, i);
        lsNewEnd(sdset->col_set, (lsGeneric) col_cell, &handle); 
        col_cell->handle = handle;
    }
}

/* Extracting single-cube divisor with exact two literals.
 * B : cube_literal_matrix.
 * sdset : single-cube divisor set.
 * wdmax : the current maximum weight of double-cube divisor.
 */
static sdivisor_t *
find_sdivisor(B,sdset,wdmax)
sm_matrix *B;
sdset_t *sdset;
int wdmax;
{
    lsHandle handle;
    sm_col *col;
    heap_entry_t *maxheap, *entry;
    sdivisor_t *candidate;
    col_cell_t *col_cell;
    array_t *T;
    int k = 0;
    int threshold = 0;
    int HEAP_CHECK, NOITERATION;
    sm_col *col1, *col2;


    do { /* Iteration */
        /* k = maximum number of literals found in any column not considered
         *     so far.
         */
        NOITERATION = 0;
        if (lsLength(sdset->columns)) {
            lsLastItem(sdset->columns, (lsGeneric *) &col_cell, &handle); 
            k = col_cell->length;
            if ((k-2) > wdmax) {
                T = cols_extract(sdset->columns, k);
                (void) coin1_compute(B, T, sdset);
                (void) coin2_compute(B, T, sdset); 
                (void) sdset_append(sdset, T);
                (void) array_free(T);
            } else {
                NOITERATION = 1;
                /* Look the top of heap, if weight is less than wdmax,
                 * then there is no better single-cube divisor. 
                 */
                maxheap = findmax_heap(sdset->heap);
                if (maxheap == NIL(heap_entry_t)) {
                    return NIL(sdivisor_t);
                } else {
                    if ((maxheap->key - 2) <= wdmax) {
                        return NIL(sdivisor_t);
                    }
                }
            }
        } else {
            NOITERATION = 1; k = 0;
        }

        /* threshold = (maxheap) */

        HEAP_CHECK = 1;

	/* Find best single-cube divisor, and update top entry if it's not
	 * maximum.
	 */

        do { /* Heap update */
            maxheap = deletemax_heap(sdset->heap);
            if (maxheap == NIL(heap_entry_t)) {
                if (NOITERATION) {
                    return NIL(sdivisor_t);
                } else {
                    threshold = HEAP_CHECK = 0;
                }
            } else {
		/* Get the top entry in the heap, and see if it has the
		 * same weight as previous weight or not. If yes, then
		 * choose it as divisor, else update and push it back into
		 * heap.
		 */
                candidate = (sdivisor_t *)maxheap->item;
                (void) heap_entry_free(maxheap);
                col1 = sm_get_col(B, candidate->col1);
                col2 = sm_get_col(B, candidate->col2);
                if ((col1) && (col2)) {
                    col = sm_col_and(col1, col2);
		    /* If candidate has the same weight as previous weight,
		     * it must imply that the top entry has max weight. 
		     */
                    if (col->length >= candidate->coin) {
                        threshold = candidate->coin = col->length; 
                        HEAP_CHECK = 0;
                        (void) sm_col_free(col);

                        if ((threshold >= k) && ((threshold - 2) > wdmax)) {
                            return candidate;
                        } else {
                            if (threshold <= 2) {
                                (void) sdivisor_free((char *) candidate);
                            } else {
                                entry = heap_entry_alloc();
                                entry->key = candidate->coin;
                                entry->item = (char *) candidate;
                                (void) insert_heap(sdset->heap, entry);
                            }
                        }
                    } else {
			/* If candidate still have the possibility to be
			 * divisor, then push it back, else throw it away.
			 */
                        if (col->length > 2) {
                            entry = heap_entry_alloc();
                            candidate->coin = entry->key = col->length;
                            entry->item = (char *) candidate;
                            (void) insert_heap(sdset->heap, entry); 
                        } else {
			    (void) sdivisor_free((char *) candidate); 
                        }
                        (void) sm_col_free(col);
                    }
                } else {
                    HEAP_CHECK = 1;
                    (void) sdivisor_free((char *)candidate);
                }
            }
        } while (HEAP_CHECK);
    } while (NOITERATION == 0);
     
    return NIL(sdivisor_t);
}


sdivisor_t *
extract_sdivisor(B, sdset, wdmax)
sm_matrix *B;
sdset_t *sdset;
int wdmax;
{
    if (B->nrows == 0) return NIL(sdivisor_t);

    (void) find_unconsidered_cols(B, sdset);
    return find_sdivisor(B,sdset,wdmax);
}
