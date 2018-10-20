/**CFile****************************************************************

  FileName    [pdHase.h]

  PackageName [MVSIS 2.0: Multi-valued logic synthesis system.]

  Synopsis    []

  Author      [MVSIS Group]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 1, 2003.]

  Revision    [$Id: pdHash.h,v 1.1 2003/05/28 04:08:11 alanmi Exp $]

***********************************************************************/


#ifndef PDST_INCLUDED
#define PDST_INCLUDED

#include "mvc.h"

typedef struct pdst_table_entry pdst_table_entry;
struct pdst_table_entry {
    Mvc_Cube_t *key;
    Mvc_Cube_t *record;              
    pdst_table_entry *next;
};

typedef struct pdst_table pdst_table;
struct pdst_table {
    int (*compare)();
    int (*hash)();
    int num_bins;
    int num_entries;
    int max_density;
    int reorder_flag;
    double grow_factor;
    pdst_table_entry **bins;
};

typedef struct pdst_generator pdst_generator;
struct pdst_generator {
    pdst_table *table;
    pdst_table_entry *entry;
    int index;
};

#define pdst_is_member(table,key) pdst_lookup(table,key,(Mvc_Cube_t **) 0)
#define pdst_count(table) ((table)->num_entries)

typedef int (*PDST_PFI)();

EXTERN pdst_table *pdst_init_table_with_params ARGS((PDST_PFI, PDST_PFI, int, int, double, int));
EXTERN pdst_table *pdst_init_table ARGS((PDST_PFI, PDST_PFI)); 
EXTERN void pdst_free_table ARGS((pdst_table *));
EXTERN int pdst_lookup ARGS((pdst_table *, Mvc_Cube_t *, Mvc_Cube_t **));
EXTERN int pdst_insert ARGS((pdst_table *, Mvc_Cube_t *, Mvc_Cube_t *));
EXTERN int pdst_delete ARGS((pdst_table *, Mvc_Cube_t **, Mvc_Cube_t **));
EXTERN int pdst_cubehash ARGS((Mvc_Cube_t *, int));
EXTERN int pdst_cubecmp ARGS((Mvc_Cube_t *, Mvc_Cube_t *));
EXTERN pdst_generator *pdst_init_gen ARGS((pdst_table *));
EXTERN int pdst_gen ARGS((pdst_generator *, Mvc_Cube_t **, Mvc_Cube_t **));
EXTERN void pdst_free_gen ARGS((pdst_generator *));


#define PDST_DEFAULT_MAX_DENSITY 5
#define PDST_DEFAULT_INIT_TABLE_SIZE 11
#define PDST_DEFAULT_GROW_FACTOR 2.0
#define PDST_DEFAULT_REORDER_FLAG 0

#define pdst_foreach_item(table, gen, key, value) \
    for(gen=pdst_init_gen(table); pdst_gen(gen,key,value) || (pdst_free_gen(gen),0);)

#define PDST_OUT_OF_MEM -10000

#endif /* ST_INCLUDED */
