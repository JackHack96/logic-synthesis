/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/atpg/fast_avl.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:17 $
 *
 */

typedef struct fast_avl_node_struct fast_avl_node;
struct fast_avl_node_struct {
    fast_avl_node *left, *right;
    char *key;
    char *value;
    int height;
    fast_avl_node *next;
};

typedef struct fast_avl_tree_struct fast_avl_tree;
struct fast_avl_tree_struct {
    fast_avl_node *root;
    int (*compar)();
    int num_entries;
    int modified;
    fast_avl_tree *next;
};

typedef struct fast_avl_nodelist_struct fast_avl_nodelist;
struct fast_avl_nodelist_struct {
    int size;
    fast_avl_node **arr;
    fast_avl_nodelist *next;
};

typedef struct fast_avl_generator_struct fast_avl_generator;
struct fast_avl_generator_struct {
    fast_avl_tree *tree;
    fast_avl_nodelist *nodelist;
    int count;
    fast_avl_generator *next;
};

#define AVL_FORWARD 	0
#define AVL_BACKWARD 	1


EXTERN fast_avl_tree *fast_avl_init_tree ARGS((int (*)()));
EXTERN int fast_avl_insert ARGS((fast_avl_tree *, char *, char *));
EXTERN int fast_avl_lookup ARGS((fast_avl_tree *, char *, char **));
EXTERN int fast_avl_numcmp ARGS((char *, char *));
EXTERN int fast_avl_gen ARGS((fast_avl_generator *, char **, char **));
EXTERN void fast_avl_foreach ARGS((fast_avl_tree *, void (*)(), int));
EXTERN void fast_avl_free_tree ARGS((fast_avl_tree *, void (*)(), void (*)()));
EXTERN void fast_avl_free_gen ARGS((fast_avl_generator *));
EXTERN fast_avl_generator *fast_avl_init_gen ARGS((fast_avl_tree *, int));
EXTERN void fast_avl_cleanup ARGS(());

#define fast_avl_count(tree) (tree)->num_entries

#define fast_avl_is_member(tree, key) fast_avl_lookup(tree, key, (char **) 0)

#define fast_avl_foreach_item(tree, gen, dir, key_p, value_p) 	\
    for(gen = fast_avl_init_gen(tree, dir); 			\
	    fast_avl_gen(gen, key_p, value_p) || (fast_avl_free_gen(gen),0);)

extern fast_avl_node *fast_avl_node_freelist;
extern fast_avl_tree *fast_avl_tree_freelist;
extern fast_avl_generator *fast_avl_generator_freelist;
extern fast_avl_nodelist *fast_avl_nodelist_freelist;

#define fast_avl_node_alloc(newobj) \
    if (fast_avl_node_freelist == NIL(fast_avl_node)) { \
	newobj = ALLOC(fast_avl_node, 1); \
    } else { \
	newobj = fast_avl_node_freelist; \
	fast_avl_node_freelist = fast_avl_node_freelist->next; \
    } \
    newobj->next = 0;

#define fast_avl_node_free(e) \
    if (e) {\
       e->next = fast_avl_node_freelist; \
       fast_avl_node_freelist = e; \
    }

#define fast_avl_tree_alloc(newobj) \
    if (fast_avl_tree_freelist == NIL(fast_avl_tree)) { \
	newobj = ALLOC(fast_avl_tree, 1); \
    } else { \
	newobj = fast_avl_tree_freelist; \
	fast_avl_tree_freelist = fast_avl_tree_freelist->next; \
    } \
    newobj->next = 0;

#define fast_avl_tree_free(e) \
    if (e) {\
       e->next = fast_avl_tree_freelist; \
       fast_avl_tree_freelist = e; \
    }

#define fast_avl_generator_alloc(newobj) \
    if (fast_avl_generator_freelist == NIL(fast_avl_generator)) { \
	newobj = ALLOC(fast_avl_generator, 1); \
    } else { \
	newobj = fast_avl_generator_freelist; \
	fast_avl_generator_freelist = fast_avl_generator_freelist->next; \
    } \
    newobj->next = 0;

#define fast_avl_generator_free(e) \
    if (e) {\
       e->next = fast_avl_generator_freelist; \
       fast_avl_generator_freelist = e; \
    }

#define fast_avl_nodelist_free(e) \
    if (e) {\
	e->next = fast_avl_nodelist_freelist; \
        fast_avl_nodelist_freelist = e; \
    }

