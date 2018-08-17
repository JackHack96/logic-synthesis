
/*
 * routines to compute the values
 */

#include "sis.h"
#include "../include/factor.h"
#include "../include/factor_int.h"

static int literal_sum();

static int factor_count();

/*
 *  Definition:
 *     The value of a node is defined to be the number of literals
 *     saved by the existence of the node.
 *  Formular:
 *     value = num_used * num_lit - num_used - num_lit
 *  where
 *     num_used is the number of times the node is used in both the positive
 *              and negative phase.
 *     num_lit is the number of literals in the factored form of the node.
 */
int node_value(nodep)
        node_t *nodep;
{
    bool   is_primary_output = FALSE;
    int    num_used;    /* number of time the function used in factored form */
    int    num_lit;    /* number of literals in the factored form */
    node_t *np;
    int    value;
    lsGen  gen;

    if (nodep->type == PRIMARY_INPUT || nodep->type == PRIMARY_OUTPUT) {
        return INFINITY;
    }

    /* if all outputs of this nodes are primary output, its value is oo */
    value = INFINITY;
    foreach_fanout(nodep, gen, np)
    {
        if (nodep->type != PRIMARY_OUTPUT) {
            value = 0;
        }
    }
    if (value != 0) {
        return value;
    }

    /* compute the number of times the function is used */
    num_used = 0;
    foreach_fanout(nodep, gen, np)
    {
        if (np->type != PRIMARY_OUTPUT) {
            num_used += factor_num_used(np, nodep);
        } else {
            is_primary_output = TRUE;
        }
    }

    num_lit = factor_num_literal(nodep);

    value = num_used * num_lit - num_used - num_lit;

    /* 
     * if the node fans out to a primary output, it cannot be eliminated.
     * So, add num_lit to the value.
     */
    if (is_primary_output) {
        value += num_lit;
    }

    return value;
}

typedef struct stat_struct {
    int index;
    int count;
} stat_t;

int
factor_num_used(o, i)
        node_t *o, *i;
{
    stat_t s;

    factor(o);
    s.index = node_get_fanin_index(o, i);
    s.count = 0;
    factor_traverse(o, factor_count, (char *) &s, FACTOR_TRAV_IN_ORDER);
    return s.count;
}

/* ARGSUSED */
static int
factor_count(r, f, stat)
        node_t *r;
        ft_t *f;
        char *stat;
{
    stat_t *s;

    s = (stat_t *) stat;
    if (f->type == FACTOR_LEAF && f->index == s->index) {
        s->count += 1;
    }
    return 0;
}

int
factor_num_literal(f)
        node_t *f;
{
    int sum = 0;

    if (f->type == PRIMARY_INPUT || f->type == PRIMARY_OUTPUT) {
        return 0;
    }

    factor(f);
    factor_traverse(f, literal_sum, (char *) &sum, FACTOR_TRAV_IN_ORDER);
    return sum;
}

/* ARGSUSED */
static int
literal_sum(r, np, sum)
        node_t *r;
        ft_t *np;
        char *sum;
{
    int *s;

    s = (int *) sum;
    if (np->type == FACTOR_LEAF) {
        *s += 1;
    }
    return 0;
}

void
value_print(fp, np)
        FILE *fp;
        node_t *np;
{
    int value;

    value = node_value(np);
    if (value >= INFINITY) {
        (void) fprintf(fp, "%s:\t(inf)\n", node_name(np));
    } else {
        (void) fprintf(fp, "%s:\t%d\n", node_name(np), value);
    }
}

int
value_cmp_inc(p1, p2)
        char **p1, **p2;
{
    return node_value((node_t * ) * p1) - node_value((node_t * ) * p2);
}

int
value_cmp_dec(p1, p2)
        char **p1, **p2;
{
    return node_value((node_t * ) * p2) - node_value((node_t * ) * p1);
}
