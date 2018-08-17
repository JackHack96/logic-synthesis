
#include "sis.h"
#include "extract_int.h"
#include "heap.h"
#include "fast_extract_int.h"


/* Memory allocation and free for ddivisor_t */

ddivisor_t *
ddivisor_alloc() {
    register ddivisor_t *ddivisor;

    ddivisor = ALLOC(ddivisor_t, 1);

    ddivisor->cube1 = ddivisor->cube2 = NIL(sm_row);
    DTYPE(ddivisor) = Dother;
    ddivisor->weight_status = NEW;
    WEIGHT(ddivisor) = ddivisor->level = 0;
    STATUS(ddivisor) = NONCHECK;
    ddivisor->handle = ddivisor->sthandle = ddivisor->div_index = NULL;

    return ddivisor;
}

void
ddivisor_free(ddivisor)
        register ddivisor_t *ddivisor;
{
    if (ddivisor->div_index) {
        lsDestroy(ddivisor->div_index, 0);
    }
    if (ddivisor->cube1) {
        (void) sm_row_free(ddivisor->cube1);
        (void) sm_row_free(ddivisor->cube2);
    }
    FREE(ddivisor);
}

/* Memory allocation and free for ddivisor_cell_t */

ddivisor_cell_t *
ddivisor_cell_alloc() {
    register ddivisor_cell_t *ddivisor_cell;

    ddivisor_cell = ALLOC(ddivisor_cell_t, 1);

    ddivisor_cell->cube1      = ddivisor_cell->cube2   = NIL(sm_row);
    ddivisor_cell->handle1    = ddivisor_cell->handle2 = NULL;
    ddivisor_cell->baselength = 0;
    DIVISOR(ddivisor_cell) = NIL(ddivisor_t);
    ddivisor_cell->handle = NULL;
    ddivisor_cell->phase  = 0;
    ddivisor_cell->sis_id = -1;

    return ddivisor_cell;
}

void
ddivisor_cell_free(ddivisor_cell)
        ddivisor_cell_t *ddivisor_cell;
{
    FREE(ddivisor_cell);
}

/* Memory allocation and initialization for ddset_t */

ddset_t *
ddset_alloc_init() {

    register ddset_t *ddivisor_set;

    ddivisor_set = ALLOC(ddset_t, 1);

    ddivisor_set->DDset         = lsCreate();
    ddivisor_set->node_cube_set = sm_row_alloc();
    ddivisor_set->D112_set      = sm_alloc();
    ddivisor_set->D222_set      = sm_alloc();
    ddivisor_set->D223_set      = sm_alloc();
    ddivisor_set->D224_set      = sm_alloc();
    ddivisor_set->Dother_set    = sm_alloc();

    return ddivisor_set;
}

static void
table_cleanup(table)
        sm_matrix *table;
{
    register sm_row     *row;
    register sm_element *p;

    sm_foreach_row(table, row)
    {
        sm_foreach_row_element(row, p)
        {
            lsDestroy((lsList) p->user_word, 0);
        }
    }
    (void) sm_free(table);
}


/* Do cleanup after every substitution. */
static void
ddset_cleanup(ddivisor_set)
        ddset_t *ddivisor_set;
{
    register sm_element *p;
    register sm_row     *row;

    (void) table_cleanup(ddivisor_set->D112_set);
    (void) table_cleanup(ddivisor_set->D222_set);
    (void) table_cleanup(ddivisor_set->D223_set);
    (void) table_cleanup(ddivisor_set->D224_set);
    sm_foreach_row(ddivisor_set->Dother_set, row)
    {
        sm_foreach_row_element(row, p)
        {
            (void) table_cleanup((sm_matrix *) p->user_word);
        }
    }
    (void) sm_free(ddivisor_set->Dother_set);
}

void
ddset_free(ddivisor_set)
        ddset_t *ddivisor_set;
{
    lsGen               gen1, gen2;
    ddivisor_t          *divisor;
    ddivisor_cell_t     *dd_cell;
    register sm_element *p;

    lsForeachItem(ddivisor_set->DDset, gen1, divisor)
    {
        lsForeachItem(divisor->div_index, gen2, dd_cell)
        {
            (void) ddivisor_cell_free(dd_cell);
        }
        (void) ddivisor_free(divisor);
    }
    lsDestroy(ddivisor_set->DDset, 0);
    sm_foreach_row_element(ddivisor_set->node_cube_set, p)
    {
        lsDestroy((lsList) p->user_word, 0);
    }
    (void) sm_row_free(ddivisor_set->node_cube_set);
    (void) ddset_cleanup(ddivisor_set);
    FREE(ddivisor_set);
}


/* Memeory alloaction for sm_cube_t */
sm_cube_t *
sm_cube_alloc() {
    register sm_cube_t *sm_cube;

    sm_cube = ALLOC(sm_cube_t, 1);
    sm_cube->div_set    = lsCreate();
    sm_cube->cubehandle = NULL;
    sm_cube->sis_id     = -1;

    return sm_cube;
}

void
sm_cube_free(sm_cube)
        register sm_cube_t *sm_cube;
{
    lsDestroy(sm_cube->div_set, 0);
    FREE(sm_cube);
}


/* Do operation {c1} \ {c2} */
void
clear_row_element(c1, c2)
        register sm_row *c1, *c2;
{
    register sm_element *p;

    sm_foreach_row_element(c2, p)
    {
        (void) sm_row_remove(c1, p->col_num);
    }
}


/* Generate double-cube divisor and store info. in corresponding dd_cell. */

ddivisor_t *
gen_ddivisor(cube1, cube2, dd_cell)
        sm_row *cube1, *cube2;
        ddivisor_cell_t *dd_cell;
{
    sm_row              *base;
    register ddivisor_t *divisor;

    divisor = ddivisor_alloc();
    base    = sm_row_and(cube1, cube2);
    divisor->cube1 = sm_row_dup(cube1);
    divisor->cube2 = sm_row_dup(cube2);

    /* If there is intersection between cube1 and cube2 */

    dd_cell->baselength = base->length;
    if (base->length > 0) {
        (void) clear_row_element(divisor->cube1, base);
        (void) clear_row_element(divisor->cube2, base);
    }
    dd_cell->cube1 = cube1;
    dd_cell->cube2 = cube2;
    (void) sm_row_free(base);

    return divisor;
}

/* Count the number of variables in D222 or D223. */
static int
variable_count(divisor)
        ddivisor_t *divisor;
{
    int count = 4;
    int i, j, t1[2], t2[2];

    t1[0] = (divisor->cube1->first_col->col_num) / 2;
    t1[1] = (divisor->cube1->last_col->col_num) / 2;
    t2[0] = (divisor->cube2->first_col->col_num) / 2;
    t2[1] = (divisor->cube2->last_col->col_num) / 2;
    for (i = 0; i < 2; i++) {
        for (j = 0; j < 2; j++) {
            if (t1[i] == t2[j]) {
                count--;
                break;
            }
        }
    }
    return count;
}


/* Decide the type of divisor and set (key1, key2) */
int
decide_dtype(div)
        register ddivisor_t *div;
{
    sm_row *temp;

    if ((div->cube1->length == 1) && (div->cube2->length == 1)) {
        if (div->cube1->first_col->col_num > div->cube2->first_col->col_num) {
            temp = div->cube1;
            div->cube1 = div->cube2;
            div->cube2 = temp;
        }
        return D112;
    }
    if ((div->cube1->length == 2) && (div->cube2->length == 2)) {
        if (div->cube1->first_col->col_num > div->cube2->first_col->col_num) {
            temp = div->cube1;
            div->cube1 = div->cube2;
            div->cube2 = temp;
        }
        switch (variable_count(div)) {
            case 2 :return D222;
            case 3 :return D223;
            case 4 :return D224;
            default:;
        }
    }
    if (div->cube1->length == div->cube2->length) {
        if (div->cube1->first_col->col_num > div->cube2->first_col->col_num) {
            temp = div->cube1;
            div->cube1 = div->cube2;
            div->cube2 = temp;
        }
    }
    return Dother;
}

/* Dother type divisor check */
static int
check_Dother(div1, div2)
        ddivisor_t *div1, *div2;
{
    if (div1->cube2->first_col->col_num != div2->cube2->first_col->col_num) {
        return 0;
    }
    if (sm_row_compare(div1->cube1, div2->cube1) == 0) {
        if (sm_row_compare(div1->cube2, div2->cube2) == 0) {
            return 1;
        } else {
            return 0;
        }
    }
    return 0;
}

static int
check_D222(div1, div2, phase)
        ddivisor_t *div1, *div2;
        int *phase;
{
    if ((div1->cube1->first_col->col_num == div2->cube1->first_col->col_num) &&
        (div1->cube1->last_col->col_num == div2->cube1->last_col->col_num)) {
        *phase = 0;
    } else {
        *phase = 1;
    }
    return 1;
}


/* D223 divisor check */

static int
check_D223(div1, div2, phase)
        ddivisor_t *div1, *div2;
        int *phase;
{
    int    con1, con2, con3, con4, col;
    sm_row *cube1 = div1->cube1;
    sm_row *cube2 = div1->cube2;
    sm_row *cube3 = div2->cube1;
    sm_row *cube4 = div2->cube2;

    con1 = cube1->first_col->col_num == cube3->first_col->col_num;
    con2 = cube1->last_col->col_num == cube3->last_col->col_num;

    *phase = 0;
    switch (con1 + con2) {
        case 2 :
            if (sm_row_compare(cube2, cube4) == 0) {
                return 1;
            } else {
                return 0;
            }
        case 1 :
            if ((cube2->first_col->col_num / 2 != cube4->first_col->col_num / 2) ||
                (cube2->last_col->col_num / 2 != cube4->last_col->col_num / 2)) {
                return 0;
            }
            col = (con1 == 1) ? cube1->first_col->col_num : cube1->last_col->col_num;
            ((col % 2) == 0) ? col++ : col--;
            if (sm_row_find(cube4, col) == 0) {
                return 0;
            }
            con3 = abs(cube2->first_col->col_num - cube4->first_col->col_num);
            con4 = abs(cube2->last_col->col_num - cube4->last_col->col_num);
            if ((con3 + con4) == 1) {
                *phase = 1;
                return 1;
            }
            return 0;
        case 0 :return 0;
        default:;
    }
}


/* Check if the div1 and div2 are equal, if div1, div2 are belong to 
 * one of D222 or D223, then check the complement too.
 */
int
check_exist(div1, div2, phase)
        ddivisor_t *div1, *div2;
        int *phase;
{

    *phase = 0;

    switch (DTYPE(div1)) {
        case D112 :return 1;
        case D222 :return check_D222(div1, div2, phase);
        case D223 :return check_D223(div1, div2, phase);
        default :return check_Dother(div1, div2);
    }
}


/* Look up the table by the (key1, key2). If not existed, create an element
 * by (key1, key2), return NIL, else return the array_t associated with
 * this element.
 */

static lsList
lookup_ddset(divisor, table, key1, key2)
        ddivisor_t *divisor;
        sm_matrix *table;
        int key1, key2;
{
    register sm_element *p1, *p2;
    lsHandle            handle;

    if ((p1 = sm_find(table, key1, key2)) == NIL(sm_element)) {
        p2 = sm_insert(table, key1, key2);
        p2->user_word = (char *) lsCreate();
        lsNewEnd((lsList) p2->user_word, (lsGeneric) divisor, &handle);
        divisor->sthandle = handle;
        return NULL;
    } else {
        if (lsLength((lsList) p1->user_word) == 0) {
            lsNewEnd((lsList) p1->user_word, (lsGeneric) divisor, &handle);
            divisor->sthandle = handle;
            return NULL;
        } else {
            return (lsList) p1->user_word;
        }
    }
}

static lsList
lookup_ddset_other(divisor, table)
        ddivisor_t *divisor;
        sm_matrix *table;
{
    register sm_element *p1, *p2, *p3;
    lsHandle            handle;
    int                 key1 = divisor->cube1->length;
    int                 key2 = divisor->cube2->length;

    if ((p1 = sm_find(table, key1, key2)) == NIL(sm_element)) {
        p2 = sm_insert(table, key1, key2);
        p2->user_word = (char *) sm_alloc();
        p3 = sm_insert((sm_matrix *) p2->user_word,
                       divisor->cube1->last_col->col_num,
                       divisor->cube2->last_col->col_num);
        p3->user_word = (char *) lsCreate();
        lsNewEnd((lsList) p3->user_word, (lsGeneric) divisor, &handle);
        divisor->sthandle = handle;
        return NULL;
    } else {
        if ((p2 = sm_find((sm_matrix *) p1->user_word,
                          divisor->cube1->last_col->col_num,
                          divisor->cube2->last_col->col_num)) == NIL(sm_element)) {

            p3 = sm_insert((sm_matrix *) p1->user_word,
                           divisor->cube1->last_col->col_num,
                           divisor->cube2->last_col->col_num);
            p3->user_word = (char *) lsCreate();
            lsNewEnd((lsList) p3->user_word, (lsGeneric) divisor, &handle);
            divisor->sthandle = handle;
            return NULL;
        } else {
            if (lsLength((lsList) p2->user_word) == 0) {
                lsNewEnd((lsList) p2->user_word, (lsGeneric) divisor, &handle);
                divisor->sthandle = handle;
                return NULL;
            } else {
                return (lsList) p2->user_word;
            }
        }
    }
}


void
hash_ddset_table(divisor, check_set)
        ddivisor_t *divisor;
        lsList check_set;
{
    lsHandle handle;

    if (check_set) {
        lsNewEnd(check_set, (lsGeneric) divisor, &handle);
        divisor->sthandle = handle;
    }
}


/* Choose corresponding check_set by (Dtype, key1, key2) */

lsList
choose_check_set(divisor, ddivisor_set)
        register ddivisor_t *divisor;
        ddset_t *ddivisor_set;
{
    lsList check_set;
    int    key1, key2;

    switch (DTYPE(divisor)) {
        case D112 :key1 = divisor->cube1->first_col->col_num;
            key2        = divisor->cube2->last_col->col_num;
            check_set   = lookup_ddset(divisor, ddivisor_set->D112_set, key1, key2);
            break;
        case D222 :key1 = (divisor->cube1->first_col->col_num) / 2;
            key2        = (divisor->cube1->last_col->col_num) / 2;
            check_set   = lookup_ddset(divisor, ddivisor_set->D222_set, key1, key2);
            break;
        case D223 :key1 = (divisor->cube1->first_col->col_num) / 2;
            key2        = (divisor->cube1->last_col->col_num) / 2;
            check_set   = lookup_ddset(divisor, ddivisor_set->D223_set, key1, key2);
            break;
        case D224 :key1 = divisor->cube1->last_col->col_num;
            key2        = divisor->cube2->last_col->col_num;
            check_set   = lookup_ddset(divisor, ddivisor_set->D224_set, key1, key2);
            break;
        case Dother :check_set = lookup_ddset_other(divisor, ddivisor_set->Dother_set);
            break;
        default:;
    }

    return (lsList) check_set;
}

/* If the divisor has already been in the set of double cube divisor,
 * then store the information which indicates where the divisor comes
 * from.
 * else store the divisor in the double-cube divisor set.
 */

ddivisor_t *
check_append(div, ddivisor_set, dd_cell)
        register ddivisor_t *div;
        ddset_t *ddivisor_set;
        ddivisor_cell_t *dd_cell;
{
    int        phase = 0;
    ddivisor_t *dd;
    lsGen      gen;
    lsHandle   handle1, handle2;
    lsList     check_set;

    /* We only check the complement of D222, or D223 */

    DTYPE(div)       = decide_dtype(div);

    /* if empty in the double cube divisor set, then create the set. */

    if (lsLength(ddivisor_set->DDset) == 0) {
        check_set = choose_check_set(div, ddivisor_set);
        (void) hash_ddset_table(div, check_set);
        lsNewEnd(ddivisor_set->DDset, (lsGeneric) div, &handle1);
        div->handle    = handle1;
        div->div_index = lsCreate();
        lsNewEnd(div->div_index, (lsGeneric) dd_cell, &handle2);
        dd_cell->handle = handle2;
        return div;
    }

    /* if not empty in the double cube divisor set, then check if the 
     * divisor exists in the set.
     */

    check_set = choose_check_set(div, ddivisor_set);
    if (check_set) {
        lsForeachItem(check_set, gen, dd)
        {
            if (check_exist(div, dd, &(phase)) == 1) {
                dd_cell->phase = phase;
                lsNewEnd(dd->div_index, (lsGeneric) dd_cell, &handle2);
                dd_cell->handle = handle2;
                (void) ddivisor_free(div);
                lsFinish(gen);
                return dd;
            }
        }
    }

    /* for check_set == NULL and not found */
    (void) hash_ddset_table(div, check_set);
    lsNewEnd(ddivisor_set->DDset, (lsGeneric) div, &handle1);
    div->handle    = handle1;
    div->div_index = lsCreate();
    lsNewEnd(div->div_index, (lsGeneric) dd_cell, &handle2);
    dd_cell->handle = handle2;
    return div;
}


/* Extract double-cube divisors, and put them in the ddivisor_set.
 * Index represents unique number to represent node in sis.
 * Row represents current row in sparse matrix.
 */
int
extract_ddivisor(B, index, row, ddivisor_set)
        sm_matrix *B;
        int index;
        int row;
        ddset_t *ddivisor_set;
{
    sm_row          *cube1, *cube2;
    sm_element      *pnode;
    ddivisor_t      *divisor, *div;
    ddivisor_cell_t *dd_cell;
    sm_cube_t       *sm_cube1, *sm_cube2;
    lsHandle        handle, handle1, handle2;
    int             i, j;

    extern int twocube_timeout_occured;

    if ((B->nrows - row) == 1) {
        cube1    = sm_get_row(B, row);
        sm_cube1 = sm_cube_alloc();
        sm_cube1->sis_id = index;
        cube1->user_word = (char *) sm_cube1;
        return 0;
    }

    pnode = sm_row_insert(ddivisor_set->node_cube_set, index);
    pnode->user_word = (char *) lsCreate();

    /* Store all cubes in the node */
    for (i = row; i < B->nrows; i++) {
        cube1 = sm_get_row(B, i);
        lsNewEnd((lsList) pnode->user_word, (lsGeneric) cube1, &handle);
        sm_cube1 = sm_cube_alloc();
        sm_cube1->sis_id     = index;
        sm_cube1->cubehandle = handle;
        cube1->user_word     = (char *) sm_cube1;
    }

    /* Generate double-cube divisors. */
    for (i = row; i < B->nrows - 1; i++) {
        cube1    = sm_get_row(B, i);
        sm_cube1 = (sm_cube_t *) cube1->user_word;
        for (j   = i + 1; j < B->nrows; j++) {
            cube2   = sm_get_row(B, j);
            dd_cell = ddivisor_cell_alloc();
            dd_cell->sis_id = index;

            /* Without loss of generality, we can let the literals in
             * the first cube is less than that of the second cube.
             */
            if (cube1->length < cube2->length) {
                divisor = gen_ddivisor(cube1, cube2, dd_cell);
            } else {
                divisor = gen_ddivisor(cube2, cube1, dd_cell);
            }
            if (FX_DELETE == 1) {
                if ((divisor->cube1->length > LENGTH1) ||
                    (divisor->cube2->length > LENGTH2)) {
                    (void) ddivisor_cell_free(dd_cell);
                    (void) ddivisor_free(divisor);
                    continue;
                }
            }
            div = check_append(divisor, ddivisor_set, dd_cell);
            DIVISOR(dd_cell) = div;
            sm_cube2 = (sm_cube_t *) cube2->user_word;

            lsNewEnd(sm_cube1->div_set, (lsGeneric) dd_cell, &handle1);
            dd_cell->handle1 = handle1;
            lsNewEnd(sm_cube2->div_set, (lsGeneric) dd_cell, &handle2);
            dd_cell->handle2 = handle2;

            if (twocube_timeout_occured) {
                /* Enough time has elapsed... do not generate any more */
                return;
            }
        }
    }
}

