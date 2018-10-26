
#include "node_int.h"
#include "sis.h"

static void handle_constant();

/*
static void handle_inverter();
*/

int node_collapse(node_t *f, node_t *g) {
    node_t *p, *q, *r, *g_not, *t1, *t2, *t3;
    int    index;

    if (f->type == PRIMARY_INPUT || g->type == PRIMARY_INPUT) {
        return 0;
    }
    if (f->type == PRIMARY_OUTPUT || g->type == PRIMARY_OUTPUT) {
        return 0;
    }

    /* make sure f really depends on g */
    index = node_get_fanin_index(f, g);
    if (index == -1) {
        return 0;
    }

    /* there are some special cases -- for speed reasons only ... */
    switch (node_function(g)) {
        case NODE_0:handle_constant(f, index, ONE);
            return 1;

        case NODE_1:handle_constant(f, index, ZERO);
            return 1;

            /* these two cases work only when the nodes are in the network */
            /*
            case NODE_INV:
            handle_inverter(f, g, index, 0);
            return 1;

            case NODE_BUF:
            handle_inverter(f, g, index, 1);
            return 1;
            */
        default:break;
    }

    /* general case: form cofactors, and re-combine */
    node_algebraic_cofactor(f, g, &p, &q, &r);
    t1 = r;

    if (node_function(p) != NODE_0) {
        t2 = node_and(p, g);
        t3 = node_or(t1, t2);
        node_free(t1);
        node_free(t2);
        t1 = t3;
    }

    if (node_function(q) != NODE_0) {
        g_not = node_not(g);
        t2    = node_and(q, g_not);
        t3    = node_or(t1, t2);
        node_free(g_not);
        node_free(t1);
        node_free(t2);
        t1 = t3;
    }

    node_free(p);
    node_free(q);
    node_replace(f, t1);

    return 1;
}

static void handle_constant(node_t *f, register int index, register int bad_value) {
    register pset last, p;
    register int  index2, index2p1;
    int           delcnt;

    index2   = 2 * index;
    index2p1 = 2 * index + 1;
    delcnt   = 0;

    foreach_set(f->F, last, p) { SET(p, ACTIVE); }
    foreach_set(f->F, last, p) {
        if (GETINPUT(p, index) == bad_value) {
            RESET(p, ACTIVE); /* mark the cube for deletion */
            delcnt++;
        } else {
            set_insert(p, index2);
            set_insert(p, index2p1);
        }
    }
    if (delcnt > 0) {
        f->F = sf_inactive(f->F); /* delete cubes which are not active */
    }
    node_invalid(f);
    node_minimum_base(f);
}

/* works only if the node is in the network */
/* phase = 0 - handle buffer
 *         1 - handle inverter
 */
/*
static void
handle_inverter(f, g, index, phase)
node_t *f, *g;
register int index;
int phase;
{
    register pset last, p;
    register int index2, index2p1;

    if (phase == 0) {
        index2 = 2 * index;
        index2p1 = 2 * index + 1;

        foreach_set(f->F, last, p) {
            switch (GETINPUT(p, index)) {
            case ZERO:
                set_insert(p, index2p1);
                set_remove(p, index2);
                break;
            case ONE:
                set_insert(p, index2);
                set_remove(p, index2p1);
                break;
            }
        }
    }
    assert(node_patch_fanin(f, g, node_get_fanin(g, 0)));
    node_invalid(f);
    node_minimum_base(f);
}
*/
