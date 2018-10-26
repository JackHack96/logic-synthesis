
#include "sis.h"

static int visit_col();

static int
visit_row(A, prow, rows_visited, cols_visited)
sm_matrix *A;
sm_row *prow;
int *rows_visited;
int *cols_visited;
{
    sm_element *p;
    sm_col *pcol;

    if (! prow->flag) {
	prow->flag = 1;
	(*rows_visited)++;
	if (*rows_visited == A->nrows) {
	    return 1;
	}
	sm_foreach_row_element(prow, p) {
	    pcol = sm_get_col(A, p->col_num);
	    if (! pcol->flag) {
		if (visit_col(A, pcol, rows_visited, cols_visited)) {
		    return 1;
		}
	    }
	}
    }
    return 0;
}


static int
visit_col(A, pcol, rows_visited, cols_visited)
sm_matrix *A;
sm_col *pcol;
int *rows_visited;
int *cols_visited;
{
    sm_element *p;
    sm_row *prow;

    if (! pcol->flag) {
	pcol->flag = 1;
	(*cols_visited)++;
	if (*cols_visited == A->ncols) {
	    return 1;
	}
	sm_foreach_col_element(pcol, p) {
	    prow = sm_get_row(A, p->row_num);
	    if (! prow->flag) {
		if (visit_row(A, prow, rows_visited, cols_visited)) {
		    return 1;
		}
	    }
	}
    }
    return 0;
}

static sm_row *
copy_row(A, prow)
register sm_matrix *A;
register sm_row *prow;
{
    register sm_element *p;
    register sm_element *element;

    sm_foreach_row_element(prow, p) {
	element = sm_insert(A, p->row_num, p->col_num);
	sm_put(element, sm_get(char *, p)); 
    }
    return sm_get_row(A, prow->row_num);
}

static void
block_copy(A, L, R)
register sm_matrix *A;
register sm_matrix **L;
register sm_matrix **R;
{
    register sm_row *prow;
    register sm_col *pcol;
    register sm_row *nrow;

    sm_foreach_row(A, prow) {
	if (prow->flag) {
	    nrow = copy_row(*L, prow);
	} else {
	    nrow = copy_row(*R, prow);
	}
	sm_put(nrow, sm_get(char *, prow));
    }

    sm_foreach_col((*L), pcol) {
	sm_put(pcol, sm_get(char *, sm_get_col(A, pcol->col_num)));
    }
    sm_foreach_col((*R), pcol) {
	sm_put(pcol, sm_get(char *, sm_get_col(A, pcol->col_num)));
    }
}

int
dec_block_partition(A, L, R)
sm_matrix *A;
sm_matrix **L, **R;
{
    int cols_visited, rows_visited;
    register sm_row *prow;
    register sm_col *pcol;

    /* Avoid the trivial case */
    if (A->nrows == 0) {
	return 0;
    }

    /* Reset the visited flags for each row and column */
    sm_foreach_row(A, prow) {
	prow->flag = 0;
    }
    sm_foreach_col(A, pcol) {
	pcol->flag = 0;
    }

    cols_visited = rows_visited = 0;
    if (visit_row(A, A->first_row, &rows_visited, &cols_visited)) {
	/* we found all of the rows */
	return 0;
    } else {
	*L = sm_alloc();
	*R = sm_alloc();
	block_copy(A, L, R);
	return 1;
    }
}
