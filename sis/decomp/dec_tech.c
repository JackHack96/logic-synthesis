
#include "sis.h"
#include "decomp.h"
#include "decomp_int.h"

static void 	     decomp_and_or();
static void 	     decomp_or();
static void 	     decomp_and();
static void          balanced_tree();

void
decomp_tech_network(network, and_limit, or_limit)
network_t *network;
int and_limit, or_limit;
{
    lsGen gen;
    node_t *node, *f;
    array_t *fa;
    int i;

    /* Check for a special case: f = a + a' will cause tech_decomp to */
    /* fail unless it is replaced by a constant. */
    foreach_node(network, gen, node) {
        if (node->type == INTERNAL) {
            if (node_function(node) == NODE_OR) {
                foreach_fanin(node, i, f) {
                    if (node_input_phase(node, f) == BINATE) {
                        node_replace(node, node_constant(1));
                        break;
                    }
                }
            }
        }
    }
    fa = network_dfs(network);
    for (i = array_n(fa) - 1; i >= 0; i--) {
	f = array_fetch(node_t *, fa, i);
	if (f->type == INTERNAL) {
	    decomp_and_or(network, f); 
	}
    }
    array_free(fa);

    fa = network_dfs(network);
    for (i = array_n(fa) - 1; i >= 0; i--) {
	f = array_fetch(node_t *, fa, i);
	if (f->type == INTERNAL) {
	    if (node_function(f) == NODE_OR && or_limit <= 0) {
		(void) node_invert(f);
	    } else if (node_function(f) == NODE_AND && and_limit <= 0) {
		(void) node_invert(f);
	    }
	}
    }

    array_free(fa);
    fa = network_dfs(network);
    for (i = array_n(fa) - 1; i >= 0; i--) {
	f = array_fetch(node_t *, fa, i);
	if (f->type == INTERNAL) {
	    switch (node_function(f)) {
	    case NODE_AND:
		decomp_and(network, f, and_limit);
		break;
	    case NODE_OR:
		decomp_or(network, f, or_limit);
		break;
	    default:
		;
	    }
	}
    }
    array_free(fa);

    (void) network_sweep(network);
}

static void
decomp_and_or(network, f)
network_t *network;
node_t *f;
{
    node_t *g, *t, *clit, *c;
    int i;

    if (node_function(f) != NODE_COMPLEX) {
	return;
    }

    g = node_constant(0);
    for (i = node_num_cube(f)-1; i>= 0; i--) {
	c = dec_node_cube(f, i);
	network_add_node(network, c);
	clit = node_literal(c, 1);
	t = node_or(g, clit);
	node_free(clit); 
	node_free(g);
	g = t;
    }
    node_replace(f, g);
}

static void
decomp_and(network, f, and_limit)
network_t *network;
node_t *f;
int and_limit;
{
    array_t *nodes, *leaves;
    node_t *root, *leaf, *fanin, *node;
    int i;

    if (and_limit <= 0) {
	fail("Error: wrong fanin limit for AND gate"); 
    }

    if (node_function(f) != NODE_AND) {
	fail("Error: function type is not AND in decomp_and");
    }

balanced_tree(node_num_fanin(f), and_limit, node_and, &root, &nodes, &leaves);

    /* connect the leaves */
    for (i = array_n(leaves)-1; i >= 0; i--) {
	leaf = array_fetch(node_t *, leaves, i);
	fanin = node_get_fanin(f, i);
	switch (node_input_phase(f, node_get_fanin(f, i))) {
	case POS_UNATE:
	    node_replace(leaf, node_literal(fanin, 1)); 
	    break;
	case NEG_UNATE:
	    node_replace(leaf, node_literal(fanin, 0)); 
	    break;
	default:
	    fail("Error: wrong phase of a input in decomp_and");
	}
    }

    /* replace f */
    for (i = array_n(nodes)-1; i >= 0; i--) {
	node = array_fetch(node_t *, nodes, i);
	if (node == root) {
	    node_replace(f, node);
	} else {
	    network_add_node(network, node);
	}
    }

    array_free(nodes);
    array_free(leaves);
}

static void
decomp_or(network, f, or_limit)
network_t *network;
node_t *f;
int or_limit;
{
    array_t *nodes, *leaves;
    node_t *root, *leaf, *fanin, *node;
    int i;

    if (or_limit <= 0) {
	fail("Error: wrong fanin limit for OR gate"); 
    }

    if (node_function(f) != NODE_OR) {
	fail("Error: function type is not OR in decomp_or");
    }

balanced_tree(node_num_fanin(f), or_limit, node_or, &root, &nodes, &leaves);

    /* connect the leaves */
    for (i = array_n(leaves)-1; i >= 0; i--) {
	leaf = array_fetch(node_t *, leaves, i);
	fanin = node_get_fanin(f, i);
	switch (node_input_phase(f, node_get_fanin(f, i))) {
	case POS_UNATE:
	    node_replace(leaf, node_literal(fanin, 1)); 
	    break;
	case NEG_UNATE:
	    node_replace(leaf, node_literal(fanin, 0)); 
	    break;
	default:
	    fail("Error: wrong phase of a input in decomp_or");
	}
    }

    /* replace f */
    for (i = array_n(nodes)-1; i >= 0; i--) {
	node = array_fetch(node_t *, nodes, i);
	if (node == root) {
	    node_replace(f, node);
	} else {
	    network_add_node(network, node);
	}
    }

    array_free(nodes);
    array_free(leaves);
}

static void
balanced_tree(n, m, node_func, root, nodes, leaves)
int n;		    		/* number of leaves */
int m;		    		/* branching factor */
node_t *(*node_func)();		/* function at a node */
node_t **root;			/* points to the root of the tree */
array_t **nodes;		/* points to all nodes */
array_t **leaves;		/* points to the leaves */ 
{
    node_t *np, *rt;
    array_t *ns, *lv;
    int d, s;
    bool first;

    if (n == 1) {
	*root = node_alloc();
	*nodes = array_alloc(node_t *, 0);
	array_insert_last(node_t *, *nodes, *root);
	*leaves = array_alloc(node_t *, 0);
	array_insert_last(node_t *, *leaves, *root);
    } else {
	first = TRUE;
	d = (n + m - 1) / m;
	while (n > 0) {
	    s = MIN(d, n);
	    n = n - d;
	    balanced_tree(s, m, node_func, &rt, &ns, &lv);
	    if (first) {
		*root = node_literal(rt, 1);
		*nodes = array_alloc(node_t *, 0);
		array_insert_last(node_t *, *nodes, *root);
		array_append((*nodes), ns);
		array_free(ns);
		*leaves = lv;
		first = FALSE;
	    } else {
		np = node_literal(rt, 1);
		node_replace(*root, (*node_func)(*root, np));
		node_free(np);
		array_append((*nodes), ns);
		array_append((*leaves), lv);
		array_free(ns);
		array_free(lv);
	    }
	}
    }
}
