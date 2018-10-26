
#include <stdio.h>

#include "util.h"
#include "fast_avl.h"

fast_avl_node *fast_avl_node_freelist = NIL(fast_avl_node);
fast_avl_tree *fast_avl_tree_freelist = NIL(fast_avl_tree);
fast_avl_generator *fast_avl_generator_freelist = NIL(fast_avl_generator);
fast_avl_nodelist *fast_avl_nodelist_freelist = NIL(fast_avl_nodelist);

#define HEIGHT(node) (node == NIL(fast_avl_node) ? -1 : (node)->height)
#define BALANCE(node) (HEIGHT((node)->right) - HEIGHT((node)->left))

#define compute_height(node) {				\
    int x=HEIGHT(node->left), y=HEIGHT(node->right);	\
    (node)->height = MAX(x,y) + 1;			\
}

#define COMPARE(key, nodekey, compare)	 		\
    ((compare == fast_avl_numcmp) ? 				\
	(int) key - (int) nodekey : 			\
	(*compare)(key, nodekey))

#define STACK_SIZE	50

static fast_avl_node *new_node();
static void do_rebalance(); 
static rotate_left();
static rotate_right();
static int do_check_tree();

fast_avl_tree *
fast_avl_init_tree(compar)
int (*compar)();
{
    fast_avl_tree *tree;

    fast_avl_tree_alloc(tree);
    tree->root = NIL(fast_avl_node);
    tree->compar = compar;
    tree->num_entries = 0;
    return tree;
}

fast_avl_lookup(tree, key, value_p)
fast_avl_tree *tree;
char *key;
char **value_p;
{
    fast_avl_node *node;
    int (*compare)() = tree->compar, diff;

    node = tree->root;
    while (node != NIL(fast_avl_node)) {
	diff = COMPARE(key, node->key, compare);
	if (diff == 0) {
	    /* got a match */
	    if (value_p != NIL(char *)) *value_p = node->value;
	    return 1;
	}
	node = (diff < 0) ? node->left : node->right;
    }
    return 0;
}

fast_avl_insert(tree, key, value)
fast_avl_tree *tree;
char *key;
char *value;
{
    fast_avl_node **node_p, *node;
    int stack_n = 0;
    int (*compare)() = tree->compar;
    fast_avl_node **stack_nodep[STACK_SIZE];
    int diff, status;

    node_p = &tree->root;

    /* walk down the tree (saving the path); stop at insertion point */
    status = 0;
    while ((node = *node_p) != NIL(fast_avl_node)) {
	stack_nodep[stack_n++] = node_p;
	diff = COMPARE(key, node->key, compare);
	if (diff == 0) status = 1;
	node_p = (diff < 0) ? &node->left : &node->right;
    }

    /* insert the item and re-balance the tree */
    *node_p = new_node(key, value);
    do_rebalance(stack_nodep, stack_n);
    tree->num_entries++;
    tree->modified = 1;
    return status;
}

static void 
fast_avl_record_gen_forward(node, gen)
fast_avl_node *node;
fast_avl_generator *gen;
{
    if (node != NIL(fast_avl_node)) {
	fast_avl_record_gen_forward(node->left, gen);
	gen->nodelist->arr[gen->count++] = node;
	fast_avl_record_gen_forward(node->right, gen);
    }
}


static void 
fast_avl_record_gen_backward(node, gen)
fast_avl_node *node;
fast_avl_generator *gen;
{
    if (node != NIL(fast_avl_node)) {
	fast_avl_record_gen_backward(node->right, gen);
	gen->nodelist->arr[gen->count++] = node;
	fast_avl_record_gen_backward(node->left, gen);
    }
}

static fast_avl_nodelist *
fast_avl_nodelist_alloc(count)
int count;
{
    fast_avl_nodelist *obj, *new;

    if (fast_avl_nodelist_freelist == NIL(fast_avl_nodelist)) {
	new = ALLOC(fast_avl_nodelist, 1); 
	new->arr = ALLOC(fast_avl_node *, count); 
	new->size = count; 
    }
    else {
	if (count <= fast_avl_nodelist_freelist->size) { 
	    new = fast_avl_nodelist_freelist; 
	    fast_avl_nodelist_freelist = fast_avl_nodelist_freelist->next; 
	}
	else { 
	    obj = fast_avl_nodelist_freelist;
	    fast_avl_nodelist_freelist = fast_avl_nodelist_freelist->next;
	    FREE(obj->arr);
	    FREE(obj);
	    new = ALLOC(fast_avl_nodelist, 1); 
	    new->arr = ALLOC(fast_avl_node *, count); 
	    new->size = count; 
	}
    }
    new->next = NIL(fast_avl_nodelist); 
    return new;
}

fast_avl_generator *
fast_avl_init_gen(tree, dir)
fast_avl_tree *tree;
int dir;
{
    int count;
    fast_avl_generator *gen;

    /* what a hack */
    fast_avl_generator_alloc(gen);
    gen->tree = tree;
    count = avl_count(tree);
    gen->nodelist = fast_avl_nodelist_alloc(count);
    gen->count = 0;
    if (dir == AVL_FORWARD) {
	fast_avl_record_gen_forward(tree->root, gen);
    } else {
	fast_avl_record_gen_backward(tree->root, gen);
    }
    gen->count = 0;

    /* catch any attempt to modify the tree while we generate */
    tree->modified = 0;
    return gen;
}

fast_avl_gen(gen, key_p, value_p)
fast_avl_generator *gen;
char **key_p;
char **value_p;
{
    fast_avl_node *node;

    if (gen->count == gen->tree->num_entries) {
	return 0;
    } else {
	node = gen->nodelist->arr[gen->count++];
	if (key_p != NIL(char *)) *key_p = node->key;
	if (value_p != NIL(char *)) *value_p = node->value;
	return 1;
    }
}

void
fast_avl_free_gen(gen)
fast_avl_generator *gen;
{
    fast_avl_nodelist_free(gen->nodelist);
    fast_avl_generator_free(gen);
}

static void
do_rebalance(stack_nodep, stack_n)
fast_avl_node ***stack_nodep;
int stack_n;
{
    fast_avl_node **node_p, *node;
    int hl, hr;
    int height;

    /* work our way back up, re-balancing the tree */
    while (--stack_n >= 0) {
	node_p = stack_nodep[stack_n];
	node = *node_p;
	hl = HEIGHT(node->left);		/* watch for NIL */
	hr = HEIGHT(node->right);		/* watch for NIL */
	if ((hr - hl) < -1) {
	    rotate_right(node_p);
	} else if ((hr - hl) > 1) {
	    rotate_left(node_p);
	} else {
	    height = MAX(hl, hr) + 1;
	    if (height == node->height) break;
	    node->height = height;
	}
    }
}

static 
rotate_left(node_p)
fast_avl_node **node_p;
{
    fast_avl_node *old_root = *node_p, *new_root, *new_right;

    if (BALANCE(old_root->right) >= 0) {
	*node_p = new_root = old_root->right;
	old_root->right = new_root->left;
	new_root->left = old_root;
    } else {
	new_right = old_root->right;
	*node_p = new_root = new_right->left;
	old_root->right = new_root->left;
	new_right->left = new_root->right;
	new_root->right = new_right;
	new_root->left = old_root;
	compute_height(new_right);
    }
    compute_height(old_root);
    compute_height(new_root);
}

static 
rotate_right(node_p)
fast_avl_node **node_p;
{
    fast_avl_node *old_root = *node_p, *new_root, *new_left;

    if (BALANCE(old_root->left) <= 0) {
	*node_p = new_root = old_root->left;
	old_root->left = new_root->right;
	new_root->right = old_root;
    } else {
	new_left = old_root->left;
	*node_p = new_root = new_left->right;
	old_root->left = new_root->right;
	new_left->right = new_root->left;
	new_root->left = new_left;
	new_root->right = old_root;
	compute_height(new_left);
    }
    compute_height(old_root);
    compute_height(new_root);
}

static void 
fast_avl_walk_forward(node, func)
fast_avl_node *node;
void (*func)();
{
    if (node != NIL(fast_avl_node)) {
	fast_avl_walk_forward(node->left, func);
	(*func)(node->key, node->value);
	fast_avl_walk_forward(node->right, func);
    }
}

static void 
fast_avl_walk_backward(node, func)
fast_avl_node *node;
void (*func)();
{
    if (node != NIL(fast_avl_node)) {
	fast_avl_walk_backward(node->right, func);
	(*func)(node->key, node->value);
	fast_avl_walk_backward(node->left, func);
    }
}

void 
fast_avl_foreach(tree, func, direction)
fast_avl_tree *tree;
void (*func)();
int direction;
{
    if (direction == AVL_FORWARD) {
	fast_avl_walk_forward(tree->root, func);
    } else {
	fast_avl_walk_backward(tree->root, func);
    }
}

static void
free_entry(node, key_free, value_free)
fast_avl_node *node;
void (*key_free)();
void (*value_free)();
{
    if (node != NIL(fast_avl_node)) {
	free_entry(node->left, key_free, value_free);
	free_entry(node->right, key_free, value_free);
	if (key_free != 0) (*key_free)(node->key);
	if (value_free != 0) (*value_free)(node->value);
	fast_avl_node_free(node);
    }
}
    
void 
fast_avl_free_tree(tree, key_free, value_free)
fast_avl_tree *tree;
void (*key_free)();
void (*value_free)();
{
    free_entry(tree->root, key_free, value_free);
    fast_avl_tree_free(tree);
}

static fast_avl_node *
new_node(key, value)
char *key;
char *value;
{
    fast_avl_node *new;

    fast_avl_node_alloc(new);
    new->key = key;
    new->value = value;
    new->height = 0;
    new->left = new->right = NIL(fast_avl_node);
    return new;
}

int 
fast_avl_numcmp(x, y)
char *x, *y; 
{
    return (int) x - (int) y;
}

int
fast_avl_check_tree(tree)
fast_avl_tree *tree;
{
    int error = 0;
    (void) do_check_tree(tree->root, tree->compar, &error);
    return error;
}

static int
do_check_tree(node, compar, error)
fast_avl_node *node;
int (*compar)();
int *error;
{
    int l_height, r_height, comp_height, bal;
    
    if (node == NIL(fast_avl_node)) {
	return -1;
    }

    r_height = do_check_tree(node->right, compar, error);
    l_height = do_check_tree(node->left, compar, error);

    comp_height = MAX(l_height, r_height) + 1;
    bal = r_height - l_height;
    
    if (comp_height != node->height) {
	(void) printf("Bad height for 0x%08x: computed=%d stored=%d\n",
	    node, comp_height, node->height);
	++*error;
    }

    if (bal > 1 || bal < -1) {
	(void) printf("Out of balance at node 0x%08x, balance = %d\n", 
	    node, bal);
	++*error;
    }

    if (node->left != NIL(fast_avl_node) && 
		    (*compar)(node->left->key, node->key) > 0) {
	(void) printf("Bad ordering between 0x%08x and 0x%08x", 
	    node, node->left);
	++*error;
    }
    
    if (node->right != NIL(fast_avl_node) && 
		    (*compar)(node->key, node->right->key) > 0) {
	(void) printf("Bad ordering between 0x%08x and 0x%08x", 
	    node, node->right);
	++*error;
    }

    return comp_height;
}

void
fast_avl_cleanup()
{
    fast_avl_tree *t, *tnext;
    fast_avl_node *n, *nnext;
    fast_avl_nodelist *e, *enext;
    fast_avl_generator *g, *gnext;

    for (t = fast_avl_tree_freelist; t != NIL(fast_avl_tree); t = tnext) {
	tnext = t->next;
	FREE(t);
    }
    fast_avl_tree_freelist = NIL(fast_avl_tree);
    for (n = fast_avl_node_freelist; n != NIL(fast_avl_node); n = nnext) {
	nnext = n->next;
	FREE(n);
    }
    fast_avl_node_freelist = NIL(fast_avl_node);
    for (e = fast_avl_nodelist_freelist; e != NIL(fast_avl_nodelist); 
	 e = enext) {
	enext = e->next;
	FREE(e->arr);
	FREE(e);
    }
    fast_avl_nodelist_freelist = NIL(fast_avl_nodelist);
    for (g = fast_avl_generator_freelist; g != NIL(fast_avl_generator); 
	 g = gnext) {
	gnext = g->next;
	FREE(g);
    }
    fast_avl_generator_freelist = NIL(fast_avl_generator);
}
