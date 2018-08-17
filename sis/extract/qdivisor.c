
#include "sis.h"

node_t *
ex_find_divisor_quick(node)
        node_t *node;
{
    register int *count, i;
    int          best, pos, phase, loops;
    node_t       *q, *f, *lit;

    f          = node_dup(node);
    for (loops = 0;; loops++) {
        count  = node_literal_count(f);
        best   = 0;
        for (i = 2 * node_num_fanin(f) - 1; i >= 0; i--) {
            if (count[i] > best) {
                best  = count[i];
                pos   = i / 2;
                phase = 1 - i % 2;
            }
        }
        FREE(count);

        if (best <= 1) break;

        lit = node_literal(node_get_fanin(f, pos), phase);
        q   = node_div(f, lit, NIL(node_t * ));
        node_free(f);
        node_free(lit);
        f = q;
    }

    if (loops == 0) {
        node_free(f);
        return 0;
    } else {
        return f;
    }
}
