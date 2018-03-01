/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/st/st.h,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2005/03/08 03:50:52 $
 *
 */
/* LINTLIBRARY */

/* $Header: /users/pchong/CVS/sis/sis/st/st.h,v 1.2 2005/03/08 03:50:52 pchong Exp $ */

#ifndef ST_INCLUDED
#define ST_INCLUDED

typedef struct st_table_entry st_table_entry;
struct st_table_entry {
    char *key;
    char *record;
    st_table_entry *next;
};

typedef struct st_table st_table;
struct st_table {
    int (*compare)();
    int (*hash)();
    int num_bins;
    int num_entries;
    int max_density;
    int reorder_flag;
    double grow_factor;
    st_table_entry **bins;
};

typedef struct st_generator st_generator;
struct st_generator {
    st_table *table;
    st_table_entry *entry;
    int index;
};

#define st_is_member(table,key) st_lookup(table,key,(char **) 0)
#define st_count(table) ((table)->num_entries)

enum st_retval {ST_CONTINUE, ST_STOP, ST_DELETE};

typedef enum st_retval (*ST_PFSR)();
typedef int (*ST_PFI)();
typedef int (*ST_PFICPI)();

EXTERN int st_delete ARGS((st_table *, char **, char **));
EXTERN int st_delete_int ARGS((st_table *, int *, char **));
EXTERN int st_insert ARGS((st_table *, char *, char *));
EXTERN int st_foreach ARGS((st_table *, ST_PFSR, char *));
EXTERN int st_gen ARGS((st_generator *, char **, char **));
EXTERN int st_gen_int ARGS((st_generator *, char **, int *));
EXTERN int st_lookup ARGS((st_table *, char *, char **));
EXTERN int st_lookup_int ARGS((st_table *, char *, int *));
EXTERN int st_find_or_add ARGS((st_table *, char *, char ***));
EXTERN int st_find ARGS((st_table *, char *, char ***));
EXTERN int st_add_direct ARGS((st_table *, char *, char *));
EXTERN int st_strhash ARGS((char *, int));
EXTERN int st_numhash ARGS((char *, int));
EXTERN int st_ptrhash ARGS((char *, int));
EXTERN int st_numcmp ARGS((char *, char *));
EXTERN int st_ptrcmp ARGS((char *, char *));
EXTERN st_table *st_init_table ARGS((ST_PFI, ST_PFI)); 
EXTERN st_table *st_init_table_with_params ARGS((ST_PFI, ST_PFI, int, int, double, int));
EXTERN st_table *st_copy ARGS((st_table *));
EXTERN st_generator *st_init_gen ARGS((st_table *));
EXTERN void st_free_table ARGS((st_table *));
EXTERN void st_free_gen ARGS((st_generator *));


#define ST_DEFAULT_MAX_DENSITY 5
#define ST_DEFAULT_INIT_TABLE_SIZE 11
#define ST_DEFAULT_GROW_FACTOR 2.0
#define ST_DEFAULT_REORDER_FLAG 0

#define st_foreach_item(table, gen, key, value) \
    for(gen=st_init_gen(table); st_gen(gen,key,value) || (st_free_gen(gen),0);)

#define st_foreach_item_int(table, gen, key, value) \
    for(gen=st_init_gen(table); st_gen_int(gen,key,value) || (st_free_gen(gen),0);)

#endif /* ST_INCLUDED */
