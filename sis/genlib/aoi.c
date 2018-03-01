/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/genlib/aoi.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:30 $
 *
 */
/* file @(#)aoi.c	1.1                      */
/* last modified on 5/29/91 at 12:35:18   */
#include "genlib_int.h"


static int 
s_sort(x, y)
char *x, *y;
{
    tree_node_t *tree1 = *(tree_node_t **) x, *tree2 = *(tree_node_t **) y;

    return tree1->s - tree2->s;
}


static int 
p_sort(x, y)
char *x, *y;
{
    tree_node_t *tree1 = *(tree_node_t **) x, *tree2 = *(tree_node_t **) y;

    return tree1->p - tree2->p;
}

int
gl_gen_complex_gates(level, s, p, root_type, forms)
int level, s, p;
tree_node_type_t root_type;
tree_node_t ***forms;
{
    int i, j, k, *value, num1, nforms, *nforms1; 
    tree_node_t *tree, **forms1;
    partition_t *part;
    combination_t *unique;
    avl_tree *hash;

    /* There is only a single level-0 form (or single form for s=1,p=1) */
    if (level <= 0 || (s <= 1 && p <= 1)) {
	*forms = ALLOC(tree_node_t *, 1);
	tree = gl_alloc_leaf(NIL(char));
	tree->type = root_type;
	tree->s = tree->p = 1;
	tree->level = 0;
	(*forms)[0] = tree;
	return 1;
    }

    /* setup a table to identify unique forms */
    hash = gl_hash_init();

    /* generate the single level-0 form */
    tree = gl_alloc_leaf(NIL(char));
    tree->type = root_type;
    (void) gl_hash_find_or_add(hash, tree);

    /* Loop for all partitions of either 's' or 'p'  */
    if (root_type == AND_NODE) {
	part = gl_init_gen_partition(s);
    } else {
	part = gl_init_gen_partition(p);
    }

    while (gl_next_partition(part, &value, &k)) {

	/* Recursively generate all forms for the largest partition value */
	if (root_type == AND_NODE) {
	    num1 = gl_gen_complex_gates(level-1, part->value[0], p, 
					OR_NODE, &forms1);
	} else {
	    num1 = gl_gen_complex_gates(level-1, s, part->value[0], 
					AND_NODE, &forms1);
	}

	/* Sort the sub-trees by 's' or 'p' */
	if (root_type == AND_NODE) {
	    qsort((char *) forms1, num1, sizeof(tree_node_t *), s_sort);
	    nforms1 = ALLOC(int, k);
	    for(j = 0; j < k; j++) {
		nforms1[j] = 0;
	    }
	    for(i = 0; i < num1; i++) {
		for(j = 0; j < k; j++) {
		    if (forms1[i]->s <= part->value[j]) {
			nforms1[j]++;
		    }
		}
	    }
	} else {
	    qsort((char *) forms1, num1, sizeof(tree_node_t *), p_sort);
	    nforms1 = ALLOC(int, k);
	    for(j = 0; j < k; j++) {
		nforms1[j] = 0;
	    }
	    for(i = 0; i < num1; i++) {
		for(j = 0; j < k; j++) {
		    if (forms1[i]->p <= part->value[j]) {
			nforms1[j]++;
		    }
		}
	    }
	}


	unique = gl_init_gen_combination(nforms1, k);
	while (gl_next_nonincreasing_combination(unique, &value)) {
	    tree = gl_alloc_node(k);
	    tree->type = root_type;
	    for(i = 0; i < k; i++) {
		tree->sons[i] = gl_dup_tree(forms1[value[i]]);
	    }

	    /* add tree to output only if it is unique */
	    /* note that 'hash_find_or_add also sets the (s,p) values */
	    if (! gl_hash_find_or_add(hash, tree)) {
		/* is not a new tree, discard it */
		gl_free_tree(tree);
	    }
	}
	gl_free_gen_combination(unique);

	for(i = 0; i < num1; i++) {
	    gl_free_tree(forms1[i]);
	}
	FREE(forms1);
	FREE(nforms1);

    }
    gl_free_gen_partition(part);

    nforms = gl_hash_end(hash, forms);
    return nforms;
}

int 
gl_generate_complex_gates(level, s, p, forms)
int level, s, p;
tree_node_t ***forms;
{
    int num1, num2, i;
    tree_node_t **forms1, **forms2;

    num1 = gl_gen_complex_gates(level, s, p, AND_NODE, &forms1);
    num2 = gl_gen_complex_gates(level, s, p, OR_NODE, &forms2);
    *forms = ALLOC(tree_node_t *, num1 + num2);
    for(i = 0; i < num1; i++) {
	(*forms)[i] = forms1[i];
    }

    /* hack -- drop the inverter from the OR-node trees */
    for(i = 1; i < num2; i++) {
	(*forms)[num1 + i - 1] = forms2[i];
    }
    FREE(forms1);
    FREE(forms2);

    return num1 + num2 - 1;
}
