
#include "sis.h"
#include "../include/node_int.h"

void
node_algebraic_cofactor(node, fanin, p, q, r)
        node_t *node, *fanin;
        node_t **p, **q, **r;
{
    register int  i;
    register pset last, p1, p2;
    pset_family   p_sf, q_sf, r_sf;

    p_sf = sf_new(node->F->count, node->nin * 2);
    q_sf = sf_new(node->F->count, node->nin * 2);
    r_sf = sf_new(node->F->count, node->nin * 2);

    i = node_get_fanin_index(node, fanin);
    if (i == -1) {
        sf_free(r_sf);
        r_sf = sf_save(node->F);
    } else {
        foreach_set(node->F, last, p1)
        {
            switch (GETINPUT(p1, i)) {
                case ONE: p2 = GETSET(p_sf, p_sf->count++);
                    INLINEset_copy(p2, p1);
                    set_insert(p2, 2 * i);
                    break;
                case ZERO: p2 = GETSET(q_sf, q_sf->count++);
                    INLINEset_copy(p2, p1);
                    set_insert(p2, 2 * i + 1);
                    break;
                case TWO: p2 = GETSET(r_sf, r_sf->count++);
                    INLINEset_copy(p2, p1);
                    break;
                default: fail("node_algebraic_cofactor: bad cube");
                    break;
            }
        }
    }

    *p = node_create(p_sf, nodevec_dup(node->fanin, node->nin), node->nin);
    *q = node_create(q_sf, nodevec_dup(node->fanin, node->nin), node->nin);
    *r = node_create(r_sf, nodevec_dup(node->fanin, node->nin), node->nin);
    (*p)->is_dup_free = node->is_dup_free;
    (*q)->is_dup_free = node->is_dup_free;
    (*r)->is_dup_free = node->is_dup_free;

    node_minimum_base(*p);
    node_minimum_base(*q);
    node_minimum_base(*r);
}

static node_t *
fast_cofactor(node, fanin, phase)
        node_t *node, *fanin;
        int phase;
{
    register int  i;
    register pset last, pdest, p;
    pset_family   func;
    node_t        *r;

    i = node_get_fanin_index(node, fanin);
    if (i == -1) {
        func = sf_save(node->F);

    } else {
        func = sf_new(node->F->count, node->nin * 2);
        foreach_set(node->F, last, p)
        {
            switch (GETINPUT(p, i)) {
                case ONE:
                    if (phase == 1) {
                        pdest = GETSET(func, func->count++);
                        INLINEset_copy(pdest, p);
                        set_insert(pdest, 2 * i);
                    }
                    break;
                case ZERO:
                    if (phase == 0) {
                        pdest = GETSET(func, func->count++);
                        INLINEset_copy(pdest, p);
                        set_insert(pdest, 2 * i + 1);
                    }
                    break;
                case TWO: pdest = GETSET(func, func->count++);
                    INLINEset_copy(pdest, p);
                    break;
                default: fail("node_cofactor: bad cube");
                    break;
            }
        }
    }

    r = node_create(func, nodevec_dup(node->fanin, node->nin), node->nin);
    r->is_dup_free = node->is_dup_free;
    node_minimum_base(r);
    return r;
}

node_t *
node_cofactor(node, cube)
        node_t *node, *cube;
{
    register pset last, p, g_cube_not, pdest;
    pset          g_cube, full;
    pset_family   newf, newg, func;
    node_t        *r, *fanin, **new_fanin;
    int           new_nin;

    switch (node_function(cube)) {
        case NODE_0: fail("node_cofactor: not defined for the 0 function");
            break;

        case NODE_1: r = node_dup(node);
            break;

        case NODE_BUF: fanin = node_get_fanin(cube, 0);
            r                = fast_cofactor(node, fanin, 1);
            break;

        case NODE_INV: fanin = node_get_fanin(cube, 0);
            r                = fast_cofactor(node, fanin, 0);
            break;

        case NODE_AND: make_common_base(node, cube, &new_fanin, &new_nin, &newf, &newg);

            define_cube_size(new_nin);

            g_cube     = GETSET(newg, 0);
            g_cube_not = set_new(new_nin * 2);
            full       = set_fill(set_new(new_nin * 2), new_nin * 2);
            (void) set_diff(g_cube_not, full, g_cube);
            set_free(full);

            func = sf_new(newf->count, newf->sf_size);
            foreach_set(newf, last, p)
            {
                if (cdist0(p, g_cube)) {
                    pdest = GETSET(func, func->count++);
                    INLINEset_or(pdest, p, g_cube_not);
                }
            }

            set_free(g_cube_not);
            sf_free(newf);
            sf_free(newg);

            /* allocate a new node */
            r = node_create(func, new_fanin, new_nin);
            r->is_dup_free = 1;        /* make_common_base assures this */
            node_minimum_base(r);
            break;

        default: fail("node_cofactor: defined for a single cube only");
    }

    return r;
}
