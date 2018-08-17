
#include "sis.h"
#include "../include/decomp.h"
#include "../include/decomp_int.h"

node_t *
decomp_quick_kernel(f)
        node_t *f;
{
    return ex_find_divisor_quick(f);
}

node_t *
decomp_good_kernel(f)
        node_t *f;
{
    return ex_find_divisor(f, 1, 1);
}

node_t *
dec_node_cube(f, i)
        node_t *f;
        int i;
{
    node_cube_t cube;
    node_t      *c, *fanin, *tlit, *t;
    int         j;

    c    = node_constant(1);
    cube = node_get_cube(f, i);
    foreach_fanin(f, j, fanin)
    {
        switch (node_get_literal(cube, j)) {
            case ZERO: tlit = node_literal(fanin, 0);
                t           = node_and(c, tlit);
                node_free(tlit);
                node_free(c);
                c = t;
                break;
            case ONE: tlit = node_literal(fanin, 1);
                t          = node_and(c, tlit);
                node_free(tlit);
                node_free(c);
                c = t;
                break;
            default:;
        }
    }

    return c;
}
