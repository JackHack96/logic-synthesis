/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/genlib/sptree.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:30 $
 *
 */
/* file @(#)sptree.h	1.1                      */
/* last modified on 5/29/91 at 12:35:35   */
#define tree_node_type_t	unsigned
#define OR_NODE		(tree_node_type_t) 0
#define AND_NODE	(tree_node_type_t) 1
#define NOR_NODE	(tree_node_type_t) 2
#define NAND_NODE	(tree_node_type_t) 3
#define ZERO_NODE	(tree_node_type_t) 4
#define ONE_NODE	(tree_node_type_t) 5
#define LEAF_NODE	(tree_node_type_t) 6


typedef struct tree_node_struct tree_node_t;
struct tree_node_struct {
    int nsons;		/* number of sons (0 implies leaf) */
    unsigned type:3;	/* NAND/NOR or OR/AND (nonleaf only) */
    unsigned phase:1;	/* 0==inverted, 1==normal (leaf and root only) */
    unsigned s:8;	/* series stacking level */
    unsigned p:8;	/* parallel stacking level */
    unsigned level:8;	/* level of the gate */
    tree_node_t **sons;	/* array of child pointers (nonleaf only)*/
    char *name;		/* name */
};


/* tree.c */
extern tree_node_t *gl_alloc_node(), *gl_alloc_leaf(), *gl_dup_tree();
extern void gl_free_tree(), gl_reverse_tree(), gl_print_tree();
extern void gl_compute_level();
extern void gl_print_tree_algebraic();
extern void gl_canonical_tree(), gl_assign_leaf_names(), gl_assign_node_names();
extern void gl_hash_save(), gl_make_well_formed();
extern int gl_hash_end(), gl_hash_find_or_add(), gl_compare_tree();
extern int gl_get_unique_leaf_pointers();
extern avl_tree *gl_hash_init();

/* io.c */
extern void gl_print_all_gates(), gl_print_all_nand_forms(); 
extern void gl_print_nand_forms(), gl_print_all_gates_genlib(); 
extern void gl_table_of_gate_count(), gl_table_of_nand_forms();

/* nand.c */
extern void gl_write_blif();
extern int gl_nand_gate_forms();

/* aoi.c */
extern int gl_gen_complex_gates(), gl_generate_complex_gates();

/* genlib.c */
extern int genlib();
extern char read_error_string[];
