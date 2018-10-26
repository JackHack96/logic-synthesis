
#include "node_int.h"
#include "sis.h"

static int divide_one_phase();

int node_substitute(node_t *f, node_t *g, int use_complement) {
    int    changed;
    node_t *r, *r1;

    if (f->type == PRIMARY_INPUT || g->type == PRIMARY_INPUT) {
        return 0;
    }
    if (f->type == PRIMARY_OUTPUT || g->type == PRIMARY_OUTPUT) {
        return 0;
    }
    if (f == g) {
        return 0;
    }
    if (!node_has_function(f) || !node_has_function(g)) {
        fail("node_substitute: node does not have a function");
    }

    /* check that support of f contains support of g */
    if (!node_base_contain(f, g)) {
        return 0;
    }

    changed = 0;

    if (divide_one_phase(f, g, &r, 1)) {
        changed = 1;
    }
    if (use_complement) {
        if (divide_one_phase(r, g, &r1, 0)) {
            changed = 1;
        }
        node_free(r);
        r = r1;
    }

    if (changed) {
        node_replace(f, r);
    } else {
        node_free(r);
    }
    return changed;
}

static int divide_one_phase(node_t *f, node_t *g, node_t **out, int phase) {
    int    changed;
    node_t *q, *divisor, *lit, *t0, *t1, *r;

    changed = 0;

    if (phase == 0) {
        divisor = node_not(g);
        q       = node_div(f, divisor, &r);
        node_free(divisor);
    } else {
        q = node_div(f, g, &r);
    }

    /* set t1 = (F / G) * g + R */
    if (node_function(q) != NODE_0) {
        changed = 1;
        lit     = node_literal(g, phase);
        t0      = node_and(q, lit);
        t1      = node_or(t0, r);
        node_free(lit);
        node_free(t0);
        node_free(r);
    } else {
        t1 = r;
    }
    node_free(q);

    *out = t1;
    return changed;
}
