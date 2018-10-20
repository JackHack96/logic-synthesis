/**CFile****************************************************************

  FileName    [pdHash.c]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: pdHash.c,v 1.5 2003/05/28 04:08:11 alanmi Exp $]

***********************************************************************/
#include <stdio.h>
#include <limits.h>
#include "util.h"
#include "pdHash.h"

#define EQUAL(func, x, y) \
      ((*func)((x), (y)) == 0)


#define do_hash(key, table)\
      (*table->hash)((key), (table)->num_bins)

static int rehash();
int pdst_cubehash(), pdst_cubecmp();

pdst_table *
pdst_init_table_with_params(compare, hash, size, density, grow_factor,
			  reorder_flag)
int (*compare)();
int (*hash)();
int size;
int density;
double grow_factor;
int reorder_flag;
{
    int i;
    pdst_table *new;

    new = ALLOC(pdst_table, 1);
    if (new == NIL(pdst_table)) {
	return NIL(pdst_table);
    }
    new->compare = compare;
    new->hash = hash;
    new->num_entries = 0;
    new->max_density = density;
    new->grow_factor = grow_factor;
    new->reorder_flag = reorder_flag;
    if (size <= 0) {
	size = 1;
    }
    new->num_bins = size;
    new->bins = ALLOC(pdst_table_entry *, size);
    if (new->bins == NIL(pdst_table_entry *)) {
	FREE(new);
	return NIL(pdst_table);
    }
    for(i = 0; i < size; i++) {
	new->bins[i] = 0;
    }
    return new;
}

pdst_table *
pdst_init_table(compare, hash)
int (*compare)();
int (*hash)();
{
    return pdst_init_table_with_params(compare, hash, PDST_DEFAULT_INIT_TABLE_SIZE,
				     PDST_DEFAULT_MAX_DENSITY,
				     PDST_DEFAULT_GROW_FACTOR,
				     PDST_DEFAULT_REORDER_FLAG);
}
			    
void
pdst_free_table(table)
pdst_table *table;
{
    register pdst_table_entry *ptr, *next;
    int i;

    for(i = 0; i < table->num_bins ; i++) {
	ptr = table->bins[i];
	while (ptr != NIL(pdst_table_entry)) {
	    next = ptr->next;
	    FREE(ptr);
	    ptr = next;
	}
    }
    FREE(table->bins);
    FREE(table);
}

#define PTR_NOT_EQUAL(table, ptr, user_key) \
    (ptr != NIL(pdst_table_entry)) && !EQUAL(table->compare, user_key, (ptr)->key)

#define FIND_ENTRY(table, hash_val, key, ptr, last) \
    (last) = &(table)->bins[hash_val];\
    (ptr) = *(last);\
    while (PTR_NOT_EQUAL((table), (ptr), (key))) {\
	(last) = &(ptr)->next; (ptr) = *(last);\
    }\
    if ((ptr) != NIL(pdst_table_entry) && (table)->reorder_flag) {\
	*(last) = (ptr)->next;\
	(ptr)->next = (table)->bins[hash_val];\
	(table)->bins[hash_val] = (ptr);\
    }

int
pdst_lookup(table, key, value)
pdst_table *table;
register Mvc_Cube_t *key;
Mvc_Cube_t **value;
{
    int hash_val;
    register pdst_table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr, last);
    
    if (ptr == NIL(pdst_table_entry)) {
	return 0;
    } else {
	if (value != NIL(Mvc_Cube_t *)) {
	    *value = ptr->record; 
	}
	return 1;
    }
}

int
pdst_insert(table, key, value)
register pdst_table *table;
register Mvc_Cube_t *key;
Mvc_Cube_t *value;
{
    int hash_val;
    pdst_table_entry *new;
    register pdst_table_entry *ptr, **last;

    hash_val = do_hash(key, table);

    FIND_ENTRY(table, hash_val, key, ptr, last);

    if (ptr == NIL(pdst_table_entry)) {
	if (table->num_entries/table->num_bins >= table->max_density) {
	    if (rehash(table) == PDST_OUT_OF_MEM) {
		return PDST_OUT_OF_MEM;
	    }
	    hash_val = do_hash(key, table);
	}
	new = ALLOC(pdst_table_entry, 1);
	if (new == NIL(pdst_table_entry)) {
	    return PDST_OUT_OF_MEM;
	}
	new->key = key;
	new->record = value;
	new->next = table->bins[hash_val];
	table->bins[hash_val] = new;
	table->num_entries++;
	return 0;
    } else {
	ptr->record = value;
	return 1;
    }
}

static int
rehash(table)
register pdst_table *table;
{
    register pdst_table_entry *ptr, *next, **old_bins;
    int             i, old_num_bins, hash_val, old_num_entries;

    /* save old values */
    old_bins = table->bins;
    old_num_bins = table->num_bins;
    old_num_entries = table->num_entries;

    /* rehash */
    table->num_bins = table->grow_factor * old_num_bins;
    if (table->num_bins % 2 == 0) {
	table->num_bins += 1;
    }
    table->num_entries = 0;
    table->bins = ALLOC(pdst_table_entry *, table->num_bins);
    if (table->bins == NIL(pdst_table_entry *)) {
	table->bins = old_bins;
	table->num_bins = old_num_bins;
	table->num_entries = old_num_entries;
	return PDST_OUT_OF_MEM;
    }
    /* initialize */
    for (i = 0; i < table->num_bins; i++) {
	table->bins[i] = 0;
    }

    /* copy data over */
    for (i = 0; i < old_num_bins; i++) {
	ptr = old_bins[i];
	while (ptr != NIL(pdst_table_entry)) {
	    next = ptr->next;
	    hash_val = do_hash(ptr->key, table);
	    ptr->next = table->bins[hash_val];
	    table->bins[hash_val] = ptr;
	    table->num_entries++;
	    ptr = next;
	}
    }
    FREE(old_bins);

    return 1;
}

int 
pdst_cubehash(cube, size)
register Mvc_Cube_t *cube;
int size;
{
    int i; 
    unsigned long position;

    position = 0;
    i = cube->iLast;  
    for(; i >= 0; i--) 
        position = position ^ cube->pData[i];
    // DCDC: below does not work if position = INT_MIN, since -INT_MIN
    // does not exist. fix is simple: use ulong
    return position % size;
}

int
pdst_cubecmp(x, y)
Mvc_Cube_t *x;
Mvc_Cube_t *y;
{
    int i, j, k;

    i = x->iLast;
    j = y->iLast;
    if ( i > j ) 
        return 1;
    else if ( i < j )
        return -1;
    else
    {
        for (k=i; k>= 0; k--)
        {
            if ( x->pData[k] > y->pData[k] )
                return 1;
            else if ( x->pData[k] < y->pData[k] )
                return -1;
        }
        // only possible case
        return 0;
    }

}

pdst_generator *
pdst_init_gen(table)
pdst_table *table;
{
    pdst_generator *gen;

    gen = ALLOC(pdst_generator, 1);
    if (gen == NIL(pdst_generator)) {
	return NIL(pdst_generator);
    }
    gen->table = table;
    gen->entry = NIL(pdst_table_entry);
    gen->index = 0;
    return gen;
}


int 
pdst_gen(gen, key_p, value_p)
pdst_generator *gen;
Mvc_Cube_t **key_p;
Mvc_Cube_t **value_p;
{
    register int i;

    if (gen->entry == NIL(pdst_table_entry)) {
	/* try to find next entry */
	for(i = gen->index; i < gen->table->num_bins; i++) {
	    if (gen->table->bins[i] != NIL(pdst_table_entry)) {
		gen->index = i+1;
		gen->entry = gen->table->bins[i];
		break;
	    }
	}
	if (gen->entry == NIL(pdst_table_entry)) {
	    return 0;		/* that's all folks ! */
	}
    }
    *key_p = gen->entry->key;
    if (value_p != 0) {
	*value_p = gen->entry->record;
    }
    gen->entry = gen->entry->next;
    return 1;
}

void
pdst_free_gen(gen)
pdst_generator *gen;
{
    FREE(gen);
}
