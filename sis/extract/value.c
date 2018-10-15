/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/extract/value.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:23 $
 *
 */
#include "sis.h"
#include "extract_int.h"


value_cell_t *
value_cell_alloc()
{
    value_cell_t *value_cell;

    value_cell = ALLOC(value_cell_t, 1);
    value_cell->cube_number = 0;
    value_cell->sis_index = -1;
    value_cell->value = 1;
    value_cell->ref_count = 1;
    return value_cell;
}


void
value_cell_free(value_cell)
value_cell_t *value_cell;
{
    if (--value_cell->ref_count == 0) {
	FREE(value_cell);
    }
}


void
free_value_cells(A)
sm_matrix *A;
{
    register sm_row *prow;
    register sm_element *p;

    sm_foreach_row(A, prow) {
	sm_foreach_row_element(prow, p) {
	    value_cell_free((value_cell_t *) p->user_word);
	}
    }
}
