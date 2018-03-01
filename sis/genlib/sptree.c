/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/genlib/sptree.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:30 $
 *
 */
/* file @(#)sptree.c	1.1                      */
/* last modified on 5/29/91 at 12:35:33   */
#include "genlib_int.h"


tree_node_t *
gl_alloc_leaf(name)
char *name;
{
    tree_node_t *node;

    node = gl_alloc_node(0);
    if (name != NIL(char)) {
	node->name = util_strsav(name);
    }
    return node;
}


tree_node_t *
gl_alloc_node(nsons)
int nsons;
{
    tree_node_t *node;

    node = ALLOC(tree_node_t, 1);
    node->nsons = nsons;
    node->type = OR_NODE;
    node->name = NIL(char);
    node->phase = 1;
    if (nsons > 0) {
	node->sons = ALLOC(tree_node_t *, nsons);
    } else {
	node->sons = NIL(tree_node_t *);
    }
    return node;
}


void
gl_free_node(node)
tree_node_t *node;
{
    if (node->sons != NIL(tree_node_t *)) FREE(node->sons);
    if (node->name != NIL(char)) FREE(node->name);
    FREE(node);
}

tree_node_t *
gl_dup_tree(tree)
tree_node_t *tree;
{
    int i;
    tree_node_t *node;

    node = gl_alloc_node((int) tree->nsons);
    node->type = tree->type;
    if (tree->name != NIL(char)) {
	node->name = util_strsav(tree->name);
    }
    for(i = 0; i < tree->nsons; i++) {
	node->sons[i] = gl_dup_tree(tree->sons[i]);
    }
    return node;
}


void 
gl_free_tree(tree)
tree_node_t *tree;
{
    int i;

    for(i = 0; i < tree->nsons; i++) {
	gl_free_tree(tree->sons[i]);
    }
    gl_free_node(tree);
}


tree_node_type_t
gl_reverse_type(type)
tree_node_type_t type;
{
    return (type == OR_NODE) ? AND_NODE : OR_NODE;
}


void
gl_dualize_node(tree)
tree_node_t *tree;
{
     tree->type = gl_reverse_type(tree->type);
}

void
gl_invert_tree(tree)
tree_node_t *tree;
{
    int i;

	if (tree->phase) {
		if (tree->nsons) {
			for(i = 0; i < tree->nsons; i++) {
				gl_invert_tree(tree->sons[i]);
			}
			gl_dualize_node(tree);
		}
		else {
			tree->phase = 0;
		}
	}
	else {
		tree->phase = 1;
	}
}

void
gl_dualize_tree(tree)
tree_node_t *tree;
{
    int i;

    for(i = 0; i < tree->nsons; i++) {
	gl_dualize_tree(tree->sons[i]);
    }
    gl_dualize_node(tree);
}


void
gl_make_well_formed(tree)
tree_node_t *tree;
{
    int i, j, new_nsons;
    tree_node_t **new_sons, *son;

	if (! tree->phase && tree->nsons) {
		tree->phase = 1;
		gl_invert_tree(tree);
	}
check_sons:
    for(i = 0; i < tree->nsons; i++) {
	son = tree->sons[i];
	if (! son->phase && son->nsons) {
		son->phase = 1;
		gl_invert_tree(son);
	}
	if (son->nsons == 0) {
	    son->type = gl_reverse_type(tree->type);

	/* his children are my children */
	} else if (tree->type == son->type) {
	    new_sons = ALLOC(tree_node_t *, tree->nsons + son->nsons - 1);
	    new_nsons = 0;
	    for(j = 0; j < i; j++) {
		new_sons[new_nsons++] = tree->sons[j];
	    }
	    for(j = 0; j < son->nsons; j++) {
		new_sons[new_nsons++] = son->sons[j];
	    }
	    for(j = i+1; j < tree->nsons; j++) {
		new_sons[new_nsons++] = tree->sons[j];
	    }
	    gl_free_node(son);
	    FREE(tree->sons);
	    tree->sons = new_sons;
	    tree->nsons = new_nsons;
	    goto check_sons;
	}
    }

    for(i = 0; i < tree->nsons; i++) {
	gl_make_well_formed(tree->sons[i]);
    }
}

void 
gl_compute_level(tree, s, p, level)
tree_node_t *tree;
int *s, *p, *level;
{
    int i, p1, s1, level1;

    if (tree->nsons == 0) {
	*s = 1;
	*p = 1;
	*level = 0;
    } else {
	*s = *p = *level = 0;
	for(i = 0; i < tree->nsons; i++) {
	    gl_compute_level(tree->sons[i], &s1, &p1, &level1);
	    if (tree->type == AND_NODE) {
		*s += s1;
		if (p1 > *p) *p = p1;
	    } else {
		*p += p1;
		if (s1 > *s) *s = s1;
	    }
	    if (level1 > *level) *level = level1;
	}
	*level += 1;
    }
}

void
gl_print_tree_recur(fp, tree, level)
FILE *fp;
tree_node_t *tree;
int level;
{
    int i;

    if (tree->nsons == 0) {
	if (! tree->phase) {
	    putc('!', fp);
	}
	(void) fputs(tree->name, fp);
    } else {
	if (tree->type == OR_NODE && level > 0) (void) putc('(', fp);
	for(i = 0; i < tree->nsons; i++) {
	    gl_print_tree_recur(fp, tree->sons[i], level+1);
	    if (tree->type == OR_NODE && i != tree->nsons - 1) {
		(void) putc('+', fp);
	    }
	}
	if (tree->type == OR_NODE && level > 0) (void) putc(')', fp);
    }
}


void
gl_print_tree(fp, tree)
FILE *fp;
tree_node_t *tree;
{
    gl_assign_leaf_names(tree);
    if (tree->phase || tree->nsons == 0) {
	gl_print_tree_recur(fp, tree, 0);
    } else {
	(void) putc('(', fp);
	gl_print_tree_recur(fp, tree, 0);
	(void) putc(')', fp);
	(void) putc('\'', fp);
    }
}

void
gl_print_tree_recur_algebraic(fp, tree, level)
FILE *fp;
tree_node_t *tree;
int level;
{
    int i;

    if (tree->nsons == 0) {
	if (! tree->phase) {
	    putc('!', fp);
	}
	(void) fputs(tree->name, fp);
    } else {
	if (tree->type == OR_NODE && level > 0) (void) putc('(', fp);
	for(i = 0; i < tree->nsons; i++) {
	    gl_print_tree_recur_algebraic(fp, tree->sons[i], level+1);
	    if (tree->type == OR_NODE && i != tree->nsons - 1) {
		(void) putc('+', fp);
	    }
	    if (tree->type == AND_NODE && i != tree->nsons - 1) {
		(void) putc('*', fp);
	    }
	}
	if (tree->type == OR_NODE && level > 0) (void) putc(')', fp);
    }
}


void
gl_print_tree_algebraic(fp, tree)
FILE *fp;
tree_node_t *tree;
{
    gl_assign_leaf_names(tree);
    if (tree->phase || tree->nsons == 0) {
	gl_print_tree_recur_algebraic(fp, tree, 0);
    } else {
	(void) putc('!', fp);
	(void) putc('(', fp);
	gl_print_tree_recur_algebraic(fp, tree, 0);
	(void) putc(')', fp);
    }
}

int 
gl_qsort_compare_tree(t1, t2)
tree_node_t **t1, **t2;
{
    return gl_compare_tree(*t1, *t2);
}


int 
gl_compare_tree(tree1, tree2)
tree_node_t *tree1, *tree2;
{
    int result, i;

    if (tree1->nsons != tree2->nsons) {
	return tree1->nsons - tree2->nsons;
    } else {
	for(i = 0; i < tree1->nsons; i++) {
	    if (result = gl_compare_tree(tree1->sons[i], tree2->sons[i])) {
		return result;
	    }
	}
    }

    return 0;
}


void 
gl_canonical_tree(tree)
tree_node_t *tree;
{
    int i;

    for(i = 0; i < tree->nsons; i++) {
	gl_canonical_tree(tree->sons[i]);
    }
    qsort((char *) tree->sons, (int) tree->nsons, 
			sizeof(tree_node_t *), gl_qsort_compare_tree);
}

static tree_node_t **g_forms;
static int g_num;


avl_tree *
gl_hash_init()
{
    return avl_init_table(gl_compare_tree);
}


/* ARGSUSED */
void 
gl_hash_save(tree, value)
char *tree;
char *value;
{
    g_forms[g_num++] = (tree_node_t *) tree;
}


int 
gl_hash_end(hash, forms)
avl_tree *hash;
tree_node_t ***forms;
{
    g_num = 0; 
    g_forms = *forms = ALLOC(tree_node_t *, avl_count(hash));
    avl_foreach(hash, gl_hash_save, AVL_FORWARD);
    avl_free_table(hash, (void(*)()) 0, (void(*)()) 0);
    return g_num;
}


int 
gl_hash_find_or_add(hash, tree)
avl_tree *hash;
tree_node_t *tree;
{
    int s, p, level;

    gl_canonical_tree(tree);
    gl_compute_level(tree, &s, &p, &level);
    tree->s = s;
    tree->p = p;
    tree->level = level;
    if (avl_lookup(hash, (char *) tree, NIL(char *))) {
	return 0;
    } else {
	(void) avl_insert(hash, (char *) tree, NIL(char));
	return 1;
    }
}

static void
gl_assign_node_names_recur(tree, count)
tree_node_t *tree;
int *count;
{
    int i;

    for(i = 0; i < tree->nsons; i++) {
	gl_assign_node_names_recur(tree->sons[i], count);
    }
    if (tree->name == NIL(char)) {
	tree->name = ALLOC(char, 20);
	(void) sprintf(tree->name, "_%d", (*count)++);
    }
}


void
gl_assign_node_names(tree)
tree_node_t *tree;
{
    int count = 0;

    gl_assign_node_names_recur(tree, &count);
}


static void
gl_assign_leaf_names_recur(tree, count)
tree_node_t *tree;
int *count;
{
    int i;

    if (tree->nsons == 0 && tree->name == NIL(char)) {
	tree->name = ALLOC(char, 5);
	tree->name[0] = "abcdefghijklmnopqrstuvwxyz"[(*count)++];
	tree->name[1] = '\0';
    } else {
	for(i = 0; i < tree->nsons; i++) {
	    gl_assign_leaf_names_recur(tree->sons[i], count);
	}
    }
}


void
gl_assign_leaf_names(tree)
tree_node_t *tree;
{
    int count = 0;

    gl_assign_leaf_names_recur(tree, &count);
}

static void
gl_get_unique_leaf_pointers_recur(tree, unique_names)
tree_node_t *tree;
st_table *unique_names;
{
    int i;
    char **slotp;

    if (tree->nsons == 0) {
	(void) st_find_or_add(unique_names, tree->name, &slotp);
	*slotp = (char *) tree;
    } else {
	for(i = 0; i < tree->nsons; i++) {
	    gl_get_unique_leaf_pointers_recur(tree->sons[i], unique_names);
	}
    }
}


int
gl_get_unique_leaf_pointers(tree, leafs)
tree_node_t *tree;
tree_node_t ***leafs;
{
    st_table *unique_names;
    st_generator *gen;
    char *key, *value;
    int nleafs = 0;

    unique_names = st_init_table(strcmp, st_strhash);
    gl_get_unique_leaf_pointers_recur(tree, unique_names);
    *leafs = ALLOC(tree_node_t *, st_count(unique_names));
    st_foreach_item(unique_names, gen, &key, &value) {
       (*leafs)[nleafs++] = (tree_node_t *) value;
    }
    st_free_table(unique_names);
    return nleafs;
}

gl_tree_dump(tree)
tree_node_t *tree;
{
    int i;

    if (tree->nsons == 0) {
	(void) printf("LEAF : %s %s-NODE\n", 
	    tree->name, tree->type == OR_NODE ? "OR" : "AND");
    } else {
	(void) printf("nsons=%d type=%s\n",
	    tree->nsons, tree->type == OR_NODE ? "OR" : "AND");
	for(i = 0; i < tree->nsons; i++) {
	    gl_tree_dump(tree->sons[i]);
	}
    }
}
