
/* 
 * routines to convert node to/from sm
 * exports:  simp_node_to_sm();
 *           simp_sm_to_node();
 */

#include "sis.h"
#include "simp_int.h"

static int get_fcol_sm();

/* 
 * create a single sparse matrix for the on set and dont care set. 
 * note that all fanin of f occur in columns before variables not 
 * used by f in the matrix created.
 */
sm_matrix *
simp_node_to_sm(f, dc)
node_t *f, *dc;
{
    sm_matrix *M;
    st_table *table;
    sm_element *element;
    sm_row *row;
    sm_col *column;
    node_cube_t cube;
    node_literal_t literal;
    node_t *fanin;
    int i, j, col_num;
    int f_numrows, dc_numrows;

    M = sm_alloc();
    table = st_init_table(st_ptrcmp, st_ptrhash);

    /* create elements first for the on set f */
    f_numrows = node_num_cube(f);
    for(i = 0; i < f_numrows; i++) {
        cube = node_get_cube(f, i);
        foreach_fanin(f, j, fanin) {
            literal = node_get_literal(cube, j);
            switch (literal) {
            case ZERO:
            case ONE:
                (void) st_insert(table, (char *) fanin, (char *) j);
                element = sm_insert(M, i, j);
                if (literal == ZERO)
                    sm_put(element, 0);
                else
                    sm_put(element, 1);
                column = sm_get_col(M, j);
                sm_put(column, fanin);
                break;
            case TWO:
                break;
            default:
                fail("bad literal");
            }
        }
        row = sm_get_row(M, i);
        sm_put(row, F_SET); 
    }

    /* create elements for the dont care set dc. variables common to the 
     * on set must be inserted in the same column as their occurrence in the
     * on set */
    dc_numrows = node_num_cube(dc);
    for(i = 0; i < dc_numrows; i++) {
        cube = node_get_cube(dc, i);
        foreach_fanin(dc, j, fanin) {
            literal = node_get_literal(cube, j);
            switch (literal) {
            case ZERO:
            case ONE:
                col_num = get_fcol_sm(table, M, fanin);
                element = sm_insert(M, (i + f_numrows), col_num);
                column = sm_get_col(M, col_num);
                if (literal == ZERO)
                    sm_put(element, 0);
                else
                    sm_put(element, 1);
                sm_put(column, fanin);
                break;
            case TWO:
                break;
            default:
                fail("bad literal");
                /* NOTREACHED */
            }
        }
        row = sm_get_row(M, (i + f_numrows));
        sm_put(row, DC_SET); 
    }
    (void) st_free_table(table);
    return M;
}

/* return the column number where a variable occurs in the matrix. return the
 * next unused column number if the variable is absent */
static int
get_fcol_sm(table, M, np)
st_table *table;
sm_matrix *M;
node_t *np;
{
    char *dummy;

    if (st_lookup(table, (char *) np, &dummy)) {
        return((int) dummy);
    }
    else {
        (void) st_insert(table, (char *) np, (char *) sm_ncols(M));
        return(sm_ncols(M));
    }
} 

void
simp_sm_print(M)
sm_matrix *M;
{
    sm_row *row;
    sm_col *col;
    sm_element *element;
    int i, j;

    (void) fprintf(sisout, "\n   ");
    sm_foreach_col(M, col) {
        (void) fprintf(sisout, "%3s", node_name(sm_get(node_t *, col)));
    }
    (void) fprintf(sisout, "\n");

    for (i = 0; i < M->rows_size; i++) {
        row = sm_get_row(M, i);
        if (row == NULL)
            continue;
        (void) fprintf(sisout," %c ",(sm_get(int, row) == F_SET)?'f':'d');
        for (j = 0; j < M->cols_size; j++) {
            if (sm_get_col(M, j) == NULL)
                continue;
            element = sm_find(M, i, j);
            if (element == NIL(sm_element)) {
                (void) fprintf(sisout, "  .");
            } 
        else {
            switch (sm_get(int, element)) {
            case 0:
                (void) fprintf(sisout, "  0"); 
                break;
            case 1:
                (void) fprintf(sisout, "  1"); 
                break;
            default:
                fail("bad sm_element_data");
            }
        }
    }
    (void) fprintf(sisout, "\n");
    }
}

/*
 * map the dont care part of a sparse-matrix representation of a Mtion 
 * and its dont care back into a dont care node
 */
node_t *
simp_sm_to_node(M)
sm_matrix *M;
{
    int i, nin, init_rows, val;
    node_t *node, *fanin_node, **fanin;
    pset setp;
    pset_family F;
    sm_col *pcol;
    sm_row *prow;
    sm_element *p;

    /* delete all rows of the on set from the matrix */
    init_rows = M->rows_size;
    for (i = 0; i < init_rows; i ++) {
        prow = sm_get_row(M, i);
        if ((prow != NULL) && (sm_get(int, prow) == F_SET))
            sm_delrow(M, i);
    }

    /*
     *  Determine the fanin points;
     *  set pcol->flag to be the column number in the set family
     */
    node = node_alloc();
    nin = 0;
    fanin = ALLOC(node_t *, 2*M->ncols);    /* worst case */
    sm_foreach_col(M, pcol) {
        if (pcol->length == 0)
            continue;
        fanin_node = sm_get(node_t *, pcol);
        pcol->flag = nin;
        fanin[nin++] = fanin_node;
    }

    /* Create the set family */
    F = sf_new(M->nrows, nin * 2);
    sm_foreach_row(M, prow) {
        setp = GETSET(F, F->count++);
        (void) set_fill(setp, nin * 2);
        sm_foreach_row_element(prow, p) {
            i = sm_get_col(M, p->col_num)->flag;
            val = sm_get(int, p);
            switch (val) {
            case 0 : 
                PUTINPUT(setp, i, ZERO);
                break;
            case 1 : 
                PUTINPUT(setp, i, ONE);
                break;
            default :
                fail("bad sm_element_data");
            }
        }
    }

    node_replace_internal(node, fanin, nin, F);
    return(node);        
}
