/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/factor/ft_util.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:23 $
 *
 */
/*
 *  factor_util: utilities to handle factor tree in which each
 *               node is ft_t.
 */

#include "sis.h"
#include "factor.h"
#include "factor_int.h"

static ft_t 	*ft_dup();
static ft_t 	*ft_new();
static void 	ft_free();
static ft_t 	*node_to_ft();

void
factor_alloc(f)
node_t *f;
{
    f->factored = NIL(char);
}

void
factor_free(f)
node_t *f;
{
    if (f->factored != NIL(char)) {
	ft_free((ft_t *) f->factored);
	f->factored = NIL(char);
    }
}

void
factor_invalid(f)
node_t *f;
{
    factor_free(f);
}

void
factor_dup(old, new)
node_t *old, *new;
{
    factor_free(new);
    new->factored = (char *) ft_dup((ft_t *) old->factored);
}

static void
ft_free(root)
ft_t *root;
{
    if (root != NIL(ft_t)) {
	ft_free(root->next_level);
	ft_free(root->same_level);
	FREE(root);
    }
}

static ft_t *
ft_dup(f)
ft_t *f;
{
    ft_t *newp;

    if (f == NIL(ft_t)) {
	return f;
    } else {
	newp = ALLOC(ft_t, 1);
	newp->type = f->type;
	newp->index = f->index;
	newp->len = f->len;
	newp->next_level = ft_dup(f->next_level);
	newp->same_level = ft_dup(f->same_level);
	return newp;
    }
}

static ft_t *
ft_new(ft_type, ft_index, ft_len, ft_same, ft_next)
factor_type ft_type;
int ft_index;
int ft_len;
ft_t *ft_same;
ft_t *ft_next;
{
    ft_t *p;
    p = ALLOC(ft_t, 1);
    p->type = ft_type; 
    p->index = ft_index; 
    p->len = ft_len;
    p->same_level = ft_same; 
    p->next_level = ft_next;
    return p;
}

void
factor_traverse(f, func, stat, order)
node_t *f;
int (*func)();
char *stat;
trav_order order;
{
    factor(f);
    (void) ft_traverse_recur(f, (ft_t *) f->factored, func, stat, order);
}

int
ft_traverse_recur(f, root, func, stat, order)
node_t *f;
ft_t *root;
int (*func)();
char *stat;
trav_order order;
{
    if (order == FACTOR_TRAV_IN_ORDER) {
	if ((*func)(f, root, stat) == 1) {
	    return 1;
	}
    }

    if (root->next_level != NIL(ft_t)) {
	if (ft_traverse_recur(f, root->next_level, func, stat, order) != 0) {
	    return 1;
	}
    }

    if (root->same_level != NIL(ft_t)) {
	if (ft_traverse_recur(f, root->same_level, func, stat, order) != 0) {
	    return 1;
	}
    }

    if (order == FACTOR_TRAV_POST_ORDER) {
	if ((*func)(f, root, stat) == 1) {
	    return 1;
	}
    }

    return 0;
}

/*
 *  nt2ft: convert node_t tree to ft_t tree.
 */
ft_t *
factor_nt_to_ft(f, nt)
node_t *f;		/* original node */
node_t *nt;		/* root of the factored node_t tree */
{
    switch (node_function(nt)) {
    case NODE_0:
	return ft_new(FACTOR_0, -1, 0, NIL(ft_t), NIL(ft_t));
    case NODE_1:
	return ft_new(FACTOR_1, -1, 0, NIL(ft_t), NIL(ft_t));
    default:
	return node_to_ft(f, nt);
    }
}

static ft_t *
node_to_ft(f, np)
node_t *f;
node_t *np;
{
    node_cube_t cube;
    node_literal_t literal;
    node_t *fanin;
    ft_t *lit, *and, *or, *temp, *cur_and, *cur_or;
    int i, j;

    if (np->type != UNASSIGNED) {
	i = node_get_fanin_index(f, np);
	return ft_new(FACTOR_LEAF, i, 0, NIL(ft_t), NIL(ft_t));
    }

    or = cur_or = NIL(ft_t);
    for (i = node_num_cube(np) - 1; i >= 0; i--) {
	and = cur_and = NIL(ft_t);
	cube = node_get_cube(np, i);
	foreach_fanin(np, j, fanin) {
	    literal = node_get_literal(cube, j);
	    switch(literal) {
	    case ZERO:
		temp = node_to_ft(f, fanin);
		lit = ft_new(FACTOR_INV, -1, 0, NIL(ft_t), temp);
		if (cur_and == NIL(ft_t)) {
		    and = cur_and = lit;
		} else {
		    cur_and->same_level = lit;
		    cur_and = lit;
		}
		break;
	    case ONE:
		lit = node_to_ft(f, fanin);
		if (cur_and == NIL(ft_t)) {
		    and = cur_and = lit;
		} else {
		    cur_and->same_level = lit;
		    cur_and = lit;
		}
	    }
	}

	if (and == NIL(ft_t)) {	 /* no literal case, should never happen */
	    fail("Error: function is not SCC minimum");
	} else if (and->same_level != NIL(ft_t)) {	/* 1 literal case */
	    and = ft_new(FACTOR_AND, -1, 0, NIL(ft_t), and); 
	}

	if (cur_or == NIL(ft_t)) {
	    or = cur_or = and;
	} else {
	    cur_or->same_level = and;
	    cur_or = and;
	}
    }

    if (or == NIL(ft_t)) {  /* no cube case, should never happen */
	fail("Error: function is not SCC minimum");
    } else if (or->same_level != NIL(ft_t)) { 		/* 1 cube case */
	or = ft_new(FACTOR_OR, -1, 0, NIL(ft_t), or);
    }

    return or;
}

void
factor_nt_free(nodep)
node_t *nodep;
{
    int i;
    node_t *np;

    if (nodep->type == UNASSIGNED) {
	foreach_fanin(nodep, i, np) {
	    factor_nt_free(np);
	}
	node_free(nodep);
    }
}

/*
 *  find the best literal of c in f. 
 */
node_t *
factor_best_literal(f, c)
node_t *f, *c;
{
    node_t *bp = NIL(node_t), *fip, *cip;
    int *f_count, *c_count;
    int b_count = -1;
    int ci, fi, phase = 1;

    f_count = node_literal_count(f);
    c_count = node_literal_count(c);

    foreach_fanin(f, fi, fip) {
	foreach_fanin(c, ci, cip) {
	    if (fip == cip) {
		if (c_count[2*ci] > 0 && f_count[2*fi] > b_count) {
		    bp = cip;
		    phase = 1;		/* ???????? */
		    b_count = f_count[2*fi];
		} 
		if (c_count[2*ci+1] > 0 && f_count[2*fi+1] > b_count) {
		    bp = cip;
		    phase = 0;		/* ???????? */
		    b_count = f_count[2*fi+1];
		}
	    }
	}
    }

    FREE(f_count);
    FREE(c_count);

    switch (phase) {
    case 0:
	return node_literal(bp, 0);
    case 1:
	return node_literal(bp, 1);
    default:
	(void) fprintf(miserr, "Error: internal error in best_literal\n");
	return NIL(node_t);
    }
}

node_t *
factor_quick_kernel(f)
node_t *f;
{
    return ex_find_divisor_quick(f);
}

node_t *
factor_best_kernel(f)
node_t *f;
{
    return ex_find_divisor(f, 1, 1);
}

static array_t *ft_conv();
static array_t *ft_and_conv();
static array_t *ft_or_conv();
static array_t *ft_inv_conv();

array_t *
factor_to_nodes(f)
node_t *f;
{
    node_t *r;
    array_t *fa;

    factor(f);
    if (node_function(f) == NODE_BUF) {
	fa = array_alloc(node_t *, 0);
	array_insert_last(node_t *, fa, node_literal(node_get_fanin(f, 0), 1));
    } else {
	fa = ft_conv(f, (ft_t *) f->factored, &r);
    }

    return fa;
}

static array_t *
ft_conv(nf, f, root)
node_t *nf;
ft_t *f;
node_t **root;
{
    array_t *fa;

    switch (f->type) {
    case FACTOR_0:
	fa = array_alloc(node_t *, 0);
	*root = node_constant(0); 
	array_insert_last(node_t *, fa, *root);
	break;
    case FACTOR_1:
	fa = array_alloc(node_t *, 0);
	*root = node_constant(1); 
	array_insert_last(node_t *, fa, *root);
	break;
    case FACTOR_AND:
	fa = ft_and_conv(nf, f, root);
	break;
    case FACTOR_OR:
	fa = ft_or_conv(nf, f, root);
	break;
    case FACTOR_INV:
	fa = ft_inv_conv(nf, f, root);
	break;
    case FACTOR_LEAF:
	fa = array_alloc(node_t *, 0);
	*root = node_get_fanin(nf, f->index);
	break;
    default:
	fail("factor_to_nodes: wrong node type");
	exit(-1);
    }

    return fa;
}

static array_t *
ft_or_conv(nf, f, root)
node_t *nf;
ft_t *f;
node_t **root;
{
    array_t *fa, *tfa;
    node_t *or, *lit, *temp, *r;
    ft_t *p;

    or = node_constant(0);
    fa = array_alloc(node_t *, 0);
    array_insert_last(node_t *, fa, or);

    for (p = f->next_level; p != NIL(ft_t); p = p->same_level) {
	tfa = ft_conv(nf, p, &r);
	lit = node_literal(r, 1);
	temp = node_or(or, lit);
	node_free(or);
	node_free(lit);
	or = temp;
	array_append(fa, tfa);
	array_free(tfa);
    }

    array_insert(node_t *, fa, 0, or);
    *root = or;
    return fa;
}

static array_t *
ft_and_conv(nf, f, root)
node_t *nf;
ft_t *f;
node_t **root;
{
    array_t *fa, *tfa;
    node_t *and, *lit, *temp, *r;
    ft_t *p;

    and = node_constant(1);
    fa = array_alloc(node_t *, 0);
    array_insert_last(node_t *, fa, and);

    for (p = f->next_level; p != NIL(ft_t); p = p->same_level) {
	tfa = ft_conv(nf, p, &r);
	lit = node_literal(r, 1);
	temp = node_and(and, lit);
	node_free(and);
	node_free(lit);
	and = temp;
	array_append(fa, tfa);
	array_free(tfa);
    }

    array_insert(node_t *, fa, 0, and);
    *root = and;
    return fa;
}

static array_t *
ft_inv_conv(nf, f, root)
node_t *nf;
ft_t *f;
node_t **root;
{
    array_t *fa, *t;
    node_t *r;

    fa = array_alloc(node_t *, 0);
    t = ft_conv(nf, f->next_level, &r);
    *root = node_literal(r, 0); 
    array_insert_last(node_t *, fa, *root);
    array_append(fa, t);
    array_free(t);

    return fa;
}
