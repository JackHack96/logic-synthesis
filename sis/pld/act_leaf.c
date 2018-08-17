
/* Created by Narendra */
#include "sis.h"
#include "../include/pld_int.h"

static check_universal();

static void unate_split_F();

/* Here we have obtained a unate leaf and wish to construct its bdd*/
act_t *
unate_act(F)
        sm_matrix *F;
{
    act_t      *act, *my_init_act(), *act_F(), *unate_act(), *my_or_act_F();
    int        i;
    sm_matrix  *M;
    sm_col     *pcol;
    sm_row     *cover;
    sm_element *p;
    array_t    *array, *array_act;
    char       *dummy;
    int        *phase;


/* Check whether the matrix has more than one cube and contains
 * the universal cube. This helps to prune the tree!*/
    if (check_universal(F)) {
        if (st_lookup(end_table, (char *) 1, &dummy)) {
            act = (act_t *) dummy;
        } else {
            (void) fprintf(sisout, "error in st_table\n");
        }
    } else if (F->nrows == 1) {
/* If matrix has only one leaf construct act so that the and of the 
 * variables is realised. */
        act = act_F(F);
    } else {
/* Construct the M matrix which is used for covering puposes*/
        phase = ALLOC(
        int, F->ncols);
        M = sm_alloc();
        sm_foreach_col(F, pcol)
        {
            sm_foreach_col_element(pcol, p)
            {
                if (p->user_word != (char *) 2) {
                    (void) sm_insert(M, p->row_num, p->col_num);
                    phase[p->col_num] = (int) p->user_word;
                    if (ACT_DEBUG) {
                        (void) fprintf(sisout, "the phase is %d\n",
                                       phase[p->col_num]);
                    }
                }
            }
        }

/* solve covering problem for M*/
        cover = sm_minimum_cover(M, NIL(
        int), 0, 0);
        sm_foreach_row_element(cover, p)
        {
            p->user_word = (char *) phase[p->col_num];
        }
        sm_free_space(M);
        FREE(phase);
        array = array_alloc(sm_matrix * , 0);
/* construct sub matrices as a result of the covering solution*/
        unate_split_F(F, cover, array);
/* Recursively solve each of the sub matrices*/
        array_act = array_alloc(act_t * , 0);
        for (i    = 0; i < array_n(array); i++) {
            act = unate_act(array_fetch(sm_matrix * , array, i));
            array_insert_last(act_t * , array_act, act);
        }
        if (array_n(array_act) != cover->length) {
            (void) fprintf(sisout,
                           "error array length (of acts) and minucov solution do not match \n");
        }
        act    = my_or_act_F(array_act, cover, array);
/* Free up the area alloced for array and the Fi s.*/
        for (i = 0; i < array_n(array); i++) {
            sm_free_space(array_fetch(sm_matrix * , array, i));
        }
        array_free(array);
        array_free(array_act);
        sm_row_free(cover);
    }
    return act;
}


/* checks for a row of 2's. returns 1 if true else 0*/
static int
check_universal(F)
        sm_matrix *F;
{
    sm_row     *row;
    sm_element *p;
    int        var;

    sm_foreach_row(F, row)
    {
        var = 1;
        sm_foreach_row_element(row, p)
        {
            if (p->user_word != (char *) 2) {
                var = 0;
            }
        }
        if (var) return 1;
    }
    return 0;
}






/* constructs a set of sub matrices from F such that */
static void
unate_split_F(F, cover, array)
        sm_matrix *F;
        sm_row *cover;
        array_t *array;
{
    sm_element *p, *q;
    sm_col     *col;
    sm_matrix  *Fi;
    sm_row     *row, *F_row, *my_row_dup();
    int        col_num;

/* Initialise row->user_word == NUll to prevent duplication of cubes
 * if 2 elements in the cover contain the same cube! Need for optimisation*/

    sm_foreach_row(F, row)
    {
        row->user_word = (char *) 0;
    }
    sm_foreach_row_element(cover, q)
    {
        col_num = q->col_num;
        col     = sm_get_col(F, col_num);
        Fi      = sm_alloc();
        sm_foreach_col_element(col, p)
        {
            F_row = sm_get_row(F, p->row_num);
            if ((p->user_word != (char *) 2) && (F_row->user_word == (char *) 0)) {
                my_sm_copy_row(F_row, Fi, col_num, F);
                if (ACT_DEBUG) {
                    my_sm_print(Fi);
                }
                F_row->user_word = (char *) 1;
            }
        }
        array_insert_last(sm_matrix * , array, Fi);
    }
}

