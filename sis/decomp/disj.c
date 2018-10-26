
#include "sis.h"
#include "decomp.h"
#include "decomp_int.h"


array_t *
decomp_disj(f)
node_t *f;
{
    array_t *fa;
    sm_matrix *M, *L, *R;
    node_t *or, *part, *lit, *temp;
 
    fa = array_alloc(node_t *, 0);
    M = dec_node_to_sm(f);
    if (dec_block_partition(M, &L, &R)) {

	/* first partition */
	part = dec_sm_to_node(L);
	or = node_literal(part, 1); 
	array_insert_last(node_t *, fa, or);
	array_insert_last(node_t *, fa, part);
	sm_free(L);
	sm_free(M);
	M = R;

	/* the rest partition */
	while (dec_block_partition(M, &L, &R)) {
	    part = dec_sm_to_node(L);
	    array_insert_last(node_t *, fa, part);
	    lit = node_literal(part, 1);
	    temp = node_or(or, lit);
	    node_free(or);
	    node_free(lit);
	    or = temp;
	    sm_free(L);
	    sm_free(M);
	    M = R;
	}

	/* last partition */
	part = dec_sm_to_node(M);
	array_insert_last(node_t *, fa, part);
	lit = node_literal(part, 1);
	temp = node_or(or, lit);
	node_free(or);
	node_free(lit);
	array_insert(node_t *, fa, 0, temp);   /* root */
	sm_free(M);

    } else {
	sm_free(M);
	array_insert_last(node_t *, fa, node_dup(f));
    }

    return fa;
}
