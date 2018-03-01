/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
 *
 */
#ifndef bdd_h /* { */
#define bdd_h

#include "var_set.h"

/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/bdd_ucb/bdd.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:15:01 $
 * $Log: bdd.h,v $
 * Revision 1.1.1.1  2004/02/07 10:15:01  pchong
 * imported
 *
 * Revision 1.9  1993/07/27  20:15:45  sis
 * Added declarations of 5 new functions, and bdd_min_match enumerated type.
 *
 * Revision 1.9  1993/07/27  20:15:45  sis
 * Added declarations of 5 new functions, and bdd_min_match enumerated type.
 *
 * Revision 1.9  1993/07/27  19:29:53  shiple
 * Added declarations of 5 new functions, and bdd_min_match enumerated type.
 *
 * Revision 1.8  1993/07/19  21:27:59  shiple
 * Added declaration of bdd_get_varids.
 *
 * Revision 1.7  1993/06/30  00:14:37  shiple
 * Added declaration of bdd_dynamic_reordering.
 *
 * Revision 1.6  1993/06/28  22:34:05  shiple
 * Added declaration of bdd_num_vars.
 *
 * Revision 1.5  1993/06/04  15:42:51  shiple
 * Added declarations for the new functions bdd_get_manager, bdd_top_var_id,
 * and bdd_get_node.  Removed declaration for bdd_set_external_hooks.
 *
 * Revision 1.4  1993/05/03  20:27:49  shiple
 * Made changes for ANSI C compatibility: 1) moved the definition of
 * bdd_mgr_init forward; 2) removed illegal comma in enum definition; 3)
 * fixed declaration of bdd_register_daemon.  Changed default sizes of
 * caches.  Added declarations for bdd_cproject and bdd_consensus.
 *
 * Revision 1.3  1993/02/24  20:30:46  shiple
 * Added declarations for bdd_get_support and bdd_count_onset.
 * Added include of var_set.h
 *
 * Revision 1.2  1992/09/19  02:17:44  shiple
 * Version 2.4
 * Prefaced compile time debug switches with BDD_.  Added BDD_MEMORY_USAGE debug switch.
 * Added sbrk, cache.itetable, gc.runtime, memory, ext ptr blocks, peak nodes,
 * and nodes collected to stats data structure.  Added BDD_DFLT_* definitions.
 * Always define boolean. Added bdd_register_daemon declaration.
 *
 * Revision 1.1  1992/07/29  00:26:43  shiple
 * Initial revision
 *
 * Revision 1.3  1992/05/06  18:51:03  sis
 * SIS release 1.1
 *
 * Revision 1.2  1992/04/17  21:29:14  sis
 * Corrected some arguments to EXTERN declarations.
 *
 * Revision 1.1  92/01/08  17:34:24  sis
 * Initial revision
 * 
 * Revision 1.2  91/05/01  17:46:40  shiple
 * convert to new declaration format using EXTERN and ARGS
 * 
 * Revision 1.1  91/03/27  14:35:27  shiple
 * Initial revision
 * 
 *
 */

/*
 *    The BDD Package using Garbage Collection
 *
 *    Within a bdd manager there can be multiple
 *    bdd's which all share the same storage.
 */

 /* basic definitions for debugging */

#undef BDD_AUTOMATED_STATISTICS_GATHERING
#undef BDD_DEBUG
#undef BDD_DEBUG_AGE
#undef BDD_DEBUG_EXT
#undef BDD_DEBUG_EXT_ALL
#undef BDD_DEBUG_GC
#undef BDD_DEBUG_GC_STATS
#undef BDD_DEBUG_LIFESPAN
#undef BDD_DEBUG_SF
#undef BDD_DEBUG_UID
#undef BDD_FLIGHT_RECORDER
#define BDD_INLINE_ITE
#define BDD_INLINE_ITE_CONSTANT
#undef BDD_NO_GC
#undef BDD_STATS
#undef BDD_DEBUG_LIFESPAN_TRACEFILE "/usr/tmp/lifespan.trace" 
#undef BDD_FLIGHT_RECORDER_LOGFILE "/usr/tmp/flight_recorder%06d.log"
#undef BDD_MEMORY_USAGE

typedef struct bdd_manager bdd_manager;		/* referenced via a pointer only */
typedef struct bdd_t bdd_t;			/* referenced via a pointer only */
typedef unsigned int bdd_variableId;		/* the id of the variable in a bdd node */
typedef struct bdd_node bdd_node;               /* referenced via a pointer only */
typedef int bdd_literal;	                /* integers in the set { 0, 1, 2 } */

#define boolean		int

/* 
 * Initialization data structure.
 */
typedef struct bdd_mgr_init {
    struct {
        boolean on;                   /* TRUE/FALSE: is the cache on */
        unsigned int resize_at;       /* percentage at which to resize (e.g. 85% is 85); doesn't apply to adhoc */
        unsigned int max_size;        /* max allowable number of buckets; for adhoc, max allowable number of entries */
    } ITE_cache,
      ITE_const_cache,
      adhoc_cache;
    struct {
      boolean on;                     /* TRUE/FALSE: is the garbage collector on */
    } garbage_collector;
    struct {
      void (*daemon)();               /* used for callback when memory limit exceeded */
      unsigned int limit;             /* upper bound on memory allocated by the manager; in megabytes */
    } memory;
    struct {
      float ratio;                    /* allocate new bdd_nodes to achieve ratio of used to unused nodes */
      unsigned int init_blocks;       /* number of bdd_nodeBlocks initially allocated */
    } nodes;
} bdd_mgr_init;

/*
 * Match types for BDD minimization.
 */
typedef enum {
    BDD_MIN_TSM,		/* two-side match */
    BDD_MIN_OSM,		/* one-side match */
    BDD_MIN_OSDM		/* one-side DC match */
} bdd_min_match_type_t;

/*
 * BDD Manager Allocation And Destruction
 */
EXTERN void bdd_end ARGS((bdd_manager *));
EXTERN void bdd_register_daemon ARGS((bdd_manager *, void (*daemon)()));
EXTERN void bdd_set_mgr_init_dflts ARGS((bdd_mgr_init *));
EXTERN bdd_manager *bdd_start ARGS((int));
EXTERN bdd_manager *bdd_start_with_params ARGS((int, bdd_mgr_init *));

/*
 * BDD Variable Allocation
 */
EXTERN bdd_t *bdd_create_variable ARGS((bdd_manager *));		
EXTERN bdd_t *bdd_get_variable ARGS((bdd_manager *, bdd_variableId));	

/*
 * BDD Formula Management
 */
EXTERN bdd_t *bdd_dup ARGS((bdd_t *));
EXTERN void bdd_free ARGS((bdd_t *));

/* 
 * Operations on BDD Formulas
 */
EXTERN bdd_t *bdd_and ARGS((bdd_t *, bdd_t *, boolean, boolean));
EXTERN bdd_t *bdd_and_smooth ARGS((bdd_t *, bdd_t *, array_t *));
EXTERN bdd_t *bdd_between ARGS((bdd_t *, bdd_t *));
EXTERN bdd_t *bdd_cofactor ARGS((bdd_t *, bdd_t *));
EXTERN bdd_t *bdd_compose ARGS((bdd_t *, bdd_t *, bdd_t *));
EXTERN bdd_t *bdd_consensus ARGS((bdd_t *, array_t *));
EXTERN bdd_t *bdd_cproject ARGS((bdd_t *, array_t *));
EXTERN bdd_t *bdd_else ARGS((bdd_t *));
EXTERN bdd_t *bdd_ite ARGS((bdd_t *, bdd_t *, bdd_t *, boolean, boolean, boolean));
EXTERN bdd_t *bdd_minimize ARGS((bdd_t *, bdd_t *));
EXTERN bdd_t *bdd_minimize_with_params ARGS((bdd_t *, bdd_t *, bdd_min_match_type_t, boolean, boolean, boolean));
EXTERN bdd_t *bdd_not ARGS((bdd_t *));
EXTERN bdd_t *bdd_one ARGS((bdd_manager *));
EXTERN bdd_t *bdd_or ARGS((bdd_t *, bdd_t *, boolean, boolean));
EXTERN bdd_t *bdd_smooth ARGS((bdd_t *, array_t *));
EXTERN bdd_t *bdd_substitute ARGS((bdd_t *, array_t *, array_t *));
EXTERN bdd_t *bdd_then ARGS((bdd_t *));
EXTERN bdd_t *bdd_top_var ARGS((bdd_t *));
EXTERN bdd_t *bdd_xnor ARGS((bdd_t *, bdd_t *));
EXTERN bdd_t *bdd_xor ARGS((bdd_t *, bdd_t *));
EXTERN bdd_t *bdd_zero ARGS((bdd_manager *));

/*
 * Queries about BDD Formulas
 */
EXTERN boolean bdd_equal ARGS((bdd_t *, bdd_t *));
EXTERN boolean bdd_is_cube ARGS((bdd_t *));
EXTERN boolean bdd_is_tautology ARGS((bdd_t *, boolean));
EXTERN boolean bdd_leq ARGS((bdd_t *, bdd_t *, boolean, boolean));

/*
 * Statistics and Other Queries
 */
typedef struct bdd_cache_stats {
    unsigned int hits;
    unsigned int misses;
    unsigned int collisions;
    unsigned int inserts;
} bdd_cache_stats;

typedef struct bdd_stats {
    struct {
	bdd_cache_stats hashtable;   /* the unique table; collisions and inserts fields not used */ 
	bdd_cache_stats itetable;
	bdd_cache_stats consttable;
	bdd_cache_stats adhoc;
    } cache;		/* various cache statistics */
    struct {
	unsigned int calls;
	struct {
	    unsigned int trivial;
	    unsigned int cached;
	    unsigned int full;
	} returns;
    } ITE_ops,
      ITE_constant_ops,
      adhoc_ops;
    struct {
	unsigned int total;
    } blocks;		/* bdd_nodeBlock count */
    struct {
	unsigned int used;
	unsigned int unused;
	unsigned int total;
        unsigned int peak;
    } nodes;		/* bdd_node count */
    struct {
	unsigned int used;
	unsigned int unused;
	unsigned int total;
        unsigned int blocks; 
    } extptrs;		/* bdd_t count */
    struct {
	unsigned int times;     /* the number of times the garbage-collector has run */
        unsigned int nodes_collected; /* cumulative number of nodes collected over life of manager */
        long runtime;           /* cumulative CPU time spent garbage collecting */
    } gc;
    struct {
        int first_sbrk;         /* value of sbrk at start of manager; used to analyze memory usage */
        int last_sbrk;          /* value of last sbrk (see "man sbrk") fetched; used to analyze memory usage */
        unsigned int manager;
	unsigned int nodes;
	unsigned int hashtable;
	unsigned int ext_ptrs;
	unsigned int ITE_cache;
	unsigned int ITE_const_cache;
	unsigned int adhoc_cache;
	unsigned int total;
    } memory;           /* memory usage */
} bdd_stats;

EXTERN double bdd_count_onset ARGS((bdd_t *, array_t *));
EXTERN bdd_manager *bdd_get_manager ARGS((bdd_t *));
EXTERN bdd_node *bdd_get_node ARGS((bdd_t *, boolean *));
EXTERN void bdd_get_stats ARGS((bdd_manager *, bdd_stats *));
EXTERN var_set_t *bdd_get_support ARGS((bdd_t *));
EXTERN array_t *bdd_get_varids ARGS((array_t *));
EXTERN unsigned int bdd_num_vars ARGS((bdd_manager *));
EXTERN void bdd_print ARGS((bdd_t *));
EXTERN void bdd_print_stats ARGS((bdd_stats, FILE *));
EXTERN int bdd_size ARGS((bdd_t *));
EXTERN bdd_variableId bdd_top_var_id ARGS((bdd_t *));

/*
 * Traversal of BDD Formulas
 */
typedef enum {
    bdd_EMPTY,
    bdd_NONEMPTY
} bdd_gen_status;

typedef enum {
    bdd_gen_cubes,
    bdd_gen_nodes
} bdd_gen_type;

typedef struct {
    bdd_manager *manager;
    bdd_gen_status status;
    bdd_gen_type type;
    union {
	struct {
	    array_t *cube;	/* bdd_cube_literal */
	    /* ... expansion ... */
	} cubes;
	struct {
	    st_table *visited;	/* of bdd_node* */
	    /* ... expansion ... */
	} nodes;
    } gen;	
    struct {
	int sp;
	bdd_node **stack;
    } stack;
    bdd_node *node;
} bdd_gen;

/*
 *    foreach macro in the most misesque tradition
 *    bdd_gen_free always returns 0
 */

/*
 *    foreach_bdd_cube(fn, gen, cube)
 *    bdd_t *fn;
 *    bdd_gen *gen;
 *    array_t *cube;	- return
 *
 *    foreach_bdd_cube(fn, gen, cube) {
 *        ...
 *    }
 */
#define foreach_bdd_cube(fn, gen, cube)\
  for((gen) = bdd_first_cube(fn, &cube);\
      ((gen)->status != bdd_EMPTY) ? TRUE: bdd_gen_free(gen);\
      (void) bdd_next_cube(gen, &cube))

/*
 *    foreach_bdd_node(fn, gen, node)
 *    bdd_t *fn;
 *    bdd_gen *gen;
 *    bdd_node *node;	- return
 */
#define foreach_bdd_node(fn, gen, node)\
  for((gen) = bdd_first_node(fn, &node);\
      ((gen)->status != bdd_EMPTY) ? TRUE: bdd_gen_free(gen);\
      (void) bdd_next_node(gen, &node))

EXTERN int bdd_gen_free ARGS((bdd_gen *));

/*
 * These are NOT to be used directly; only indirectly in the macros.
 */
EXTERN bdd_gen *bdd_first_cube ARGS((bdd_t *, array_t **));
EXTERN boolean bdd_next_cube ARGS((bdd_gen *, array_t **));
EXTERN bdd_gen *bdd_first_node ARGS((bdd_t *, bdd_node **));
EXTERN boolean bdd_next_node ARGS((bdd_gen *, bdd_node **));

/* 
 * Miscellaneous
 */
EXTERN void bdd_set_gc_mode ARGS((bdd_manager *, boolean));

/*
 *    These are the hooks for stuff that uses bdd's
 *
 *    There are three hooks, and users may use them in whatever
 *    way they wish; these hooks are guaranteed to never be used
 *    by the bdd package.
 */
typedef struct bdd_external_hooks {
    char *network;
    char *mdd;
    char *undef1;
} bdd_external_hooks;

EXTERN bdd_external_hooks *bdd_get_external_hooks ARGS((bdd_manager *));

/*
 * Dynamic reordering.
 */
typedef enum {
    BDD_REORDER_SIFT,
    BDD_REORDER_WINDOW,
    BDD_REORDER_NONE
} bdd_reorder_type_t;

EXTERN void bdd_dynamic_reordering ARGS((bdd_manager *, bdd_reorder_type_t));

/*
 * Default settings.
 */
#define BDD_NO_LIMIT                      ((1<<30)-2)
#define BDD_DFLT_ITE_ON                   TRUE
#define BDD_DFLT_ITE_RESIZE_AT            75
#define BDD_DFLT_ITE_MAX_SIZE             1000000
#define BDD_DFLT_ITE_CONST_ON             TRUE
#define BDD_DFLT_ITE_CONST_RESIZE_AT      75
#define BDD_DFLT_ITE_CONST_MAX_SIZE       1000000
#define BDD_DFLT_ADHOC_ON                 TRUE
#define BDD_DFLT_ADHOC_RESIZE_AT          0
#define BDD_DFLT_ADHOC_MAX_SIZE           10000000
#define BDD_DFLT_GARB_COLLECT_ON          TRUE
#define BDD_DFLT_DAEMON                   NIL(void)
#define BDD_DFLT_MEMORY_LIMIT             BDD_NO_LIMIT   
#define BDD_DFLT_NODE_RATIO               2.0
#define BDD_DFLT_INIT_BLOCKS              10

#endif /* } */

