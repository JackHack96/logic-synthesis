/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/sparse/sparse.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:51 $
 *
 */
#ifndef SPARSE_H
#define SPARSE_H

#include "ansi.h"

/* hack to fix conflict with libX11.a */
#define sm_alloc sm_allocate
#define sm_free sm_free_space

/*
 *  sparse.h -- sparse matrix package header file
 */

typedef struct sm_element_struct sm_element;
typedef struct sm_row_struct sm_row;
typedef struct sm_col_struct sm_col;
typedef struct sm_matrix_struct sm_matrix;


/*
 *  sparse matrix element
 */
struct sm_element_struct {
    int row_num;		/* row number of this element */
    int col_num;		/* column number of this element */
    sm_element *next_row;	/* next row in this column */
    sm_element *prev_row;	/* previous row in this column */
    sm_element *next_col;	/* next column in this row */
    sm_element *prev_col;	/* previous column in this row */
    char *user_word;		/* user-defined word */
};


/*
 *  row header
 */
struct sm_row_struct {
    int row_num;		/* the row number */
    int length;			/* number of elements in this row */
    int flag;			/* user-defined word */
    sm_element *first_col;	/* first element in this row */
    sm_element *last_col;	/* last element in this row */
    sm_row *next_row;		/* next row (in sm_matrix linked list) */
    sm_row *prev_row;		/* previous row (in sm_matrix linked list) */
    char *user_word;		/* user-defined word */
};


/*
 *  column header
 */
struct sm_col_struct {
    int col_num;		/* the column number */
    int length;			/* number of elements in this column */
    int flag;			/* user-defined word */
    sm_element *first_row;	/* first element in this column */
    sm_element *last_row;	/* last element in this column */
    sm_col *next_col;		/* next column (in sm_matrix linked list) */
    sm_col *prev_col;		/* prev column (in sm_matrix linked list) */
    char *user_word;		/* user-defined word */
};


/*
 *  A sparse matrix
 */
struct sm_matrix_struct {
    sm_row **rows;		/* pointer to row headers (by row #) */
    int rows_size;		/* alloc'ed size of above array */
    sm_col **cols;		/* pointer to column headers (by col #) */
    int cols_size;		/* alloc'ed size of above array */
    sm_row *first_row;		/* first row (linked list of all rows) */
    sm_row *last_row;		/* last row (linked list of all rows) */
    int nrows;			/* number of rows */
    sm_col *first_col;		/* first column (linked list of columns) */
    sm_col *last_col;		/* last column (linked list of columns) */
    int ncols;			/* number of columns */
    char *user_word;		/* user-defined word */
};


#define sm_get_col(A, colnum)	\
    (((colnum) >= 0 && (colnum) < (A)->cols_size) ? \
	(A)->cols[colnum] : (sm_col *) 0)

#define sm_get_row(A, rownum)	\
    (((rownum) >= 0 && (rownum) < (A)->rows_size) ? \
	(A)->rows[rownum] : (sm_row *) 0)

#define sm_foreach_row(A, prow)	\
	for(prow = A->first_row; prow != 0; prow = prow->next_row)

#define sm_foreach_col(A, pcol)	\
	for(pcol = A->first_col; pcol != 0; pcol = pcol->next_col)

#define sm_foreach_row_element(prow, p)	\
	for(p = (prow == 0) ? 0 : prow->first_col; p != 0; p = p->next_col)

#define sm_foreach_col_element(pcol, p) \
	for(p = (pcol == 0) ? 0 : pcol->first_row; p != 0; p = p->next_row)

#define sm_put(x, val) \
	(x->user_word = (char *) val)

#define sm_get(type, x) \
	((type) (x->user_word))

EXTERN sm_matrix *sm_allocate ARGS((void));
EXTERN sm_matrix *sm_alloc_size ARGS((int, int));
EXTERN void sm_free_space ARGS((sm_matrix *));
EXTERN sm_matrix *sm_dup ARGS((sm_matrix *));
EXTERN void sm_resize ARGS((sm_matrix *, int, int));
EXTERN sm_element *sm_insert ARGS((sm_matrix *, int, int));
EXTERN sm_element *sm_find ARGS((sm_matrix *, int, int));
EXTERN void sm_remove ARGS((sm_matrix *, int, int));
EXTERN void sm_remove_element ARGS((sm_matrix *, sm_element *));
EXTERN void sm_delrow ARGS((sm_matrix *, int));
EXTERN void sm_delcol ARGS((sm_matrix *, int));
EXTERN void sm_copy_row ARGS((sm_matrix *, int, sm_row *));
EXTERN void sm_copy_col ARGS((sm_matrix *, int, sm_col *));
EXTERN sm_row *sm_longest_row ARGS((sm_matrix *));
EXTERN sm_col *sm_longest_col ARGS((sm_matrix *));
EXTERN int sm_num_elements ARGS((sm_matrix *));
EXTERN int sm_read ARGS((FILE *, sm_matrix **));
EXTERN int sm_read_compressed ARGS((FILE *, sm_matrix **));
EXTERN void sm_write ARGS((FILE *, sm_matrix *));
EXTERN void sm_print ARGS((FILE *, sm_matrix *));
EXTERN void sm_dump ARGS((sm_matrix *, char *, int));
EXTERN void sm_cleanup ARGS((void));

EXTERN sm_col *sm_col_alloc ARGS((void));
EXTERN void sm_col_free ARGS((sm_col *));
EXTERN sm_col *sm_col_dup ARGS((sm_col *));
EXTERN sm_element *sm_col_insert ARGS((sm_col *, int));
EXTERN void sm_col_remove ARGS((sm_col *, int));
EXTERN sm_element *sm_col_find ARGS((sm_col *, int));
EXTERN int sm_col_contains ARGS((sm_col *, sm_col *));
EXTERN int sm_col_intersects ARGS((sm_col *, sm_col *));
EXTERN int sm_col_compare ARGS((sm_col *, sm_col *));
EXTERN sm_col *sm_col_and ARGS((sm_col *, sm_col *));
EXTERN int sm_col_hash ARGS((sm_col *, int));
EXTERN void sm_col_remove_element ARGS((sm_col *, sm_element *));
EXTERN void sm_col_print ARGS((FILE *, sm_col *));

EXTERN sm_row *sm_row_alloc ARGS((void));
EXTERN void sm_row_free ARGS((sm_row *));
EXTERN sm_row *sm_row_dup ARGS((sm_row *));
EXTERN sm_element *sm_row_insert ARGS((sm_row *, int));
EXTERN void sm_row_remove ARGS((sm_row *, int));
EXTERN sm_element *sm_row_find ARGS((sm_row *, int));
EXTERN int sm_row_contains ARGS((sm_row *, sm_row *));
EXTERN int sm_row_intersects ARGS((sm_row *, sm_row *));
EXTERN int sm_row_compare ARGS((sm_row *, sm_row *));
EXTERN sm_row *sm_row_and ARGS((sm_row *, sm_row *));
EXTERN int sm_row_hash ARGS((sm_row *, int));
EXTERN void sm_row_remove_element ARGS((sm_row *, sm_element *));
EXTERN void sm_row_print ARGS((FILE *, sm_row *));

#endif
