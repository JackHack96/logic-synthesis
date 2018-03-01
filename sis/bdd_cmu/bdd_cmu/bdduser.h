/* BDD user-visible definitions */


#if !defined(_BDDUSERH)
#define _BDDUSERH


#include <stdio.h>
#include <memuser.h>


#if defined(__STDC__)
#define ARGS(args) args
#else
#define ARGS(args) ()
#endif


/* Types */

typedef struct bdd_ *bdd;
typedef struct bdd_manager_ *cmu_bdd_manager;
typedef struct block_ *block;


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

extern bdd cmu_bdd_one ARGS((cmu_bdd_manager));
extern bdd cmu_bdd_zero ARGS((cmu_bdd_manager));
extern bdd cmu_bdd_new_var_first ARGS((cmu_bdd_manager));
extern bdd cmu_bdd_new_var_last ARGS((cmu_bdd_manager));
extern bdd cmu_bdd_new_var_before ARGS((cmu_bdd_manager, bdd));
extern bdd cmu_bdd_new_var_after ARGS((cmu_bdd_manager, bdd));
extern bdd cmu_bdd_var_with_index ARGS((cmu_bdd_manager, long));
extern bdd cmu_bdd_var_with_id ARGS((cmu_bdd_manager, long));
extern bdd cmu_bdd_ite ARGS((cmu_bdd_manager, bdd, bdd, bdd));
extern bdd cmu_bdd_and ARGS((cmu_bdd_manager, bdd, bdd));
extern bdd cmu_bdd_nand ARGS((cmu_bdd_manager, bdd, bdd));
extern bdd cmu_bdd_or ARGS((cmu_bdd_manager, bdd, bdd));
extern bdd cmu_bdd_nor ARGS((cmu_bdd_manager, bdd, bdd));
extern bdd cmu_bdd_xor ARGS((cmu_bdd_manager, bdd, bdd));
extern bdd cmu_bdd_xnor ARGS((cmu_bdd_manager, bdd, bdd));
extern bdd cmu_bdd_identity ARGS((cmu_bdd_manager, bdd));
extern bdd cmu_bdd_not ARGS((cmu_bdd_manager, bdd));
extern bdd cmu_bdd_if ARGS((cmu_bdd_manager, bdd));
extern long cmu_bdd_if_index ARGS((cmu_bdd_manager, bdd));
extern long cmu_bdd_if_id ARGS((cmu_bdd_manager, bdd));
extern bdd cmu_bdd_then ARGS((cmu_bdd_manager, bdd));
extern bdd cmu_bdd_else ARGS((cmu_bdd_manager, bdd));
extern bdd cmu_bdd_intersects ARGS((cmu_bdd_manager, bdd, bdd));
extern bdd cmu_bdd_implies ARGS((cmu_bdd_manager, bdd, bdd));
extern int cmu_bdd_type ARGS((cmu_bdd_manager, bdd));
extern void cmu_bdd_unfree ARGS((cmu_bdd_manager, bdd));
extern void cmu_bdd_free ARGS((cmu_bdd_manager, bdd));
extern long cmu_bdd_vars ARGS((cmu_bdd_manager));
extern long cmu_bdd_total_size ARGS((cmu_bdd_manager));
extern int cmu_bdd_cache_ratio ARGS((cmu_bdd_manager, int));
extern long cmu_bdd_node_limit ARGS((cmu_bdd_manager, long));
extern int cmu_bdd_overflow ARGS((cmu_bdd_manager));
extern void cmu_bdd_overflow_closure ARGS((cmu_bdd_manager, void (*) ARGS((cmu_bdd_manager, pointer)), pointer));
extern void cmu_bdd_abort_closure ARGS((cmu_bdd_manager, void (*) ARGS((cmu_bdd_manager, pointer)), pointer));
extern void cmu_bdd_stats ARGS((cmu_bdd_manager, FILE *));
extern cmu_bdd_manager cmu_bdd_init ARGS((void));
extern void cmu_bdd_quit ARGS((cmu_bdd_manager));


/* Variable association routine declarations */

extern int cmu_bdd_new_assoc ARGS((cmu_bdd_manager, bdd *, int));
extern void cmu_bdd_free_assoc ARGS((cmu_bdd_manager, int));
extern void cmu_bdd_temp_assoc ARGS((cmu_bdd_manager, bdd *, int));
extern void cmu_bdd_augment_temp_assoc ARGS((cmu_bdd_manager, bdd *, int));
extern int cmu_bdd_assoc ARGS((cmu_bdd_manager, int));


/* Comparison routine declarations */

extern int cmu_bdd_compare ARGS((cmu_bdd_manager, bdd, bdd, bdd));


/* Composition routine declarations */

extern bdd cmu_bdd_compose ARGS((cmu_bdd_manager, bdd, bdd, bdd));
extern bdd cmu_bdd_substitute ARGS((cmu_bdd_manager, bdd));


/* Variable exchange routine declarations */

extern bdd cmu_bdd_swap_vars ARGS((cmu_bdd_manager, bdd, bdd, bdd));


/* Quantification routine declarations */

extern bdd cmu_bdd_exists ARGS((cmu_bdd_manager, bdd));
extern bdd cmu_bdd_forall ARGS((cmu_bdd_manager, bdd));


/* Reduce routine declarations */

extern bdd cmu_bdd_reduce ARGS((cmu_bdd_manager, bdd, bdd));
extern bdd cmu_bdd_cofactor ARGS((cmu_bdd_manager, bdd, bdd));


/* Relational product routine declarations */

extern bdd cmu_bdd_rel_prod ARGS((cmu_bdd_manager, bdd, bdd));


/* Satisfying valuation routine declarations */

extern bdd cmu_bdd_satisfy ARGS((cmu_bdd_manager, bdd));
extern bdd cmu_bdd_satisfy_support ARGS((cmu_bdd_manager, bdd));
extern double cmu_bdd_satisfying_fraction ARGS((cmu_bdd_manager, bdd));


/* Generic apply routine declarations */

extern bdd bdd_apply2 ARGS((cmu_bdd_manager, bdd (*) ARGS((cmu_bdd_manager, bdd *, bdd *, pointer)), bdd, bdd, pointer));
extern bdd bdd_apply1 ARGS((cmu_bdd_manager, bdd (*) ARGS((cmu_bdd_manager, bdd *, pointer)), bdd, pointer));


/* Size and profile routine declarations */

extern long cmu_bdd_size ARGS((cmu_bdd_manager, bdd, int));
extern long cmu_bdd_size_multiple ARGS((cmu_bdd_manager, bdd *, int));
extern void cmu_bdd_profile ARGS((cmu_bdd_manager, bdd, long *, int));
extern void cmu_bdd_profile_multiple ARGS((cmu_bdd_manager, bdd *, long *, int));
extern void cmu_bdd_function_profile ARGS((cmu_bdd_manager, bdd, long *));
extern void cmu_bdd_function_profile_multiple ARGS((cmu_bdd_manager, bdd *, long *));


/* Print routine declarations */

#if defined(__STDC__)
#define bdd_naming_fn_none ((char *(*)(cmu_bdd_manager, bdd, pointer))0)
#define bdd_terminal_id_fn_none ((char *(*)(cmu_bdd_manager, INT_PTR, INT_PTR, pointer))0)
#else
#define bdd_naming_fn_none ((char *(*)())0)
#define bdd_terminal_id_fn_none ((char *(*)())0)
#endif

extern void cmu_bdd_print_bdd ARGS((cmu_bdd_manager,
				bdd,
				char *(*) ARGS((cmu_bdd_manager, bdd, pointer)),
				char *(*) ARGS((cmu_bdd_manager, INT_PTR, INT_PTR, pointer)),
				pointer,
				FILE *));
extern void cmu_bdd_print_profile_aux ARGS((cmu_bdd_manager,
					long *,
					char *(*) ARGS((cmu_bdd_manager, bdd, pointer)),
					pointer,
					int,
					FILE *));
extern void cmu_bdd_print_profile ARGS((cmu_bdd_manager,
				    bdd,
				    char *(*) ARGS((cmu_bdd_manager, bdd, pointer)),
				    pointer,
				    int,
				    FILE *));
extern void cmu_bdd_print_profile_multiple ARGS((cmu_bdd_manager,
					     bdd *,
					     char *(*) ARGS((cmu_bdd_manager, bdd, pointer)),
					     pointer,
					     int,
					     FILE *));
extern void cmu_bdd_print_function_profile ARGS((cmu_bdd_manager,
					     bdd,
					     char *(*) ARGS((cmu_bdd_manager, bdd, pointer)),
					     pointer,
					     int,
					     FILE *));
extern void cmu_bdd_print_function_profile_multiple ARGS((cmu_bdd_manager,
						      bdd *,
						      char *(*) ARGS((cmu_bdd_manager, bdd, pointer)),
						      pointer,
						      int,
						      FILE *));


/* Dump/undump routine declarations */

extern int cmu_bdd_dump_bdd ARGS((cmu_bdd_manager, bdd, bdd *, FILE *));
extern bdd cmu_bdd_undump_bdd ARGS((cmu_bdd_manager, bdd *, FILE *, int *));


/* Support routine declarations */

extern int cmu_bdd_depends_on ARGS((cmu_bdd_manager, bdd, bdd));
extern void cmu_bdd_support ARGS((cmu_bdd_manager, bdd, bdd *));


/* Unique table routine declarations */

extern void cmu_bdd_gc ARGS((cmu_bdd_manager));
extern void cmu_bdd_clear_refs ARGS((cmu_bdd_manager));


/* Dynamic reordering routines */

#if defined(__STDC__)
#define cmu_bdd_reorder_none ((void (*)(cmu_bdd_manager))0)
#else
#define cmu_bdd_reorder_none ((void (*)())0)
#endif

extern void cmu_bdd_reorder_stable_window3 ARGS((cmu_bdd_manager));
extern void cmu_bdd_reorder_sift ARGS((cmu_bdd_manager));
extern void cmu_bdd_reorder_hybrid ARGS((cmu_bdd_manager));
extern void cmu_bdd_var_block_reorderable ARGS((cmu_bdd_manager, block, int));
extern void cmu_bdd_dynamic_reordering ARGS((cmu_bdd_manager, void (*) ARGS((cmu_bdd_manager))));
extern void cmu_bdd_reorder ARGS((cmu_bdd_manager));


/* Variable block routines */

extern block cmu_bdd_new_var_block ARGS((cmu_bdd_manager, bdd, long));


/* Multi-terminal BDD routine declarations */

extern void mtbdd_transform_closure ARGS((cmu_bdd_manager,
					  int (*) ARGS((cmu_bdd_manager, INT_PTR, INT_PTR, pointer)),
					  void (*) ARGS((cmu_bdd_manager, INT_PTR, INT_PTR, INT_PTR *, INT_PTR *, pointer)),
					  pointer));
extern void mtcmu_bdd_one_data ARGS((cmu_bdd_manager, INT_PTR, INT_PTR));
extern void cmu_mtbdd_free_terminal_closure ARGS((cmu_bdd_manager,
					      void (*) ARGS((cmu_bdd_manager, INT_PTR, INT_PTR, pointer)),
					      pointer));
extern bdd cmu_mtbdd_get_terminal ARGS((cmu_bdd_manager, INT_PTR, INT_PTR));
extern void cmu_mtbdd_terminal_value ARGS((cmu_bdd_manager, bdd, INT_PTR *, INT_PTR *));
extern bdd mtcmu_bdd_ite ARGS((cmu_bdd_manager, bdd, bdd, bdd));
extern bdd cmu_mtbdd_equal ARGS((cmu_bdd_manager, bdd, bdd));
extern bdd mtcmu_bdd_substitute ARGS((cmu_bdd_manager, bdd));
#define mtbdd_transform(bddm, f) (cmu_bdd_not(bddm, f))


#undef ARGS

#endif
