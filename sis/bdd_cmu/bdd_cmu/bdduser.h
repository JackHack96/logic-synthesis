/* BDD user-visible definitions */


#if !defined(_BDDUSERH)
#define _BDDUSERH


#include <stdio.h>
#include <memuser.h>


#if defined(__STDC__)
#define args args
#else
#define args) (
#endif


/* Types */

typedef struct bdd_         *bdd;
typedef struct bdd_manager_ *cmu_bdd_manager;
typedef struct block_       *block;


/* Return values for cmu_bdd_type */

#define BDD_TYPE_NONTERMINAL 0
#define BDD_TYPE_ZERO 1
#define BDD_TYPE_ONE 2
#define BDD_TYPE_POSVAR 3
#define BDD_TYPE_NEGVAR 4
#define BDD_TYPE_OVERFLOW 5
#define BDD_TYPE_CONSTANT 6


/* Error codes for cmu_bdd_undump_bdd */

#define BDD_UNDUMP_FORMAT 1
#define BDD_UNDUMP_OVERFLOW 2
#define BDD_UNDUMP_IOERROR 3
#define BDD_UNDUMP_EOF 4


/* Basic BDD routine declarations */

extern bdd cmu_bdd_one(cmu_bdd_manager);

extern bdd cmu_bdd_zero(cmu_bdd_manager);

extern bdd cmu_bdd_new_var_first(cmu_bdd_manager);

extern bdd cmu_bdd_new_var_last(cmu_bdd_manager);

extern bdd cmu_bdd_new_var_before(cmu_bdd_manager, bdd);

extern bdd cmu_bdd_new_var_after(cmu_bdd_manager, bdd);

extern bdd cmu_bdd_var_with_index(cmu_bdd_manager, long);

extern bdd cmu_bdd_var_with_id(cmu_bdd_manager, long);

extern bdd cmu_bdd_ite(cmu_bdd_manager, bdd, bdd, bdd);

extern bdd cmu_bdd_and(cmu_bdd_manager, bdd, bdd);

extern bdd cmu_bdd_nand(cmu_bdd_manager, bdd, bdd);

extern bdd cmu_bdd_or(cmu_bdd_manager, bdd, bdd);

extern bdd cmu_bdd_nor(cmu_bdd_manager, bdd, bdd);

extern bdd cmu_bdd_xor(cmu_bdd_manager, bdd, bdd);

extern bdd cmu_bdd_xnor(cmu_bdd_manager, bdd, bdd);

extern bdd cmu_bdd_identity(cmu_bdd_manager, bdd);

extern bdd cmu_bdd_not(cmu_bdd_manager, bdd);

extern bdd cmu_bdd_if(cmu_bdd_manager, bdd);

extern long cmu_bdd_if_index(cmu_bdd_manager, bdd);

extern long cmu_bdd_if_id(cmu_bdd_manager, bdd);

extern bdd cmu_bdd_then(cmu_bdd_manager, bdd);

extern bdd cmu_bdd_else(cmu_bdd_manager, bdd);

extern bdd cmu_bdd_intersects(cmu_bdd_manager, bdd, bdd);

extern bdd cmu_bdd_implies(cmu_bdd_manager, bdd, bdd);

extern int cmu_bdd_type(cmu_bdd_manager, bdd);

extern void cmu_bdd_unfree(cmu_bdd_manager, bdd);

extern void cmu_bdd_free(cmu_bdd_manager, bdd);

extern long cmu_bdd_vars(cmu_bdd_manager);

extern long cmu_bdd_total_size(cmu_bdd_manager);

extern int cmu_bdd_cache_ratio(cmu_bdd_manager, int);

extern long cmu_bdd_node_limit(cmu_bdd_manager, long);

extern int cmu_bdd_overflow(cmu_bdd_manager);

extern void                 cmu_bdd_overflow_closure(cmu_bdd_manager, void (*)(cmu_bdd_manager, pointer)), pointer;

extern void cmu_bdd_abort_closure(cmu_bdd_manager, void (*)(cmu_bdd_manager, pointer)

),
pointer;

extern void cmu_bdd_stats(cmu_bdd_manager, FILE *);

extern cmu_bdd_manager cmu_bdd_init(void);

extern void cmu_bdd_quit(cmu_bdd_manager);


/* Variable association routine declarations */

extern int cmu_bdd_new_assoc(cmu_bdd_manager, bdd *, int);

extern void cmu_bdd_free_assoc(cmu_bdd_manager, int);

extern void cmu_bdd_temp_assoc(cmu_bdd_manager, bdd *, int);

extern void cmu_bdd_augment_temp_assoc(cmu_bdd_manager, bdd *, int);

extern int cmu_bdd_assoc(cmu_bdd_manager, int);


/* Comparison routine declarations */

extern int cmu_bdd_compare(cmu_bdd_manager, bdd, bdd, bdd);


/* Composition routine declarations */

extern bdd cmu_bdd_compose(cmu_bdd_manager, bdd, bdd, bdd);

extern bdd cmu_bdd_substitute(cmu_bdd_manager, bdd);


/* Variable exchange routine declarations */

extern bdd cmu_bdd_swap_vars(cmu_bdd_manager, bdd, bdd, bdd);


/* Quantification routine declarations */

extern bdd cmu_bdd_exists(cmu_bdd_manager, bdd);

extern bdd cmu_bdd_forall(cmu_bdd_manager, bdd);


/* Reduce routine declarations */

extern bdd cmu_bdd_reduce(cmu_bdd_manager, bdd, bdd);

extern bdd cmu_bdd_cofactor(cmu_bdd_manager, bdd, bdd);


/* Relational product routine declarations */

extern bdd cmu_bdd_rel_prod(cmu_bdd_manager, bdd, bdd);


/* Satisfying valuation routine declarations */

extern bdd cmu_bdd_satisfy(cmu_bdd_manager, bdd);

extern bdd cmu_bdd_satisfy_support(cmu_bdd_manager, bdd);

extern double cmu_bdd_satisfying_fraction(cmu_bdd_manager, bdd);


/* Generic apply routine declarations */

extern bdd bdd_apply2(cmu_bdd_manager, bdd (*)(cmu_bdd_manager, bdd *, bdd *, pointer)

), bdd, bdd,
pointer;

extern bdd bdd_apply1(cmu_bdd_manager, bdd (*)(cmu_bdd_manager, bdd *, pointer)

), bdd,
pointer;


/* Size and profile routine declarations */

extern long cmu_bdd_size(cmu_bdd_manager, bdd, int);

extern long cmu_bdd_size_multiple(cmu_bdd_manager, bdd *, int);

extern void cmu_bdd_profile(cmu_bdd_manager, bdd, long *, int);

extern void cmu_bdd_profile_multiple(cmu_bdd_manager, bdd *, long *, int);

extern void cmu_bdd_function_profile(cmu_bdd_manager, bdd, long *);

extern void cmu_bdd_function_profile_multiple(cmu_bdd_manager, bdd *, long *);


/* Print routine declarations */

#if defined(__STDC__)
#define bdd_naming_fn_none ((char *(*)(cmu_bdd_manager, bdd, pointer))0)
#define bdd_terminal_id_fn_none ((char *(*)(cmu_bdd_manager, INT_PTR, INT_PTR, pointer))0)
#else
#define bdd_naming_fn_none ((char *(*)())0)
#define bdd_terminal_id_fn_none ((char *(*)())0)
#endif

extern void cmu_bdd_print_bdd((cmu_bdd_manager,
                               bdd,
                               char *(*)(cmu_bdd_manager, bdd, pointer),

char *(*) (cmu_bdd_manager, INT_PTR, INT_PTR, pointer),
pointer,
FILE *));

extern void cmu_bdd_print_profile_aux((cmu_bdd_manager,
                                       long *,
                                       char *(*)(cmu_bdd_manager, bdd, pointer),
            pointer,

int,
FILE *));

extern void cmu_bdd_print_profile((cmu_bdd_manager,
                                   bdd,
                                   char *(*)(cmu_bdd_manager, bdd, pointer),
            pointer,

int,
FILE *));

extern void cmu_bdd_print_profile_multiple((cmu_bdd_manager,
                                            bdd *,
                                            char *(*)(cmu_bdd_manager, bdd, pointer),
            pointer,

int,
FILE *));

extern void cmu_bdd_print_function_profile((cmu_bdd_manager,
                                            bdd,
                                            char *(*)(cmu_bdd_manager, bdd, pointer),
            pointer,

int,
FILE *));

extern void cmu_bdd_print_function_profile_multiple((cmu_bdd_manager,
                                                     bdd *,
                                                     char *(*)(cmu_bdd_manager, bdd, pointer),
            pointer,

int,
FILE *));


/* Dump/undump routine declarations */

extern int cmu_bdd_dump_bdd(cmu_bdd_manager, bdd, bdd *, FILE *);

extern bdd cmu_bdd_undump_bdd(cmu_bdd_manager, bdd *, FILE *, int *);


/* Support routine declarations */

extern int cmu_bdd_depends_on(cmu_bdd_manager, bdd, bdd);

extern void cmu_bdd_support(cmu_bdd_manager, bdd, bdd *);


/* Unique table routine declarations */

extern void cmu_bdd_gc(cmu_bdd_manager);

extern void cmu_bdd_clear_refs(cmu_bdd_manager);


/* Dynamic reordering routines */

#if defined(__STDC__)
#define cmu_bdd_reorder_none ((void (*)(cmu_bdd_manager))0)
#else
#define cmu_bdd_reorder_none ((void (*)())0)
#endif

extern void cmu_bdd_reorder_stable_window3(cmu_bdd_manager);

extern void cmu_bdd_reorder_sift(cmu_bdd_manager);

extern void cmu_bdd_reorder_hybrid(cmu_bdd_manager);

extern void cmu_bdd_var_block_reorderable(cmu_bdd_manager, block, int);

extern void cmu_bdd_dynamic_reordering(cmu_bdd_manager, void (*)(cmu_bdd_manager));

extern void cmu_bdd_reorder(cmu_bdd_manager);


/* Variable block routines */

extern block cmu_bdd_new_var_block(cmu_bdd_manager, bdd, long);


/* Multi-terminal BDD routine declarations */

extern void mtbdd_transform_closure((cmu_bdd_manager,
                                     int (*)(cmu_bdd_manager, INT_PTR, INT_PTR, pointer),

void (*) (cmu_bdd_manager, INT_PTR, INT_PTR, INT_PTR *, INT_PTR *, pointer),
pointer));

extern void mtcmu_bdd_one_data(cmu_bdd_manager, INT_PTR, INT_PTR);

extern void cmu_mtbdd_free_terminal_closure((cmu_bdd_manager,
                                             void (*)(cmu_bdd_manager, INT_PTR, INT_PTR, pointer),
            pointer

));

extern bdd cmu_mtbdd_get_terminal(cmu_bdd_manager, INT_PTR, INT_PTR);

extern void cmu_mtbdd_terminal_value(cmu_bdd_manager, bdd, INT_PTR *, INT_PTR *);

extern bdd mtcmu_bdd_ite(cmu_bdd_manager, bdd, bdd, bdd);

extern bdd cmu_mtbdd_equal(cmu_bdd_manager, bdd, bdd);

extern bdd mtcmu_bdd_substitute(cmu_bdd_manager, bdd);

#define mtbdd_transform(bddm, f) (cmu_bdd_not(bddm, f))


#undef ARGS

#endif
