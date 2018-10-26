
#include "sis.h"
#include "decomp.h"
#include "decomp_int.h"


array_t *
decomp_recur(f, gen_divisor)
node_t *f;
node_t *(*gen_divisor)();
{
    node_t *g;
    array_t *fa, *ga;

    if ((g = (*gen_divisor)(f)) != NIL(node_t)) {
	if (node_substitute(f, g, 1)) {
	    fa = decomp_recur(f, gen_divisor);
	    ga = decomp_recur(g, gen_divisor);
	    array_append(fa, ga);
	    array_free(ga);
	    return fa; 
	} else {
	    fail("Internal error: divisor can't be substituted\n");
	    return NIL(array_t);
	}
    } else {
	fa = array_alloc(node_t *, 0);
	array_insert_last(node_t *, fa, f);
	return fa;
    }
}
