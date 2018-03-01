/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/avl/avl.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:17 $
 *
 */
#ifndef AVL_INCLUDED
#define AVL_INCLUDED


typedef struct avl_node_struct avl_node;
struct avl_node_struct {
    avl_node *left, *right;
    char *key;
    char *value;
    int height;
};


typedef struct avl_tree_struct avl_tree;
struct avl_tree_struct {
    avl_node *root;
    int (*compar)();
    int num_entries;
    int modified;
};


typedef struct avl_generator_struct avl_generator;
struct avl_generator_struct {
    avl_tree *tree;
    avl_node **nodelist;
    int count;
};


#define AVL_FORWARD 	0
#define AVL_BACKWARD 	1


EXTERN avl_tree *avl_init_table ARGS((int (*)()));
EXTERN int avl_delete ARGS((avl_tree *, char **, char **));
EXTERN int avl_insert ARGS((avl_tree *, char *, char *));
EXTERN int avl_lookup ARGS((avl_tree *, char *, char **));
EXTERN int avl_first ARGS((avl_tree *, char **, char **));
EXTERN int avl_last ARGS((avl_tree *, char **, char **));
EXTERN int avl_find_or_add ARGS((avl_tree *, char *, char ***));
EXTERN int avl_count ARGS((avl_tree *));
EXTERN int avl_numcmp ARGS((char *, char *));
EXTERN int avl_gen ARGS((avl_generator *, char **, char **));
EXTERN void avl_foreach ARGS((avl_tree *, void (*)(), int));
EXTERN void avl_free_table ARGS((avl_tree *, void (*)(), void (*)()));
EXTERN void avl_free_gen ARGS((avl_generator *));
EXTERN avl_generator *avl_init_gen ARGS((avl_tree *, int));

#define avl_is_member(tree, key)	avl_lookup(tree, key, (char **) 0)

#define avl_foreach_item(table, gen, dir, key_p, value_p) 	\
    for(gen = avl_init_gen(table, dir); 			\
	    avl_gen(gen, key_p, value_p) || (avl_free_gen(gen),0);)

#endif
