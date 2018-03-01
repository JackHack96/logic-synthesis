/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/genlib/nand.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:30 $
 *
 */
/* file @(#)nand.c	1.1                      */
/* last modified on 5/29/91 at 12:35:28   */
#include "util.h"		/* for ASSERT !! */
#include "genlib_int.h"

static void
set_node_type(tree, type)
tree_node_t *tree;
tree_node_type_t type;
{
    int i;

    if (tree != 0) {
	for(i = 0; i < tree->nsons; i++) {
	    set_node_type(tree->sons[i], type);
	}
	tree->phase = 1;
	tree->type = type;
    }
}

#define DUMP_TABLES


static void
gl_write_blif_tables(fp, tree, use_nor_gate)
register FILE *fp;
tree_node_t *tree;
int use_nor_gate;
{
    register int i;

    for(i = 0; i < tree->nsons; i++) {
	gl_write_blif_tables(fp, tree->sons[i], use_nor_gate);
    }

    if (tree->nsons != 0) {
	(void) fprintf(fp, ".names");
	for(i = 0; i < tree->nsons; i++) {
	    (void) putc(' ', fp);
	    (void) fputs(tree->sons[i]->name, fp);
	}
	(void) putc(' ', fp);
	(void) fputs(tree->name, fp);
	(void) putc('\n', fp);

#ifdef DUMP_TABLES
	if (use_nor_gate) {		/* in theory, tree->type == NOR_NODE */
	    for(i = 0; i < tree->nsons; i++) {
		(void) putc('0', fp);
	    }
	    (void) putc(' ', fp);
	    (void) putc('1', fp);
	    (void) putc('\n', fp);
	} else {
	    register int j;
	    for(i = 0; i < tree->nsons; i++) {
		for(j = 0; j < tree->nsons; j++) {
		    if (i == j) {
			(void) putc('0', fp);
		    } else {
			(void) putc('-', fp);
		    }
		}
		(void) putc(' ', fp);
		(void) putc('1', fp);
		(void) putc('\n', fp);
	    }
	}
#endif
    }
}


static int 
leaf_compare(t1, t2)
char **t1, **t2;
{
    tree_node_t *tree1 = *(tree_node_t **) t1;
    tree_node_t *tree2 = *(tree_node_t **) t2;

    return strcmp(tree1->name, tree2->name);
}


static void 
gl_write_blif_inputs(fp, tree, latch)
FILE *fp;
tree_node_t *tree;
latch_info_t *latch;    /* sequential support */
{
    int i, nleafs;
    tree_node_t **leafs;

    nleafs = gl_get_unique_leaf_pointers(tree, &leafs);
    qsort((char *) leafs, nleafs, sizeof(tree_node_t *), leaf_compare);
    for(i = 0; i < nleafs; i++) {

	/* seqential support */
        /* Don't count latch output as a PI */
	if (latch != NIL(latch_info_t)) {
	    if ( strcmp(leafs[i]->name, latch->out) == 0 ) continue;
        }
	(void) putc(' ', fp);
	(void) fputs(leafs[i]->name, fp);
    }
    FREE(leafs);
}


void
gl_write_blif(fp, tree, use_nor_gate, latch)
FILE *fp;
tree_node_t *tree;
int use_nor_gate;
latch_info_t *latch;    /* sequential support */
{
    if (tree->type == ZERO_NODE) {
	(void) fprintf(fp, ".outputs %s\n", tree->name);
	(void) fprintf(fp, ".names %s\n", tree->name);
    } else if (tree->type == ONE_NODE) {
	(void) fprintf(fp, ".outputs %s\n", tree->name);
	(void) fprintf(fp, ".names %s\n 1\n", tree->name);
    } else {
	(void) fprintf(fp, ".inputs");
	gl_write_blif_inputs(fp, tree, latch);
	(void) fprintf(fp, "\n");
	/* sequential support */
	/* Don't count latch input as a real PO */
	if (latch == NIL(latch_info_t)) {
	  (void) fprintf(fp, ".outputs %s\n", tree->name);
	}
	gl_write_blif_tables(fp, tree, use_nor_gate);
    }
}

/* 
 *  make all binary-trees with 'n' leaves
 *  this is contained inside a permutation generator which tries all
 *    possible permutations of the leaves; hence, we can take the
 *    given order of the leaves as fixed
 */

static array_t *
make_tree_recur(leafs, nleafs, level)
tree_node_t **leafs;
int nleafs;
int level;
{
    int i, j, k;
    array_t *forms, *forms1, *forms2;
    tree_node_t *node, *root, *son1, *son2;

    assert(nleafs > 0);

    forms = array_alloc(tree_node_t *, 0);

    if (nleafs == 1) {
	root = gl_dup_tree(leafs[0]);		/* tee-hee */
	array_insert_last(tree_node_t *, forms, root);

    } else {
	for(i = 1; i <= nleafs/2; i++) {
	    forms1 = make_tree_recur(leafs, i, level + 1);
	    forms2 = make_tree_recur(leafs + i, nleafs - i, level + 1);

	    for(j = 0; j < array_n(forms1); j++) {
		for(k = 0; k < array_n(forms2); k++) {
		    node = gl_alloc_node(2);
		    son1 = gl_dup_tree(array_fetch(tree_node_t *, forms1, j));
		    son2 = gl_dup_tree(array_fetch(tree_node_t *, forms2, k));
		    node->sons[0] = son1;
		    node->sons[1] = son2;

		    /* add extra inverter on all levels except top */
		    if (level > 0) {
			root = gl_alloc_node(1);
			root->sons[0] = node;
			array_insert_last(tree_node_t *, forms, root);
		    } else {
			array_insert_last(tree_node_t *, forms, node);
		    }
		}
	    }

	    /* cleanup */
	    for(j = 0; j < array_n(forms1); j++) {
		gl_free_tree(array_fetch(tree_node_t *, forms1, j));
	    }
	    for(k = 0; k < array_n(forms2); k++) {
		gl_free_tree(array_fetch(tree_node_t *, forms2, k));
	    }
	    array_free(forms1);
	    array_free(forms2);
	}
    }
    return forms;
}

static void
make_tree(list_in, n, state_in)
char **list_in;
int n;
char *state_in;
{
    tree_node_t *tree, **list = (tree_node_t **) list_in;
    avl_tree *hash_table = (avl_tree *) state_in;
    array_t *forms;
    int i;

    forms = make_tree_recur(list, n, 0);
    for(i = 0; i < array_n(forms); i++) {
	tree = array_fetch(tree_node_t *, forms, i);
	if (! gl_hash_find_or_add(hash_table, tree)) {
	    gl_free_tree(tree);
	}
    }
    array_free(forms);
}

static int 
gl_nand_gate_forms_recur(tree, list, use_nor_gate)
tree_node_t *tree;
tree_node_t ***list;
int use_nor_gate;
{
    int i, j, n, nlist[32], *indices, invert_input, all_sons_isomorphic;
    tree_node_t *node, **xlist[32], *temp_list[32];
    avl_tree *hash_table;
    combination_t *comb;

    if (tree->nsons == 0) {
	/*
	 *  only problem with leafs is watching the extra inverters ...
	 */
	invert_input = tree->phase == 0;
	if (((tree->type == OR_NODE) == use_nor_gate) != invert_input) {
	    node = gl_alloc_node(1);
	    node->sons[0] = gl_alloc_leaf(tree->name);
	} else {
	    node = gl_alloc_leaf(tree->name);
	}
	*list = ALLOC(tree_node_t *, 1);
	(*list)[0] = node;
	n = 1;

    } else {
	/*
	 *  First generate all forms for our children
	 */
	for(i = 0; i < tree->nsons; i++) {
	    nlist[i] = gl_nand_gate_forms_recur(tree->sons[i], &(xlist[i]),
				    use_nor_gate);
	}

	/*
	 *  then, try all possible 'combinations' of one form from each child
	 */
	hash_table = gl_hash_init();
	comb = gl_init_gen_combination(nlist, (int) tree->nsons);
	while (gl_next_combination(comb, &indices)) {
	    /*
	     *  then, try all possible permutations of the child forms; bottom
	     *  line is to call 'make_tree' to make all binary-trees with
	     *  n leafs 
	     */
	    for(i = 0; i < tree->nsons; i++) {
		temp_list[i] = xlist[i][indices[i]];
	    }

	    /* hack !!! check if all of our children are isomorphic -- if so,
	     * we can avoid 'permute' at this step; this makes the special
	     * case of a 6-wide or 8-wide nand gate go MUCH MUCH faster
	     * (8! = 40,320 ...); even 4-wide should be helped ...
	     */
	    all_sons_isomorphic = 1;
	    for(i = 1; i < tree->nsons; i++) {
		if (gl_compare_tree(temp_list[i-1], temp_list[i]) != 0) {
		    all_sons_isomorphic = 0;
		    break;
		}
	    }

	    if (all_sons_isomorphic) {
		make_tree((char **) temp_list, 
				    (int) tree->nsons, (char *) hash_table);
	    } else {
		gl_permute((char **) temp_list, (int) tree->nsons, 
				make_tree, (char *) hash_table);
	    }
	}
	gl_free_gen_combination(comb);

	for(i = 0; i < tree->nsons; i++) {
	    for(j = 0; j < nlist[i]; j++) {
		gl_free_tree(xlist[i][j]);
	    }
	    FREE(xlist[i]);
	}

	n = gl_hash_end(hash_table, list);
    }

    return n;
}

int 
gl_nand_gate_forms(tree, list, use_nor_gate)
tree_node_t *tree;
tree_node_t ***list;
int use_nor_gate;
{
    int i, n, invert_output;
    tree_node_t *node;

    /* special case inverter and buffer */
    if (tree->nsons == 0) {
	if (tree->phase == 1) {
	    node = gl_alloc_node(1);
	    node->sons[0] = gl_alloc_node(1);
	    node->sons[0]->sons[0] = gl_alloc_leaf(tree->name);
	} else {
	    node = gl_alloc_node(1);
	    node->sons[0] = gl_alloc_leaf(tree->name);
	}
	*list = ALLOC(tree_node_t *, 1);
	(*list)[0] = node;
	n = 1;
    } else {
	invert_output = tree->phase == 0;
	n = gl_nand_gate_forms_recur(tree, list, use_nor_gate);

	/* Check for extra inverter at root node */
	for(i = 0; i < n; i++) {
	    if (((tree->type == OR_NODE) != use_nor_gate) == invert_output) {
		node = gl_alloc_node(1);
		node->sons[0] = (*list)[i];
		(*list)[i] = node;
	    }
	}
    }

    /* set all internal types/phases */
    for(i = 0; i < n; i++) {
	set_node_type((*list)[i], use_nor_gate ? NOR_NODE : NAND_NODE);
    }

    return n;
}
