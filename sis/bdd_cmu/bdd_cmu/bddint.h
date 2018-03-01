/* BDD package internal definitions */


#if !defined(_BDDINTH)
#define _BDDINTH


#include <setjmp.h>


/* >>> Configuration things */

/* Define this for cache and unique sizes to be a power of 2. */
/* Probably not a good idea given the current simplistic hash function. */

/* #define POWER_OF_2_SIZES */

/* Define this if your C preprocessor is broken in such a way that */
/* token concatenation can be done with something like: */
/* #define CONCAT(f, g) f/''/g */
/* where /''/ above would be an empty comment (' => *) if this */
/* wasn't itself a comment.  Most of the (non-ANSI) UNIX C preprocessors */
/* do seem to be broken, which is why this is defined.  If your C */
/* preprocessor is ANSI, this shouldn't have any effect. */

#define BROKEN_CPP



/* >>> All user-visible stuff */

#include "bdduser.h"


#if defined(__STDC__)
#define ARGS(args) args
#else
#define ARGS(args) ()
#endif


/* Miscellaneous type definitions */

typedef struct hash_table_ *hash_table;


/* >>> BDD data structures */

#if !defined(POWER_OF_2_SIZES)
extern long bdd_primes[];
#endif


/* Pointer tagging stuff */

#define POINTER(p) ((pointer)(((INT_PTR)(p)) & ~(INT_PTR)0x7))
#define BDD_POINTER(p) ((bdd)POINTER(p))
#define CACHE_POINTER(p) ((cache_entry)POINTER(p))

#define TAG(p) ((int)(((INT_PTR)(p)) & 0x7))
#define SET_TAG(p, t) ((pointer)((INT_PTR)POINTER(p) | (t)))

#define TAG0(p) ((int)((INT_PTR)(p) & 0x1))
#define FLIP_TAG0(p) ((pointer)((INT_PTR)(p) ^ 0x1))
#define TAG0_HI(p) ((pointer)((INT_PTR)(p) | 0x1))
#define TAG0_LO(p) ((pointer)((INT_PTR)(p) & ~(INT_PTR)0x1))

#define TAG1(p) ((int)((INT_PTR)(p) & 0x2))
#define FLIP_TAG1(p) ((pointer)((INT_PTR)(p) ^ 0x2))
#define TAG1_HI(p) ((pointer)((INT_PTR)(p) | 0x2))
#define TAG1_LO(p) ((pointer)((INT_PTR)(p) & ~(INT_PTR)0x2))

#define TAG2(p) ((int)((INT_PTR)(p) & 0x4))
#define FLIP_TAG2(p) ((pointer)((INT_PTR)(p) ^ 0x4))
#define TAG2_HI(p) ((pointer)((INT_PTR)(p) | 0x4))
#define TAG2_LO(p) ((pointer)((INT_PTR)(p) & ~(INT_PTR)0x4))


/* Indexes */

typedef unsigned short bdd_index_type;


/* Types of various BDD node fields */

typedef unsigned short bdd_indexindex_type;
typedef unsigned char bdd_refcount_type;
typedef unsigned char bdd_mark_type;


/* A BDD node */

struct bdd_
{
  bdd_indexindex_type indexindex;
				/* Index into indexes table */
  bdd_refcount_type refs;	/* External reference count */
  bdd_mark_type mark;		/* Mark and temporary ref count */
  INT_PTR data[2];		/* Then and else pointers, or data */
				/* values for terminals */
  struct bdd_ *next;
};


/* Maximum reference counts */

#define BDD_MAX_REFS ((bdd_refcount_type)((1l << (8*sizeof(bdd_refcount_type))) - 1))
#define BDD_MAX_TEMP_REFS ((bdd_mark_type)((1l << (8*sizeof(bdd_mark_type)-1)) - 1))

#define BDD_GC_MARK ((bdd_mark_type)(1l << (8*sizeof(bdd_mark_type)-1)))


/* Special indexindexes and indexes */

#define BDD_CONST_INDEXINDEX 0
#define BDD_MAX_INDEXINDEX ((bdd_indexindex_type)((1l << (8*sizeof(bdd_indexindex_type))) - 1))
#define BDD_MAX_INDEX ((bdd_index_type)((1l << (8*sizeof(bdd_index_type))) - 1))


/* Can this be done legally with a non-ANSI cpp? */

#if defined(__STDC__)
#define CONCAT_PTR(f) f##_ptr
#elif defined(BROKEN_CPP)
#define CONCAT_PTR(f) f/**/_ptr
#endif

#if defined(CONCAT_PTR)
/* Field accessing stuff */

#define BDD_SETUP(f) bdd CONCAT_PTR(f)=BDD_POINTER(f)
#define BDD_RESET(f) CONCAT_PTR(f)=BDD_POINTER(f)
#define BDD_INDEXINDEX(f) (CONCAT_PTR(f)->indexindex)
#define BDD_DATA(f) (CONCAT_PTR(f)->data)
#define BDD_DATA0(f) (CONCAT_PTR(f)->data[0])
#define BDD_DATA1(f) (CONCAT_PTR(f)->data[1])
#define BDD_REFS(f) (CONCAT_PTR(f)->refs)
#define BDD_MARK(f) (CONCAT_PTR(f)->mark)
#define BDD_TEMP_REFS(f) (CONCAT_PTR(f)->mark)


/* Basic stuff stuff for indexes, testing, etc. */

#define BDD_INDEX(bddm, f) ((bddm)->indexes[BDD_INDEXINDEX(f)])
#define BDD_THEN(f) ((bdd)(BDD_DATA0(f) ^ TAG0(f)))
#define BDD_ELSE(f) ((bdd)(BDD_DATA1(f) ^ TAG0(f)))
#define BDD_SAME_OR_NEGATIONS(f, g) (CONCAT_PTR(f) == CONCAT_PTR(g))
#define BDD_IS_CONST(f) (BDD_INDEXINDEX(f) == BDD_CONST_INDEXINDEX)
#else
/* Field accessing stuff */

#define BDD_SETUP(f)
#define BDD_RESET(f)
#define BDD_INDEXINDEX(f) (BDD_POINTER(f)->indexindex)
#define BDD_DATA(f) (BDD_POINTER(f)->data)
#define BDD_DATA0(f) (BDD_POINTER(f)->data[0])
#define BDD_DATA1(f) (BDD_POINTER(f)->data[1])
#define BDD_REFS(f) (BDD_POINTER(f)->refs)
#define BDD_MARK(f) (BDD_POINTER(f)->mark)
#define BDD_TEMP_REFS(f) (BDD_POINTER(f)->mark)


/* Basic stuff stuff for indexes, testing, etc. */

#define BDD_INDEX(bddm, f) ((bddm)->indexes[BDD_INDEXINDEX(f)])
#define BDD_THEN(f) ((bdd)(BDD_DATA0(f) ^ TAG0(f)))
#define BDD_ELSE(f) ((bdd)(BDD_DATA1(f) ^ TAG0(f)))
#define BDD_SAME_OR_NEGATIONS(f, g) (BDD_POINTER(f) == BDD_POINTER(g))
#define BDD_IS_CONST(f) (BDD_INDEXINDEX(f) == BDD_CONST_INDEXINDEX)
#endif


/* BDD complement flag stuff */

#define BDD_IS_OUTPOS(f) (!TAG0(f))
#define BDD_OUTPOS(f) ((bdd)TAG0_LO(f))
#define BDD_NOT(f) ((bdd)FLIP_TAG0(f))


/* Cofactoring stuff */

#define BDD_TOP_VAR2(top_indexindex, bddm, f, g)\
do\
  if (BDD_INDEX(bddm, f) < BDD_INDEX(bddm, g))\
    top_indexindex=BDD_INDEXINDEX(f);\
  else\
    top_indexindex=BDD_INDEXINDEX(g);\
while (0)

#define BDD_TOP_VAR3(top_indexindex, bddm, f, g, h)\
do\
  if (BDD_INDEX(bddm, f) < BDD_INDEX(bddm, g))\
    {\
      top_indexindex=BDD_INDEXINDEX(f);\
      if ((bddm)->indexes[top_indexindex] > BDD_INDEX(bddm, h))\
	top_indexindex=BDD_INDEXINDEX(h);\
    }\
  else\
    {\
      top_indexindex=BDD_INDEXINDEX(g);\
      if ((bddm)->indexes[top_indexindex] > BDD_INDEX(bddm, h))\
	top_indexindex=BDD_INDEXINDEX(h);\
    }\
while (0)

#define BDD_COFACTOR(top_indexindex, f, f_then, f_else)\
do\
  if (BDD_INDEXINDEX(f) == top_indexindex)\
    {\
      f_then=BDD_THEN(f);\
      f_else=BDD_ELSE(f);\
    }\
  else\
    {\
      f_then=f;\
      f_else=f;\
    }\
while (0)


/* Ordering stuff */

#define BDD_OUT_OF_ORDER(f, g) ((INT_PTR)f > (INT_PTR)g)

#if defined(CONCAT_PTR)
#define BDD_SWAP(f, g)\
do\
  {\
    bdd _temp;\
    _temp=f;\
    f=g;\
    g=_temp;\
    _temp=CONCAT_PTR(f);\
    CONCAT_PTR(f)=CONCAT_PTR(g);\
    CONCAT_PTR(g)=_temp;\
  }\
while (0)
#else
#define BDD_SWAP(f, g)\
do\
  {\
    bdd _temp;\
    _temp=f;\
    f=g;\
    g=_temp;\
  }\
while (0)
#endif


/* This gets the variable at the top of a node. */

#define BDD_IF(bddm, f) ((bddm)->variables[BDD_INDEXINDEX(f)])


/* Reference count stuff */

#define BDD_INCREFS(f)\
do\
  {\
    if (BDD_REFS(f) >= BDD_MAX_REFS-1)\
      {\
	BDD_REFS(f)=BDD_MAX_REFS;\
	BDD_TEMP_REFS(f)=0;\
      }\
   else\
     BDD_REFS(f)++;\
  }\
while (0)

#define BDD_DECREFS(f)\
do\
  {\
    if (BDD_REFS(f) < BDD_MAX_REFS)\
      BDD_REFS(f)--;\
  }\
while (0)

#define BDD_TEMP_INCREFS(f)\
do\
  {\
    if (BDD_REFS(f) < BDD_MAX_REFS)\
      {\
	BDD_TEMP_REFS(f)++;\
	if (BDD_TEMP_REFS(f) == BDD_MAX_TEMP_REFS)\
	  {\
	    BDD_REFS(f)=BDD_MAX_REFS;\
	    BDD_TEMP_REFS(f)=0;\
	  }\
      }\
  }\
while (0)

#define BDD_TEMP_DECREFS(f)\
do\
  {\
    if (BDD_REFS(f) < BDD_MAX_REFS)\
      BDD_TEMP_REFS(f)--;\
  }\
while (0)

#define BDD_IS_USED(f) ((BDD_MARK(f) & BDD_GC_MARK) != 0)


/* Convert an internal reference to an external one and return. */

#define RETURN_BDD(thing)\
return (bdd_make_external(thing))


/* These return the constants. */

#define BDD_ONE(bddm) ((bddm)->one)
#define BDD_ZERO(bddm) ((bddm)->zero)


/* Cache entries */

struct cache_entry_
{
  INT_PTR slot[4];
};

typedef struct cache_entry_ *cache_entry;


/* Cache entry tags; ITE is for IF-THEN-ELSE operations.  TWO is for */
/* the two argument operations that return a BDD result.  ONEDATA */
/* and TWODATA are for one and two argument operations that return */
/* double and single word data results.  Everything else is under */
/* user control. */

#define CACHE_TYPE_ITE 0x0
#define CACHE_TYPE_TWO 0x1
#define CACHE_TYPE_ONEDATA 0x2
#define CACHE_TYPE_TWODATA 0x3
#define CACHE_TYPE_USER1 0x4

#define USER_ENTRY_TYPES 4


/* Operation numbers (for CACHE_TYPE_TWO) */
/* OP_RELPROD, OP_QNT and OP_SUBST need a reasonable number of holes */
/* after them since the variable association number is added to them */
/* to get the actual op.  OP_COMP, OP_SWAP and OP_CMPTO need */
/* BDD_MAX_INDEX.  OP_SWAPAUX needs 2*BDD_MAX_INDEX.  Negative */
/* operation numbers denote temporaries and are generated as needed. */

#define OP_COFACTOR 100l
#define OP_SATFRAC 200l
#define OP_FWD 300l
#define OP_REV 400l
#define OP_RED 500l
#define OP_EQUAL 600l
#define OP_QNT 10000l
#define OP_RELPROD 20000l
#define OP_SUBST 30000l
#define OP_COMP 100000l
#define OP_CMPTO 200000l
#define OP_SWAP 300000l
#define OP_SWAPAUX 400000l


/* Variable associations */

struct var_assoc_
{
  bdd *assoc;			/* Array with associated BDDs */
  long last;			/* Indexindex for lowest variable */
};

typedef struct var_assoc_ *var_assoc;


struct assoc_list_
{
  struct var_assoc_ va;		/* The association */
  int id;			/* Identifier for this association */
  int refs;			/* Number of outstanding references */
  struct assoc_list_ *next;	/* The next association */
};

typedef struct assoc_list_ *assoc_list;


/* Variable blocks */

struct block_
{
  long num_children;
  struct block_ **children;
  int reorderable;
  long first_index;
  long last_index;
};


/* A cache bin; the cache is two-way associative. */

struct cache_bin_
{
  cache_entry entry[2];		/* LRU has index 1 */
};

typedef struct cache_bin_ cache_bin;


/* The cache */

struct cache_
{
  cache_bin *table;		/* The cache itself */
  int size_index;		/* Index giving number of cache lines */
  long size;			/* Number of cache lines */
  int cache_level;		/* Bin to start search in cache */
				/* Cache control functions: */
  long (*rehash_fn[8]) ARGS((cmu_bdd_manager, cache_entry));
				/* Rehashes a cache entry */
  int (*gc_fn[8]) ARGS((cmu_bdd_manager, cache_entry));
				/* Checks to see if an entry needs flushing */
  void (*purge_fn[8]) ARGS((cmu_bdd_manager, cache_entry));
				/* Called when purging an entry */
  void (*return_fn[8]) ARGS((cmu_bdd_manager, cache_entry));
				/* Called before returning from cache hit */
  int (*flush_fn[8]) ARGS((cmu_bdd_manager, cache_entry, pointer));
				/* Called when freeing variable association */
  int cache_ratio;		/* Cache to unique table size ratio */
  long entries;			/* Number of cache entries */
  long lookups;			/* Number of cache lookups */
  long hits;			/* Number of cache hits */
  long inserts;			/* Number of cache inserts */
  long collisions;		/* Number of cache collisions */
};

typedef struct cache_ cache;


/* One part of the node table */

struct var_table_
{
  bdd *table;			/* Pointers to the start of each bucket */
  int size_index;		/* Index giving number of buckets */
  long size;			/* Number of buckets */
  long entries;			/* Number of BDD nodes in table */
};

typedef struct var_table_ *var_table;


/* The BDD node table */

struct unique_
{
  var_table *tables;		/* Individual variable tables */
  void (*free_terminal_fn) ARGS((cmu_bdd_manager, INT_PTR, INT_PTR, pointer));
				/* Called when freeing MTBDD terminals */
  pointer free_terminal_env;	/* Environment for free terminal function */
  long entries;			/* Total number of BDD nodes */
  long gc_limit;		/* Try garbage collection at this point */
  long node_limit;		/* Maximum number of BDD nodes allowed */
  long gcs;			/* Number of garbage collections */
  long freed;			/* Number of nodes freed */
  long finds;			/* Number of find operations */
};

typedef struct unique_ unique;


/* Record manager size range stuff */

#define MIN_REC_SIZE ALLOC_ALIGNMENT
#define MAX_REC_SIZE 64

#define REC_MGRS (((MAX_REC_SIZE-MIN_REC_SIZE)/ALLOC_ALIGNMENT)+1)


/* Wrapper for jmp_buf since we may need to copy it sometimes and */
/* we can't easily do it if it happens to be an array. */

struct jump_buf_
{
  jmp_buf context;
};

typedef struct jump_buf_ jump_buf;


/* A BDD manager */

struct bdd_manager_
{
  unique unique_table;		/* BDD node table */
  cache op_cache;		/* System result cache */
  int check;			/* Number of find calls 'til size checks */
  bdd one;			/* BDD for one */
  bdd zero;			/* BDD for zero */
  int overflow;			/* Nonzero if node limit exceeded */
  void (*overflow_fn) ARGS((cmu_bdd_manager, pointer));
				/* Function to call on overflow */
  pointer overflow_env;		/* Environment for overflow function */
  void (*transform_fn) ARGS((cmu_bdd_manager, INT_PTR, INT_PTR, INT_PTR *, INT_PTR *, pointer));
				/* Function to transform terminal values */
  pointer transform_env;	/* Environment for transform_fn */
  int (*canonical_fn) ARGS((cmu_bdd_manager, INT_PTR, INT_PTR, pointer));
				/* Function to check if a terminal value is */
				/* canonical */
  block super_block;		/* Top-level variable block */
  void (*reorder_fn) ARGS((cmu_bdd_manager));
				/* Function to call to reorder variables */
  pointer reorder_data;		/* For saving information btwn reorderings */
  int allow_reordering;		/* Nonzero if reordering allowed */
  long nodes_at_start;		/* Nodes at start of operation */
  long vars;			/* Number of variables */
  long maxvars;			/* Maximum number of variables w/o resize */
  bdd *variables;		/* Array of variables, by indexindex */
  bdd_index_type *indexes;	/* indexindex -> index table */
  bdd_indexindex_type *indexindexes;
				/* index -> indexindex table */
  int curr_assoc_id;		/* Current variable association number */
  var_assoc curr_assoc;		/* Current variable association */
  assoc_list assocs;		/* Variable associations */
  struct var_assoc_ temp_assoc;	/* Temporary variable association */
  rec_mgr rms[REC_MGRS];	/* Record managers */
  long temp_op;			/* Current temporary operation number */
  jump_buf abort;		/* Jump for out-of-memory cleanup */
  void (*bag_it_fn) ARGS((cmu_bdd_manager, pointer));
				/* Non-null if going to abort at next find */
  pointer bag_it_env; char *hooks;		/* Environment for bag it function */
};


/* Abort stuff */

#define BDD_ABORTED 1
#define BDD_OVERFLOWED 2
#define BDD_REORDERED 3


#define FIREWALL(bddm)\
do\
  {\
    int retcode;\
    (bddm)->allow_reordering=1;\
    (bddm)->nodes_at_start=(bddm)->unique_table.entries;\
    while ((retcode=(setjmp((bddm)->abort.context))))\
      {\
	bdd_cleanup(bddm, retcode);\
	if (retcode == BDD_ABORTED || retcode == BDD_OVERFLOWED)\
	  return ((bdd)0);\
	(bddm)->nodes_at_start=(bddm)->unique_table.entries;\
      }\
  }\
while (0)


#define FIREWALL1(bddm, cleanupcode)\
do\
  {\
    int retcode;\
    (bddm)->allow_reordering=1;\
    (bddm)->nodes_at_start=(bddm)->unique_table.entries;\
    while ((retcode=(setjmp((bddm)->abort.context))))\
      {\
	bdd_cleanup(bddm, retcode);\
	cleanupcode\
	(bddm)->nodes_at_start=(bddm)->unique_table.entries;\
      }\
  }\
while (0)


/* Node hash function */

#define HASH_NODE(f, g) (((f) << 1)+(g))


/* Table size stuff */

#if defined(POWER_OF_2_SIZES)
#define TABLE_SIZE(size_index) (1l << (size_index))
#define BDD_REDUCE(i, size) (i)&=(size)-1
#else
#define TABLE_SIZE(size_index) (bdd_primes[size_index])
#define BDD_REDUCE(i, size)\
do\
  {\
    (i)%=(size);\
    if ((i) < 0)\
      (i)= -(i);\
  }\
while (0)
#endif


/* Record management */

#define BDD_NEW_REC(bddm, size) mem_new_rec((bddm)->rms[(ROUNDUP(size)-MIN_REC_SIZE)/ALLOC_ALIGNMENT])
#define BDD_FREE_REC(bddm, rec, size) mem_free_rec((bddm)->rms[(ROUNDUP(size)-MIN_REC_SIZE)/ALLOC_ALIGNMENT], (rec))


/* >>> Declarations for random routines */

/* Internal BDD routines */

extern int bdd_check_arguments ARGS((int, ...));
extern void bdd_check_array ARGS((bdd *));
extern bdd bdd_make_external ARGS((bdd));
extern int cmu_bdd_type_aux ARGS((cmu_bdd_manager, bdd));
extern void bdd_rehash_var_table ARGS((var_table, int));
extern bdd bdd_find_aux ARGS((cmu_bdd_manager, bdd_indexindex_type, INT_PTR, INT_PTR));
extern bdd cmu_bdd_ite_step ARGS((cmu_bdd_manager, bdd, bdd, bdd));
extern bdd cmu_bdd_exists_temp ARGS((cmu_bdd_manager, bdd, long));
extern bdd cmu_bdd_compose_temp ARGS((cmu_bdd_manager, bdd, bdd, bdd));
extern bdd cmu_bdd_substitute_step ARGS((cmu_bdd_manager, bdd, long, bdd (*) ARGS((cmu_bdd_manager, bdd, bdd, bdd)), var_assoc));
extern bdd cmu_bdd_swap_vars_temp ARGS((cmu_bdd_manager, bdd, bdd, bdd));
extern int cmu_bdd_compare_temp ARGS((cmu_bdd_manager, bdd, bdd, bdd));
extern double cmu_bdd_satisfying_fraction_step ARGS((cmu_bdd_manager, bdd));
extern void bdd_mark_shared_nodes ARGS((cmu_bdd_manager, bdd));
extern void bdd_number_shared_nodes ARGS((cmu_bdd_manager, bdd, hash_table, long *));
extern char *bdd_terminal_id ARGS((cmu_bdd_manager, bdd, char *(*) ARGS((cmu_bdd_manager, INT_PTR, INT_PTR, pointer)), pointer));
extern char *bdd_var_name ARGS((cmu_bdd_manager, bdd, char *(*) ARGS((cmu_bdd_manager, bdd, pointer)), pointer));
extern long bdd_find_block ARGS((block, long));
extern void bdd_block_delta ARGS((block, long));
extern void cmu_bdd_reorder_aux ARGS((cmu_bdd_manager));
extern void cmu_mtbdd_terminal_value_aux ARGS((cmu_bdd_manager, bdd, INT_PTR *, INT_PTR *));


/* System cache routines */

extern void bdd_insert_in_cache31 ARGS((cmu_bdd_manager, int, INT_PTR, INT_PTR, INT_PTR, INT_PTR));
extern int bdd_lookup_in_cache31 ARGS((cmu_bdd_manager, int, INT_PTR, INT_PTR, INT_PTR, INT_PTR *));
extern void bdd_insert_in_cache22 ARGS((cmu_bdd_manager, int, INT_PTR, INT_PTR, INT_PTR, INT_PTR));
extern int bdd_lookup_in_cache22 ARGS((cmu_bdd_manager, int, INT_PTR, INT_PTR, INT_PTR *, INT_PTR *));
extern void bdd_insert_in_cache13 ARGS((cmu_bdd_manager, int, INT_PTR, INT_PTR, INT_PTR, INT_PTR));
extern int bdd_lookup_in_cache13 ARGS((cmu_bdd_manager, int, INT_PTR, INT_PTR *, INT_PTR *, INT_PTR *));
extern void bdd_flush_cache ARGS((cmu_bdd_manager, int (*) ARGS((cmu_bdd_manager, cache_entry, pointer)), pointer));
extern void bdd_purge_cache ARGS((cmu_bdd_manager));
extern void bdd_flush_all ARGS((cmu_bdd_manager));
extern int bdd_cache_functions ARGS((cmu_bdd_manager,
				     int,
				     int (*) ARGS((cmu_bdd_manager, cache_entry)),
				     void (*) ARGS((cmu_bdd_manager, cache_entry)),
				     void (*) ARGS((cmu_bdd_manager,cache_entry)),
				     int (*) ARGS((cmu_bdd_manager, cache_entry, pointer))));
extern void cmu_bdd_free_cache_tag ARGS((cmu_bdd_manager, int));
extern void bdd_rehash_cache ARGS((cmu_bdd_manager, int));
extern void cmu_bdd_init_cache ARGS((cmu_bdd_manager));
extern void cmu_bdd_free_cache ARGS((cmu_bdd_manager));

#define bdd_insert_in_cache2(bddm, op, f, g, result)\
bdd_insert_in_cache31((bddm), CACHE_TYPE_TWO, (INT_PTR)(op), (INT_PTR)(f), (INT_PTR)(g), (INT_PTR)(result))
#define bdd_lookup_in_cache2(bddm, op, f, g, result)\
bdd_lookup_in_cache31((bddm), CACHE_TYPE_TWO, (INT_PTR)(op), (INT_PTR)(f), (INT_PTR)(g), (INT_PTR *)(result))

#define bdd_insert_in_cache1(bddm, op, f, result)\
bdd_insert_in_cache2((bddm), (op), (f), BDD_ONE(bddm), (result))
#define bdd_lookup_in_cache1(bddm, op, f, result)\
bdd_lookup_in_cache2((bddm), (op), (f), BDD_ONE(bddm), (result))

#define bdd_insert_in_cache2d(bddm, op, f, g, result)\
bdd_insert_in_cache31((bddm), CACHE_TYPE_TWODATA, (INT_PTR)(op), (INT_PTR)(f), (INT_PTR)(g), (INT_PTR)(result))
#define bdd_lookup_in_cache2d(bddm, op, f, g, result)\
bdd_lookup_in_cache31((bddm), CACHE_TYPE_TWODATA, (INT_PTR)(op), (INT_PTR)(f), (INT_PTR)(g), (INT_PTR *)(result))

#define bdd_insert_in_cache1d(bddm, op, f, result1, result2)\
bdd_insert_in_cache22((bddm), CACHE_TYPE_ONEDATA, (INT_PTR)(op), (INT_PTR)(f), (INT_PTR)(result1), (INT_PTR)(result2))
#define bdd_lookup_in_cache1d(bddm, op, f, result1, result2)\
bdd_lookup_in_cache22((bddm), CACHE_TYPE_ONEDATA, (INT_PTR)(op), (INT_PTR)(f), (INT_PTR *)(result1), (INT_PTR *)(result2))

#if defined(__STDC__)
#define cache_return_fn_none ((void (*)(cmu_bdd_manager, cache_entry))0)
#define cache_purge_fn_none ((void (*)(cmu_bdd_manager, cache_entry))0)
#define cache_reclaim_fn_none ((int (*)(cmu_bdd_manager, cache_entry, pointer))0)
#else
#define cache_return_fn_none ((void (*)())0)
#define cache_purge_fn_none ((void (*)())0)
#define cache_reclaim_fn_none ((int (*)())0)
#endif


/* Unique table routines */

extern void bdd_clear_temps ARGS((cmu_bdd_manager));
extern void bdd_sweep_var_table ARGS((cmu_bdd_manager, long, int));
extern void bdd_sweep ARGS((cmu_bdd_manager));
extern void bdd_cleanup ARGS((cmu_bdd_manager, int));
extern bdd bdd_find ARGS((cmu_bdd_manager, bdd_indexindex_type, bdd, bdd));
extern bdd bdd_find_terminal ARGS((cmu_bdd_manager, INT_PTR, INT_PTR));
extern var_table bdd_new_var_table ARGS((cmu_bdd_manager));
extern void cmu_bdd_init_unique ARGS((cmu_bdd_manager));
extern void cmu_bdd_free_unique ARGS((cmu_bdd_manager));


/* Error routines */

extern void cmu_bdd_fatal ARGS((char *));
extern void cmu_bdd_warning ARGS((char *));


/* >>> Hash table declarations */

struct hash_rec_
{
  bdd key;
  struct hash_rec_ *next;
};

typedef struct hash_rec_ *hash_rec;


struct hash_table_
{
  hash_rec *table;
  int size_index;
  long size;
  long entries;
  int item_size;
  cmu_bdd_manager bddm;
};


/* Hash table routines */

extern void bdd_insert_in_hash_table ARGS((hash_table, bdd, pointer));
extern pointer bdd_lookup_in_hash_table ARGS((hash_table, bdd));
extern hash_table bdd_new_hash_table ARGS((cmu_bdd_manager, int));
extern void cmu_bdd_free_hash_table ARGS((hash_table));


#undef ARGS

#endif
