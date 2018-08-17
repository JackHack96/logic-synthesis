
#ifndef bdd_h /* { */
#define bdd_h

#include "var_set.h"



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

typedef struct bdd_manager bdd_manager;        /* referenced via a pointer only */
typedef struct bdd_t       bdd_t;            /* referenced via a pointer only */
typedef unsigned int       bdd_variableId;        /* the id of the variable in a bdd node */
typedef struct bdd_node    bdd_node;               /* referenced via a pointer only */
typedef int                bdd_literal;                    /* integers in the set { 0, 1, 2 } */

#define boolean        int

/* 
 * Initialization data structure.
 */
typedef struct bdd_mgr_init {
    struct {
        boolean      on;                   /* TRUE/FALSE: is the cache on */
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
        float        ratio;                    /* allocate new bdd_nodes to achieve ratio of used to unused nodes */
        unsigned int init_blocks;       /* number of bdd_nodeBlocks initially allocated */
    } nodes;
}                          bdd_mgr_init;

/*
 * Match types for BDD minimization.
 */
typedef enum {
    BDD_MIN_TSM,        /* two-side match */
    BDD_MIN_OSM,        /* one-side match */
    BDD_MIN_OSDM        /* one-side DC match */
}                          bdd_min_match_type_t;

/*
 * BDD Manager Allocation And Destruction
 */
extern void bdd_end(bdd_manager *);

extern void bdd_register_daemon(bdd_manager *, void (*daemon)());

extern void bdd_set_mgr_init_dflts(bdd_mgr_init *);

extern bdd_manager *bdd_start(int);

extern bdd_manager *bdd_start_with_params(int, bdd_mgr_init *);

/*
 * BDD Variable Allocation
 */
extern bdd_t *bdd_create_variable(bdd_manager *);

extern bdd_t *bdd_get_variable(bdd_manager *, bdd_variableId);

/*
 * BDD Formula Management
 */
extern bdd_t *bdd_dup(bdd_t *);

extern void bdd_free(bdd_t *);

/* 
 * Operations on BDD Formulas
 */
extern bdd_t *bdd_and(bdd_t *, bdd_t *, boolean, boolean);

extern bdd_t *bdd_and_smooth(bdd_t *, bdd_t *, array_t *);

extern bdd_t *bdd_between(bdd_t *, bdd_t *);

extern bdd_t *bdd_cofactor(bdd_t *, bdd_t *);

extern bdd_t *bdd_compose(bdd_t *, bdd_t *, bdd_t *);

extern bdd_t *bdd_consensus(bdd_t *, array_t *);

extern bdd_t *bdd_cproject(bdd_t *, array_t *);

extern bdd_t *bdd_else(bdd_t *);

extern bdd_t *bdd_ite(bdd_t *, bdd_t *, bdd_t *, boolean, boolean, boolean);

extern bdd_t *bdd_minimize(bdd_t *, bdd_t *);

extern bdd_t *bdd_minimize_with_params(bdd_t *, bdd_t *, bdd_min_match_type_t, boolean, boolean, boolean);

extern bdd_t *bdd_not(bdd_t *);

extern bdd_t *bdd_one(bdd_manager *);

extern bdd_t *bdd_or(bdd_t *, bdd_t *, boolean, boolean);

extern bdd_t *bdd_smooth(bdd_t *, array_t *);

extern bdd_t *bdd_substitute(bdd_t *, array_t *, array_t *);

extern bdd_t *bdd_then(bdd_t *);

extern bdd_t *bdd_top_var(bdd_t *);

extern bdd_t *bdd_xnor(bdd_t *, bdd_t *);

extern bdd_t *bdd_xor(bdd_t *, bdd_t *);

extern bdd_t *bdd_zero(bdd_manager *);

/*
 * Queries about BDD Formulas
 */
extern boolean bdd_equal(bdd_t *, bdd_t *);

extern boolean bdd_is_cube(bdd_t *);

extern boolean bdd_is_tautology(bdd_t *, boolean);

extern boolean bdd_leq(bdd_t *, bdd_t *, boolean, boolean);

/*
 * Statistics and Other Queries
 */
typedef struct bdd_cache_stats {
    unsigned int hits;
    unsigned int misses;
    unsigned int collisions;
    unsigned int inserts;
}                          bdd_cache_stats;

typedef struct bdd_stats {
    struct {
        bdd_cache_stats hashtable;   /* the unique table; collisions and inserts fields not used */
        bdd_cache_stats itetable;
        bdd_cache_stats consttable;
        bdd_cache_stats adhoc;
    } cache;        /* various cache statistics */
    struct {
        unsigned int calls;
        struct {
            unsigned int trivial;
            unsigned int cached;
            unsigned int full;
        }            returns;
    } ITE_ops,
      ITE_constant_ops,
      adhoc_ops;
    struct {
        unsigned int total;
    } blocks;        /* bdd_nodeBlock count */
    struct {
        unsigned int used;
        unsigned int unused;
        unsigned int total;
        unsigned int peak;
    } nodes;        /* bdd_node count */
    struct {
        unsigned int used;
        unsigned int unused;
        unsigned int total;
        unsigned int blocks;
    } extptrs;        /* bdd_t count */
    struct {
        unsigned int times;     /* the number of times the garbage-collector has run */
        unsigned int nodes_collected; /* cumulative number of nodes collected over life of manager */
        long         runtime;           /* cumulative CPU time spent garbage collecting */
    } gc;
    struct {
        int          first_sbrk;         /* value of sbrk at start of manager; used to analyze memory usage */
        int          last_sbrk;          /* value of last sbrk (see "man sbrk") fetched; used to analyze memory usage */
        unsigned int manager;
        unsigned int nodes;
        unsigned int hashtable;
        unsigned int ext_ptrs;
        unsigned int ITE_cache;
        unsigned int ITE_const_cache;
        unsigned int adhoc_cache;
        unsigned int total;
    } memory;           /* memory usage */
}                          bdd_stats;

extern double bdd_count_onset(bdd_t *, array_t *);

extern bdd_manager *bdd_get_manager(bdd_t *);

extern bdd_node *bdd_get_node(bdd_t *, boolean *);

extern void bdd_get_stats(bdd_manager *, bdd_stats *);

extern var_set_t *bdd_get_support(bdd_t *);

extern array_t *bdd_get_varids(array_t *);

extern unsigned int bdd_num_vars(bdd_manager *);

extern void bdd_print(bdd_t *);

extern void bdd_print_stats(bdd_stats, FILE *);

extern int bdd_size(bdd_t *);

extern bdd_variableId bdd_top_var_id(bdd_t *);

/*
 * Traversal of BDD Formulas
 */
typedef enum {
    bdd_EMPTY,
    bdd_NONEMPTY
}                          bdd_gen_status;

typedef enum {
    bdd_gen_cubes,
    bdd_gen_nodes
}                          bdd_gen_type;

typedef struct {
    bdd_manager    *manager;
    bdd_gen_status status;
    bdd_gen_type   type;
    union {
        struct {
            array_t *cube;    /* bdd_cube_literal */
            /* ... expansion ... */
        } cubes;
        struct {
            st_table *visited;    /* of bdd_node* */
            /* ... expansion ... */
        } nodes;
    }              gen;
    struct {
        int      sp;
        bdd_node **stack;
    }              stack;
    bdd_node       *node;
}                          bdd_gen;

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

extern int bdd_gen_free(bdd_gen *);

/*
 * These are NOT to be used directly; only indirectly in the macros.
 */
extern bdd_gen *bdd_first_cube(bdd_t *, array_t **);

extern boolean bdd_next_cube(bdd_gen *, array_t **);

extern bdd_gen *bdd_first_node(bdd_t *, bdd_node **);

extern boolean bdd_next_node(bdd_gen *, bdd_node **);

/* 
 * Miscellaneous
 */
extern void bdd_set_gc_mode(bdd_manager *, boolean);

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
}                          bdd_external_hooks;

extern bdd_external_hooks *bdd_get_external_hooks(bdd_manager *);

/*
 * Dynamic reordering.
 */
typedef enum {
    BDD_REORDER_SIFT,
    BDD_REORDER_WINDOW,
    BDD_REORDER_NONE
}                          bdd_reorder_type_t;

extern void bdd_dynamic_reordering(bdd_manager *, bdd_reorder_type_t);

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

