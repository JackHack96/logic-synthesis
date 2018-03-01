/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/mincov/bin_mincov.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:31 $
 *
 */
#include "sis.h"
#include "mincov_int.h"

typedef struct _value_t {
    sm_element * elem;
    double value;
} value_t;

static int is_unate;
static int (*g_record_fun) ();    /* called at the recursion end (if non-nil) */
static int verify_cover();

static sm_matrix *A;    /* reordered matrix */
static int *weights;    /* column weights */

/* expanded[row_num] points to the element currently expanded;
 * it is 0 if rule 3 was applied at the corresponding row level 
 */
static sm_element ** expanded;

/* order[row_num] is an array of pointers to elements; the row should
 * be expanded in that order.
 */
static sm_element *** order;

/* partial prime currently generated, in set form, and its cost */
static bin_solution_t *sol;    
/* best prime up to now, in set form, and its cost */
static bin_solution_t *best;

/* stop at first leaf if true */
static int g_heu;
static int ended;

/* debugging and statistics variables */
static int leaves_count, r1, r2, r3, r4, r5;
static int g_debug, g_option, got_best;
static long start_time;
int call_count, cc;

static int compare (), compare_rev ();
static char * literal_name ();
static sm_matrix * reorder_rows ();
static sm_row * mat_minimum_cover ();

/* get the index of the complement of the current literal */
#define GETBAR(v) (((v)&1)?((v)-1):((v)+1))

/* flags stored in the user_word of each element */
#define EXPANDED    1
#define MARKED        2

/* do the binate covering of matrix M with weights w (might be NULL);
 * if heuristic != 0, terminate as soon as the first result is obtained;
 * g_debug gives the amount of debugging information;
 * ubound, if > 0, gives the best possible estimate of the covering cost.
 * Any row can be missing.sh from M, but at least one column for each odd-even
 * pair should be present.
 */

sm_row *
sm_mat_bin_minimum_cover (M, weights, heuristic, debug, ubound, option, record_fun)

sm_matrix *M;
int *weights;
int heuristic, debug, ubound, option;
int (*record_fun) ();

{
    is_unate = 0;
    return mat_minimum_cover (M, weights, heuristic, debug, ubound, 
        option, record_fun);
}

sm_row *
sm_mat_minimum_cover (M, weights, heuristic, debug, ubound, option, record_fun)

sm_matrix *M;
int *weights;
int heuristic, debug, ubound, option;
int (*record_fun) ();

{
    is_unate = 1;
    return mat_minimum_cover (M, weights, heuristic, debug, ubound, 
        option, record_fun);
}

static sm_row *
mat_minimum_cover (M, w, heuristic, l_debug, ubound, l_option, record_fun)

sm_matrix *M;
int *w;
int heuristic, l_debug, ubound, l_option;
int (*record_fun) ();

{
    int nelem, i, row, col, cost, added, toadd;
    double sparsity, *col_value_save;
    sm_row *prow, *result;
    sm_col *pcol;
    sm_matrix *M_save, *temp1, *temp;
    sm_element *elem;
    value_t * col_value;
    pset unate;

    g_debug = l_debug;
    g_record_fun = record_fun;
    g_option = l_option;
    g_heu = heuristic;
    weights = w;
    ended = 0;
    M_save = NIL(sm_matrix);

    if (M->nrows <= 0) {
        return sm_row_alloc();
    }

    if (g_debug > 0) {
        start_time = util_cpu_time();

        /* Check the matrix sparsity */
        nelem = 0;
        sm_foreach_row(M, prow) {
            nelem += prow->length;
        }
        sparsity = (double) nelem / (double) (M->nrows * M->ncols);
        (void) fprintf(misout, "matrix     = %d by %d with %d elements (%4.3f%%)\n",
            M->nrows, M->ncols, nelem, sparsity * 100.0); 
    }
    if (g_debug > 3) {
        (void) fprintf (misout, "Weights:\n");
        for (i = 0; i < M->last_col->col_num + 1; i++) {
            (void) fprintf (misout, "%d ", WEIGHT(w, i));
        }
        (void) fprintf (misout, "\n");
        (void) fprintf (misout, "Matrix:\n");
        sm_print(misout, M);
        (void) fprintf (misout, "\n");
    }

    if (g_option & 2048) {
    /* reorder rows such that the tree-like ordering is preserved */
        M_save = M;
        M = sm_dup (M_save);
        A = sm_alloc ();

        unate = set_clear (set_new (M->last_col->col_num + 1), 
            M->last_col->col_num + 1);
        do {
            added = 0;
            temp = sm_alloc ();
            /* find unate columns */
            sm_foreach_col (M, pcol) {
                if (pcol->col_num % 2) {
                    if (! pcol->prev_col ||
                        pcol->prev_col->col_num != pcol->col_num - 1) {
                        set_insert (unate, pcol->col_num);
                        set_insert (unate, (pcol->col_num - 1));
                    }
                }
                else {
                    if (! pcol->next_col ||
                    pcol->next_col->col_num != pcol->col_num + 1) {
                        set_insert (unate, pcol->col_num);
                        set_insert (unate, (pcol->col_num + 1));
                    }
                }
            }

            /* check if all elements are unate */
            for (i = 0; M->nrows && i <= M->last_row->row_num; i++) {
                if (! (prow = sm_get_row (M, i))) {
                    continue;
                }

                toadd = 1;
                sm_foreach_row_element (prow, elem) {
                    if (! is_in_set (unate, elem->col_num)) {
                        toadd = 0;
                        break;
                    }
                }
                if (toadd) {
                    added = 1;
                    row = temp->nrows;
                    sm_foreach_row_element (prow, elem) {
                        (void) sm_insert (temp, row, elem->col_num);
                    }
                    sm_delrow (M, prow->row_num);
                    i--;
                }
            }

            /* check if all positive elements are unate */
            for (i = 0; M->nrows && i <= M->last_row->row_num; i++) {
                if (! (prow = sm_get_row (M, i))) {
                    continue;
                }

                toadd = 1;
                sm_foreach_row_element (prow, elem) {
                    if (! (elem->col_num % 2) &&
                        ! is_in_set (unate, elem->col_num)) {
                        toadd = 0;
                        break;
                    }
                }
                if (toadd) {
                    added = 1;
                    row = temp->nrows;
                    sm_foreach_row_element (prow, elem) {
                        (void) sm_insert (temp, row, elem->col_num);
                    }
                    sm_delrow (M, prow->row_num);
                    i--;
                }
            }

            temp1 = reorder_rows (temp);
            sm_foreach_row (temp1, prow) {
                row = A->nrows;
                sm_foreach_row_element (prow, elem) {
                    (void) sm_insert (A, row, elem->col_num);
                }
            }
            sm_free (temp);
            sm_free (temp1);
        } while (added && M->nrows);

        if (M->nrows) {
            (void) fprintf (miserr, 
                "warning: unate heuristics left %d rows out of %d\n",
                M->nrows, A->nrows + M->nrows);
            if (g_debug > 2) {
                (void) fprintf (misout, "Left-over ratrix:\n");
                sm_print(misout, M);
                (void) fprintf (misout, "\n");
            }
            temp = sm_alloc ();
            sm_foreach_row (M, prow) {
                row = temp->nrows;
                sm_foreach_row_element (prow, elem) {
                    (void) sm_insert (temp, row, elem->col_num);
                }
            }
            temp1 = reorder_rows (temp);
            sm_foreach_row (temp1, prow) {
                row = A->nrows;
                sm_foreach_row_element (prow, elem) {
                    (void) sm_insert (A, row, elem->col_num);
                }
            }
            sm_free (temp);
            sm_free (temp1);
        }
        sm_free (M);

        M = M_save;
        set_free (unate);
    }
    else {
        A = reorder_rows (M);
    }

    /* reorder cols; at every level the column pair with most elements BELOW
     * it is put first; weights might be used also... 
     */
    col_value = ALLOC (value_t, A->last_col->col_num + 1);
    col_value_save = ALLOC (double, A->last_col->col_num + 1);
    order = ALLOC (sm_element **, A->nrows);

    for (col = 0; col <= A->last_col->col_num; col++) {
        col_value_save[col] = 0.0;
    }

    /* run through the rows in reverse order */
    for (row = A->nrows - 1; row >= 0; row--) {
        prow = sm_get_row (A, row);

        col = 0;
        sm_foreach_row_element (prow, elem) {
            if (is_unate) {
                col_value_save[elem->col_num]++;
            }
            else {
                if (g_option & 4) {
                    /* skip odd columns */
                    if (! (elem->col_num % 2)) {
                        col_value_save[elem->col_num / 2]++;
                    }
                }
                else {
                    col_value_save[elem->col_num / 2]++;
                }
            }

            col_value[col].elem = elem;

            if (is_unate) {
                col_value[col].value = col_value_save[elem->col_num];
            }
            else {
                col_value[col].value = col_value_save[elem->col_num / 2];
                if (g_option & 64) {
                    if (WEIGHT(weights, elem->col_num) > 0) {
                        col_value[col].value /= WEIGHT(weights, elem->col_num);
                    }
                    else {
                        col_value[col].value *= 2;
                    }
                }
                if (g_option & 32) {
                    /* leave odd columns first */
                    if (elem->col_num % 2) {
                        col_value[col].value = 1000000.0;
                    }
                }
                else if (g_option & 128) {
                    /* leave odd columns last */
                    if (elem->col_num % 2) {
                        col_value[col].value = 0.0;
                    }
                }
            }

            col++;
        }

        if (g_option & 8) {
            /* do not sort columns */
        }
        else {
            if (g_option & 16) {
                qsort ((char*) col_value, col, sizeof(value_t), compare_rev);
            }
            else {
                qsort ((char*) col_value, col, sizeof(value_t), compare);
            }
        }

        order[row] = ALLOC (sm_element *, prow->length);

        for (col = 0; col < prow->length; col++) {
            order[row][col] = col_value[col].elem;
        }
    }

    /* statically allocate the stacks */
    expanded = ALLOC (sm_element *, A->nrows);

    if (g_debug > 2) {
        (void) fprintf (misout, "Col ordering:\n");
        for (row = 0; row < A->nrows; row++) {
            (void) fprintf (misout, "%d: ", row);
            prow = sm_get_row (A, row);
            for (col = 0; col < prow->length; col++) {
                (void) fprintf (misout, "%d ", order[row][col]->col_num);
            }
            (void) fprintf (misout, "\n");
        }
    }

    if (g_debug > 3) {
        (void) fprintf (misout, "Reordered matrix:\n");
        sm_print (misout, A);
        (void) fprintf (misout, "\n");
    }

    if (g_debug > 1) {
        (void) fprintf(misout, "reordering done (time is %s)\n", 
            util_print_time(util_cpu_time() - start_time));
    }

    /* give an upper bound (for rule 5) if available */
    got_best = 0;
    sol = bin_solution_alloc (A->last_col->col_num + 1);
    best = bin_solution_alloc (A->last_col->col_num + 1);
    if (ubound > 0) {
        /* hope we'll succeed... */
        best->cost = ubound;
    }
    else {
        /* not a very good bound... */
        sm_foreach_col (A, pcol) {
            best->cost += WEIGHT(weights, pcol->col_num);
        }
    }
    best->cost++;

    /* do the actual work */
    sm_bin_mincov (0);

    if (! g_record_fun) {
        /* check out various things */
        if (! got_best) {
            (void) fprintf(miserr,"internal error; couldn't improve on bound\n");
	    (void) fprintf(miserr,"unsatisfiable binate covering matrix ?\n");
            exit(-1);
        }

        /* put back the solution in sparse matrix (reordered) form */
        result = sm_row_alloc();
        for (i = 0; i < A->last_col->col_num + 1; i++) {
            if (is_in_set (best->set, i)) {
                sm_row_insert (result, i);
            }
        }

        if (g_debug > 0) {
            (void) fprintf (misout, "calls %d leaves %d r1 %d r2 %d r3 %d r4 %d r5 %d\n", 
                call_count, leaves_count, r1, r2, r3, r4, r5);

            if (g_debug > 1) {
                (void) fprintf (misout, "Solution is ");
                sm_row_print (misout, result);
                (void) fprintf (misout, "\n");
            }

            (void) fprintf(misout, "best solution %d; time %s\n", best->cost,
                util_print_time(util_cpu_time() - start_time));
        }

        /* check the cost and the result itself ... */
        cost = 0;
        sm_foreach_row_element (result, elem) {
            cost += WEIGHT (w, elem->col_num);
        }
        if (cost != best->cost) {
            (void) fprintf (miserr, "Solution cost is corrupted\n");
            exit (-1);
        }
        if (! verify_cover(M, result)) {
            (void) fprintf (misout, "Illegal solution ");
            sm_row_print (misout, result);
            (void) fprintf (misout, "\nwith cost %d\n", best->cost);

            (void) fprintf (miserr, "internal error -- cover verification failed\n");
            exit (-1);
        }
    }

    FREE (expanded);
    FREE (col_value);
    FREE (col_value_save);
    bin_solution_free(best);
    bin_solution_free(sol);
    for (row = A->nrows - 1; row >= 0; row--) {
        FREE (order[row]);
    }
    FREE (order);
    sm_free (A);

    return result;
}

/* reorder the rows of M according to g_option */
static sm_matrix *
reorder_rows (M)

sm_matrix *M;

{
    int i, row, neg_unate, pos_unate;
    sm_row *prow;
    sm_element *elem;
    sm_matrix *result;
    value_t * row_value;

    /* reorder rows; longest first */
    /* row_value[i].elem->row_num gives the number of the old row in the i-th 
     * position of the new matrix;
     */
    result = sm_alloc ();
    if (M->last_row == NIL(sm_row) || M->last_col == NIL(sm_col)) {
		return result;
	}
    row_value = ALLOC (value_t, M->nrows);
    i = 0;
    sm_foreach_row (M, prow) {
        if (prow->length) {
            row_value[i].elem = prow->first_col;
            if (g_option & 1) {
                /* save original order for ties */
                row_value[i].value = 10000 * prow->length + i;
            }
            else if (g_option & 256) {
            /* define as value the sum of all column values */
                row_value[i].value = 0;
                sm_foreach_row_element (prow, elem) {
                    row_value[i].value += WEIGHT (weights, elem->col_num);
                }
            }
            else {
                row_value[i].value = prow->length;
            }

            if (g_option & (512 | 1024)) {
            /* check if it is 'unate' */
                pos_unate = 1;
                neg_unate = 1;
                sm_foreach_row_element (prow, elem) {
                    if (elem->col_num % 2) {
                        pos_unate = 0;
                    }
                    else {
                        neg_unate = 0;
                    }
                }

                if (pos_unate || neg_unate) {
                    if (g_option & 512) {
                        /* 'unate' rows first */
                        row_value[i].value += 10000;
                    }
                    else {
                        /* 'unate' rows last */
                        row_value[i].value -= 10000;
                    }
                }
            }

            i++;
        }
    }

    if (g_option & 2) {
        /* do not sort rows */
    }
    else if (g_option & 4096) {
        qsort ((char *) row_value, M->nrows, sizeof(value_t), compare_rev);
    }
    else {
        qsort ((char *) row_value, M->nrows, sizeof(value_t), compare);
    }

    /* permute the rows of M into A, according to col_value */
    sm_resize(result, M->last_row->row_num, M->last_col->col_num);
    for (i = 0; i < M->nrows; i++) {
        /* the i-th old column has row_value[i].elem->row_num now !! */
        prow = sm_get_row (M, row_value[i].elem->row_num);
        sm_foreach_row_element (prow, elem) {
            (void) sm_insert (result, i, elem->col_num);
        }
    }

    if (g_debug > 2) {
        (void) fprintf (misout, "Partial row ordering:\n");
        for (row = 0; row < result->nrows; row++) {
            (void) fprintf (misout, "%d ", row_value[row].elem->row_num);
        }
        (void) fprintf (misout, "\n");
    }

    FREE (row_value);

    return result;
}

/* Apply Mathony's algorithm to generate all weighted primes of matrix A.
 * 1) if we reached the end of the recursion, check if the current solution
 *    improves the best seen so far, and save it.
 * 2) rule 3: if the current solution contains a literal appearing also
 *    in the current row, ignore the current row.
 * 3) for each literal in the current row (sum):
 *    a) rule 1: if the current solution contains the complement of the 
 *       literal, ignore the current literal;
 *    b) rule 5: if the current solution plus the cost of the current literal
 *       would cost more than the best solution, ignore the current literal;
 *    c) rule 2: if a row at an upper level contains the same literal yet to
 *       be expanded, ignore the current literal and mark the literal in the
 *       upper row;
 *    d) rule 4: if a row at an upper level contains the same literal already
 *       expanded and if rule 2 was not applied with respect to the literal
 *       in the upper row in the current expansion (i.e. it is not marked),
 *       ignore the current literal;
 *    e) otherwise, add the current literal to the solution and recur for the
 *       next row.
 * 4) remove rule 2 marks from the current rows (if any).
 */

void
sm_bin_mincov (row_num)

int row_num;

{
    sm_row *curr_row;
    sm_element *elem, *uplevel_elem;
    int literal, literal_bar, col;

    call_count++;
#ifdef DEBUG
    /* debugging; dbx is too slow... */
    if (call_count == cc) {
        (void) fprintf (miserr, "got it\n");
    }
#endif

    /* check for recursion end, and record the solution if improving */
    if (row_num == A->nrows) {
        if (g_record_fun) {
            curr_row = sm_row_alloc();
            for (col = 0; col < A->last_col->col_num + 1; col++) {
                if (is_in_set (sol->set, col)) {
                    sm_row_insert (curr_row, col);
                }
            }
            if ((*g_record_fun) (curr_row)) {
                ended = 1;
            }
            sm_row_free (curr_row);
            return;
        }
        else if (sol->cost < best->cost) {
            got_best = 1;
            bin_solution_free (best);
            best = bin_solution_dup (sol, A->last_col->col_num + 1);

            if (g_debug > 0) {
                (void) fprintf(misout, "  new 'best' solution %d; time %s; calls %d\n", 
                    best->cost,
                    util_print_time(util_cpu_time() - start_time),
                    call_count);
                (void) fflush (misout);
#ifdef DEBUG
                if (g_debug > 5) {
                    if (g_debug > 9) {
                        sp (best->set);
                    }
                    else {
                        (void) fprintf (misout, "\n");
                    }
                }
#endif
            }

        }
        leaves_count++;
        if (g_heu) {
            ended = 1;
        }
        return;
    }

    /* initialize the level */
    curr_row = sm_get_row (A, row_num);
    expanded[row_num] = NIL(sm_element);    /* nothing chosen... */

#ifdef DEBUG
    if (g_debug > 9) {
        print_lev (row_num, "beginning ");
        rp (curr_row);
    }
#endif

    /* begin R3 */
    /* check if any of the literals in the row appears in the solution already;
     * if so ignore the current row
     */
    sm_foreach_row_element (curr_row, elem) {
        if (is_in_set ((sol->set), elem->col_num)) {
#ifdef DEBUG
            if (g_debug > 9) {
                print_lev (row_num, "ignoring (R3)\n");
            }
#endif

            r3++;
            /* recur for next row */
            sm_bin_mincov (row_num + 1);
            return;
        }
    }
    /* end R3 */

    /* now select each literal in turn */
    for (col = 0; ! ended && col < curr_row->length; col++) {
        elem = order[row_num][col];

        literal = elem->col_num;
        if (! is_unate) {
            literal_bar = GETBAR (literal);

            /* begin R1 */
            /* check if literal appears complemented in the solution; if so
             * ignore the literal
             */
            if (is_in_set ((sol->set), literal_bar)) {
#ifdef DEBUG
                if (g_debug > 9) {
                    print_lev (row_num, "pruning (R1) %s\n", 
                        literal_name (literal));
                }
#endif

                r1++;
                continue;
            }
            /* end R1 */
        }

        /* begin R5 */
        /* check if adding the current literal would make the solution worse;
         * if so, ignore the literal
         */
        if (sol->cost + WEIGHT(weights, literal) >= best->cost) {
#ifdef DEBUG
            if (g_debug > 6) {
                print_lev (row_num, "pruning (R5) (cost=%d) ", 
                    sol->cost + WEIGHT(weights, literal));
                if (g_debug > 9) {
                    (void) fprintf (misout, "%s ", literal_name (literal));
                    sp (sol->set);
                }
                else {
                    (void) fprintf (misout, "\n");
                }
            }
#endif

            r5++;
            continue;
        }
        /* end R5 */

        /* begin R2 and R4 */
        /* prepare for rule 5 and 6 check:
         * update the uplevel_elem pointer to be the first element in the 
         * same column (if any) with row < row_num (i.e. belonging to
         * an upper sum) and with expanded[row] != 0.
         * If this element was EXPANDED, then the currently expanded element
         * at that level must not have been MARKED
         */
        for (uplevel_elem = elem->prev_row; uplevel_elem != NIL(sm_element);
            uplevel_elem = uplevel_elem->prev_row) {
            if (expanded[uplevel_elem->row_num]) {
                if ((EXPANDED & (sm_get (int, uplevel_elem)))) {
                    if (! (MARKED & 
                        (sm_get (int, expanded[uplevel_elem->row_num])))) {
                        break;
                    }
                }
                else {
                    break;
                }
            }
        }

        /* check if the same literal appears at an upper level */
        if (uplevel_elem != NIL(sm_element)) {

            /* check if it was a yet unexpanded literal; if so, ignore the
             * current literal and record this, marking the current literal
             * at the upper level (and the row, for a faster reset).
             */
            if (! (EXPANDED & (sm_get (int, uplevel_elem)))) {
                sm_put (uplevel_elem, ((sm_get (int, uplevel_elem)) | MARKED));
#ifdef DEBUG
                if (g_debug > 9) {
                    print_lev (row_num, "pruning (R2) %s\n", 
                        literal_name (literal));
                }
#endif
                r2++;
                continue;
            }
            else {
            /* ignore the current literal (no need to record it...) */
#ifdef DEBUG
                if (g_debug > 9) {
                    print_lev (row_num, "pruning (R4) %s\n", 
                        literal_name (literal));
                }
#endif
                r4++;
                continue;
            }
        }
        /* end R2 and R4 */ 
        
        /* add the current literal to the solution and recur */
        expanded[row_num] = elem;
        sm_put (elem, ((sm_get (int, elem)) | EXPANDED));
        bin_solution_add (sol, weights, literal);

#ifdef DEBUG
        if (g_debug > 9) {
            print_lev (row_num, "choosing %s: ", literal_name (literal));
            sp (sol->set);
        }
#endif

        sm_bin_mincov (row_num + 1);

        /* remove the current literal */
        bin_solution_del (sol, weights, literal);
    }

    /* end of literal selection loop */

    /* un-mark the current row */
    sm_foreach_row_element (curr_row, elem) {
        sm_put (elem, 0);
    }

    expanded[row_num] = NIL(sm_element);

#ifdef DEBUG
    if (g_debug > 9) {
        print_lev (row_num, "ending\n");
    }
#endif
}

/* compare two value records
 */
static int 
compare (value1, value2)

value_t *value1, *value2;

{
    return value2->value - value1->value;
}

static int 
compare_rev (value1, value2)

value_t *value1, *value2;

{
    return value1->value - value2->value;
}

/* debugging stuff */
static char *
literal_name (literal)

int literal;

{
    static char buf[20];

    (void) sprintf (buf, "%d", literal);
    /* 
    if (! (literal % 2)) {
        (void) sprintf (buf, "%d", literal / 2);
    }
    else {
        (void) sprintf (buf, "%d'", literal / 2);
    } */
    return buf;
}

rp (row)

sm_row *row;

{
    sm_element *element;

    sm_foreach_row_element (row, element) {
        (void) fprintf (misout, "%s ", literal_name (element->col_num));
    }
    (void) fprintf (misout, "\n");
}

sp (set)
pset set;

{
    int i;

    for (i = 0; i < A->last_col->col_num + 1; i++) {
        if (is_in_set (set, i)) {
            (void) fprintf (misout, "%s ", literal_name (i));
        }
    }
    (void) fprintf (misout, "\n");
}

print_lev (row_num, p1, p2, p3, p4)

int row_num;
char *p1, *p2, *p3, *p4;

{
    int i;

    (void) fprintf (misout, "%d\t ", call_count);
    for (i = 0; i < row_num; i++) {
        putchar (' ');
        putchar (' ');
    }
    (void) fprintf (misout, p1, p2, p3, p4);
}

ps (set)
pset set;
{
    (void) fprintf (misout, ps1(set));
}

pm (mat)
sm_matrix *mat;
{
    sm_print (misout, mat);
}

wm (mat)
sm_matrix *mat;
{
    sm_write (misout, mat);
}

static int 
verify_cover(A, cover)
sm_matrix *A;
sm_row *cover;
{
    sm_row *prow;

    sm_foreach_row(A, prow) {
    if (! sm_row_intersects(prow, cover)) {
        return 0;
    }
    }
    return 1;
}
