
#include "extract_int.h"
#include "fast_extract_int.h"
#include "heap.h"
#include "sis.h"

static int debugging = 1;

static void sd_modify();

/* Map a network node to a sparse matrix representation. */
void fx_node_to_sm(register node_t *node, register sm_matrix *M) {
    register pset last, p;
    register int  i, row;
    int           *fanin_index;
    node_t        *fanin;
    sm_element    *pelement;

    foreach_set(node->F, last, p) {
        row = M->nrows;

        fanin_index = ALLOC(int, node->nin);
        foreach_fanin(node, i, fanin) {
            fanin_index[i] = 2 * nodeindex_indexof(global_node_index, fanin);
        }

        foreach_fanin(node, i, fanin) {
            switch (GETINPUT(p, i)) {
                case ZERO:pelement = sm_insert(M, row, fanin_index[i] + 1);
                    break;
                case ONE:pelement = sm_insert(M, row, fanin_index[i]);
                    break;
                case TWO:pelement = NIL(sm_element);
                    break;
            }
        }

        FREE(fanin_index);
    }
}

/* Map a sparse matrix representation to a network node. */
node_t *fx_sm_to_node(sm_matrix *func) {
    int         i, nin;
    node_t      *node, *fanin_node, **fanin;
    pset        setp, fullset;
    pset_family F;
    sm_col      *pcol;
    sm_row      *prow;
    sm_element  *p;

    /*
     *  Determine the fanin points;
     *  set pcol->flag to be the column number in the set family
     */
    node  = node_alloc();
    nin   = 0;
    fanin = ALLOC(node_t *, 2 * func->ncols);
    sm_foreach_col(func, pcol) {
        fanin_node = nodeindex_nodeof(global_node_index, pcol->col_num / 2);
        if ((pcol->col_num % 2) == 0) {
            pcol->flag = 2 * nin;
            fanin[nin++] = fanin_node;
        } else {
            if (nin == 0 || fanin[nin - 1] != fanin_node) {
                pcol->flag = 2 * nin + 1;
                fanin[nin++] = fanin_node;
            } else {
                pcol->flag = 2 * (nin - 1) + 1;
            }
        }
    }

    /* Create the set family */
    F       = sf_new(func->nrows, nin * 2);
    fullset = set_full(nin * 2);
    sm_foreach_row(func, prow) {
        setp = GETSET(F, F->count++);
        (void) set_clear(setp, nin * 2);
        sm_foreach_row_element(prow, p) {
            i = sm_get_col(func, p->col_num)->flag;
            set_insert(setp, i);
        }
        (void) set_diff(setp, fullset, setp);
    }
    set_free(fullset);
    node_replace_internal(node, fanin, nin, F);
    return node;
}

/* Divide each fanout */
static int fx_divide_each_fanout(array_t *fanout, node_t *newnode, int complement) {
    int    i, changed, count, fanout_index;
    node_t *node;

    if (debugging > 1) {
        (void) fprintf(sisout, "\nnew factor is");
        (void) node_print(sisout, newnode);
    }

    for (i = count = 0; i < array_n(fanout); i++) {
        fanout_index = array_fetch(int, fanout, i);
        node         = nodeindex_nodeof(global_node_index, fanout_index);
        changed      = node_substitute(node, newnode, complement);
        count += changed;
    }
    return count;
}

/* Map  D112 to rectangle data structure. */
static rect_t *D112_to_S2(sm_matrix *B, ddivisor_t *divisor) {
    rect_t       *rect;
    sm_col       *col1, *col2, *col3;
    register int col_num1 = divisor->cube1->first_col->col_num;
    register int col_num2 = divisor->cube2->first_col->col_num;

    (col_num1 % 2 == 0) ? col_num1++ : col_num1--;
    (col_num2 % 2 == 0) ? col_num2++ : col_num2--;
    col1 = sm_get_col(B, col_num1);
    col2 = sm_get_col(B, col_num2);
    if ((col1) && (col2)) {
        col3 = sm_col_and(col1, col2);
        if (col3->length == 0) {
            (void) sm_col_free(col3);
            return NIL(rect_t);
        } else {
            rect = ALLOC(rect_t, 1);
            rect->cols = sm_row_alloc();
            rect->rows = col3;
            sm_row_insert(rect->cols, col_num1);
            sm_row_insert(rect->cols, col_num2);
            return rect;
        }
    } else {
        return NIL(rect_t);
    }
}

/* Calculate the weight of D112 divisor. */
static int calc_weight_D112(sm_matrix *B, ddivisor_t *divisor) {
    sm_col       *col1, *col2, *col3;
    register int col_num1 = divisor->cube1->first_col->col_num;
    register int col_num2 = divisor->cube2->first_col->col_num;
    int          weight;

    (col_num1 % 2 == 0) ? col_num1++ : col_num1--;
    (col_num2 % 2 == 0) ? col_num2++ : col_num2--;
    col1 = sm_get_col(B, col_num1);
    col2 = sm_get_col(B, col_num2);
    if ((col1) && (col2)) {
        col3   = sm_col_and(col1, col2);
        weight = col3->length;
        (void) sm_col_free(col3);
        return weight;
    } else {
        return 0;
    }
}

/* Calculate the weight of each double-cube divisor.
 * Choose the best divisor from double-cube divisor set.
 */
static ddivisor_t *choose_best_divisor(sm_matrix *B, lsList ddivisor_set, int *wdmax) {
    int             maxweight = -1;
    int             weight;
    char            *data;
    lsGen           gen1, gen2;
    ddivisor_t      *divisor, *bestdivisor;
    ddivisor_cell_t *dd_cell;

    if (DONT_USE_WEIGHT_ZERO) {
        maxweight = 0;
    }
    bestdivisor               = NIL(ddivisor_t);
    lsForeachItem(ddivisor_set, gen1, divisor) {
        /* If empty, then remove it out. */
        if (lsLength(divisor->div_index) == 0) {
            lsRemoveItem(divisor->handle, &data);
            lsRemoveItem(divisor->sthandle, &data);
            (void) ddivisor_free(divisor);
            continue;
        }

        /* If divisor is D112, calculate the weight of its complement. */
        if (DTYPE(divisor) == D112) {
            weight = calc_weight_D112(B, divisor);
        } else {
            weight = 0;
        }

        /* If weight doesn't need to recalculate, then skip. */
        switch (divisor->weight_status) {
            register int w;
            case CHANGED:
            case NEW:
                w = (lsLength(divisor->div_index) - 1) *
                    (divisor->cube1->length + divisor->cube2->length);
                lsForeachItem(divisor->div_index, gen2, dd_cell) {
                    w += dd_cell->baselength;
                }
                w -= lsLength(divisor->div_index);
                WEIGHT(divisor) = w;
                divisor->weight_status = OLD;
            case OLD:weight += WEIGHT(divisor);
                break;
            default:;
        }

        /* Choose the best double-cube divisor, and throw unworthy candidates */
        if (weight < DONT_USE_WEIGHT_ZERO) {
            lsRemoveItem(divisor->handle, &data);
            lsRemoveItem(divisor->sthandle, &data);
            lsForeachItem(divisor->div_index, gen2, dd_cell) {
                lsRemoveItem(dd_cell->handle1, &data);
                lsRemoveItem(dd_cell->handle2, &data);
                (void) ddivisor_cell_free(dd_cell);
            }
            (void) ddivisor_free(divisor);
        } else {
            STATUS(divisor) = NONCHECK;
            if (maxweight < weight) {
                maxweight   = weight;
                bestdivisor = divisor;
            }
        }
    }

    *wdmax = maxweight;
    return bestdivisor;
}

/* exported function (used in gen_2c_kernel.c)
 * compute the weight of a 2-cube divisor
 */
int fx_compute_divisor_weight(sm_matrix *func, ddivisor_t *divisor) {
    int             weight, w;
    lsGen           gen;
    ddivisor_cell_t *dd_cell;

    /* If divisor is D112, calculate the weight of its complement */
    if (DTYPE(divisor) == D112) {
        weight = calc_weight_D112(func, divisor);
    } else {
        weight = 0;
    }
    w = (lsLength(divisor->div_index) - 1) *
        (divisor->cube1->length + divisor->cube2->length);
    lsForeachItem(divisor->div_index, gen, dd_cell) { w += dd_cell->baselength; }
    w -= lsLength(divisor->div_index);

    weight += w;

    return weight;
}

/* Finding the fanouts of the double-cube divisor */
static array_t *find_ddivisor_fanout(ddivisor_t *divisor) {
    ddivisor_cell_t *dd_cell;
    st_table        *fanout_table;
    st_generator    *gen;
    lsGen           gen1;
    array_t         *fanouts;
    char            *key;

    /*
     *  Find all of the functions (and cubes) it fans out to
     */

    fanout_table = st_init_table(st_numcmp, st_numhash);

    lsForeachItem(divisor->div_index, gen1, dd_cell) {
        key = (char *) dd_cell->sis_id;
        (void) st_insert(fanout_table, key, NIL(char));
    }
    fanouts      = array_alloc(char *, 0);
    st_foreach_item(fanout_table, gen, &key, NIL(
            char *)) {
        array_insert_last(char *, fanouts, key);
    }
    (void) st_free_table(fanout_table);

    return fanouts;
}

/* Finding the fanouts of the single-cube divisor */
static array_t *find_sdivisor_fanout(sm_matrix *B, rect_t *rect) {
    register sm_element *p;
    sm_row              *row;
    sm_cube_t           *sm_cube;
    st_table            *fanout_table;
    st_generator        *gen;
    array_t             *fanouts;
    char                *key;

    /* Find all of the functions it fans out to */

    fanout_table = st_init_table(st_numcmp, st_numhash);
    sm_foreach_col_element(rect->rows, p) {
        row     = sm_get_row(B, p->row_num);
        sm_cube = (sm_cube_t *) row->user_word;
        key     = (char *) sm_cube->sis_id;
        (void) st_insert(fanout_table, key, NIL(char));
    }

    fanouts = array_alloc(char *, 0);
    st_foreach_item(fanout_table, gen, &key, NIL(
            char *)) {
        (void) array_insert_last(char *, fanouts, key);
    }
    (void) st_free_table(fanout_table);

    return fanouts;
}

/* Update the sparse matrix after substituting single cube divisor */
static void sd_update_matrix(sm_matrix *B, rect_t *rect) {
    register sm_element *p1, *p2, *p;

    /* Clear rectangle */
    sm_foreach_col_element(rect->rows, p1) {
        sm_foreach_row_element(rect->cols, p2) {
            p = sm_find(B, p1->row_num, p2->col_num);
            sm_remove_element(B, p);
        }
    }
}

/* Reexpress network after substituting a singl-cube divisor.
 * dd112_on  : indicate either a single-cube divisor or a complement of D112.
 * index     : sis_id of the current divisor.
 * newcolumn : new column number in the sparse matrix.
 */
static void sd_reexpress(sm_matrix *B, rect_t *rect, int dd112_on, int index, int *newcolumn) {
    sm_element *p;
    sm_matrix  *func;
    array_t    *fanout;
    int        new_colnum, new_index, count;
    node_t     *node;

    func = sm_alloc();
    sm_copy_row(func, 0, rect->cols);
    if (dd112_on == 0) {
        node = fx_sm_to_node(func);
    } else {
        node = nodeindex_nodeof(global_node_index, index);
    }

    fanout = find_sdivisor_fanout(B, rect);
    count  = fx_divide_each_fanout(fanout, node, dd112_on);

    if ((debugging) && (count != array_n(fanout))) {
        (void) fprintf(sisout, "warning: divided only %d of %d\n", count,
                       array_n(fanout));
        (void) fprintf(sisout, "In sd_reexpress\n");
    }

    if (dd112_on == 0) {
        if (count > 0) {
            (void) network_add_node(global_network, node);
            new_index = nodeindex_insert(global_node_index, node);
        } else {
            node_free(node);
            new_index = -1;
        }
    } else {
        new_index = index;
    }

    (void) array_free(fanout);
    (void) sm_free(func);

    /* Create new column in the sm and insert in proper positions */
    *newcolumn = new_colnum = new_index * 2 + dd112_on;
    sm_foreach_col_element(rect->rows, p) {
        (void) sm_insert(B, p->row_num, new_colnum);
    }
}

/* Doing the substitution in sparse matrix and reexpression in sis for
 * double-cube divisor. Return an array which stores the new rows.
 * node_index stores the sis_id of this new node.
 */
static array_t *dd_reexpress(sm_matrix *B, ddivisor_t *divisor, int *node_index) {
    sm_matrix       *func;
    array_t         *fanout, *newcubes;
    int             row, new_colnum, new_index, count, complement;
    sm_row          *cube, *base;
    sm_cube_t       *sm_cube;
    lsGen           gen;
    node_t          *node;
    ddivisor_cell_t *dd_cell;

    /* reexpression */

    func = sm_alloc();
    (void) sm_copy_row(func, 0, divisor->cube1);
    (void) sm_copy_row(func, 1, divisor->cube2);
    node   = fx_sm_to_node(func);
    fanout = find_ddivisor_fanout(divisor);

    complement = ((DTYPE(divisor) == D222) || (DTYPE(divisor) == D223));
    count      = fx_divide_each_fanout(fanout, node, complement);

    if ((debugging) && (count != array_n(fanout))) {
        (void) fprintf(sisout, "warning: divided only %d of %d\n", count,
                       array_n(fanout));
        (void) fprintf(sisout, "In dd_reexpress\n");
    }
    (void) array_free(fanout);
    (void) sm_free(func);

    if (count > 0) {
        (void) network_add_node(global_network, node);
        new_index = nodeindex_insert(global_node_index, node);
    } else {
        (void) node_free(node);
        new_index = -1;
    }
    if (DTYPE(divisor) == D112)
        *node_index = new_index;

    /* add a new column */

    new_colnum = new_index * 2;
    newcubes   = array_alloc(sm_row *, 0);

    /* Update the sparse matrix after substituting double-cube divisor. */

    row = B->last_row->row_num;
    (void) sm_copy_row(B, ++row, divisor->cube1);
    cube    = sm_get_row(B, row);
    sm_cube = sm_cube_alloc();
    sm_cube->sis_id = new_index;
    cube->user_word = (char *) sm_cube;
    row = B->last_row->row_num;
    (void) sm_copy_row(B, ++row, divisor->cube2);
    cube    = sm_get_row(B, row);
    sm_cube = sm_cube_alloc();
    sm_cube->sis_id = new_index;
    cube->user_word = (char *) sm_cube;

    lsForeachItem(divisor->div_index, gen, dd_cell) {
        base = sm_row_and(dd_cell->cube1, dd_cell->cube2);
        (void) sm_copy_row(B, ++row, base);
        (void) sm_row_free(base);
        sm_cube = sm_cube_alloc();
        (void) sm_insert(B, row, new_colnum + dd_cell->phase);
        cube = sm_get_row(B, row);
        sm_cube->sis_id = dd_cell->sis_id;
        cube->user_word = (char *) sm_cube;
        (void) array_insert_last(sm_row *, newcubes, cube);
    }
    return newcubes;
}

/* Recompute the weight of double-cube divisors after substituting double
 * cube divisor. newcubes stores new rows.
 */
static void dd_modify(sm_matrix *B, ddivisor_t *div, int node_index, array_t *newcubes, ddset_t *ddivisor_set) {
    lsGen           gen1, gen2;
    sm_row          *cube1, *cube2;
    sm_element      *pnode;
    sm_cube_t       *sm_cube1, *sm_cube2;
    ddivisor_t      *divisor, *divisor1;
    ddivisor_cell_t *dd1, *dd2, *dd_cell;
    array_t         *del_set;
    lsHandle        handle, handle1, handle2;
    rect_t          *rect;
    char            *data;
    int             i, j, newcolumn;
    int             phase, found;
    lsList          check_set;

    /* del_set : the ddivisor_cells needed to be discarded. */
    del_set = array_alloc(ddivisor_cell_t *, 0);

    lsForeachItem(div->div_index, gen1, dd1) {
        sm_cube1 = (sm_cube_t *) dd1->cube1->user_word;
        sm_cube2 = (sm_cube_t *) dd1->cube2->user_word;
        lsRemoveItem(dd1->handle1, &data);
        lsRemoveItem(dd1->handle2, &data);
        (void) array_insert_last(ddivisor_cell_t *, del_set, dd1);
        lsForeachItem(sm_cube1->div_set, gen2, dd2) {
            lsRemoveItem(dd2->handle1, &data);
            lsRemoveItem(dd2->handle2, &data);
            (void) array_insert_last(ddivisor_cell_t *, del_set, dd2);
        }
        lsForeachItem(sm_cube2->div_set, gen2, dd2) {
            lsRemoveItem(dd2->handle1, &data);
            lsRemoveItem(dd2->handle2, &data);
            (void) array_insert_last(ddivisor_cell_t *, del_set, dd2);
        }
        lsRemoveItem(sm_cube1->cubehandle, &data);
        lsRemoveItem(sm_cube2->cubehandle, &data);
        (void) sm_cube_free(sm_cube1);
        (void) sm_cube_free(sm_cube2);
        (void) sm_delrow(B, dd1->cube1->row_num);
        (void) sm_delrow(B, dd1->cube2->row_num);
    }

    if (ONE_PASS == 0) {
        for (i = 0; i < array_n(del_set); i++) {
            dd_cell = array_fetch(ddivisor_cell_t *, del_set, i);
            divisor = DIVISOR(dd_cell);
            if (divisor != div) {
                lsRemoveItem(dd_cell->handle, &data);
                if (lsLength(divisor->div_index) == 0) {
                    lsRemoveItem(divisor->sthandle, &data);
                    lsRemoveItem(divisor->handle, &data);
                    (void) ddivisor_free(divisor);
                } else {
                    divisor->weight_status = CHANGED;
                }
                (void) ddivisor_cell_free(dd_cell);
            } else {
                lsRemoveItem(dd_cell->handle, &data);
                (void) ddivisor_cell_free(dd_cell);
            }
        }
    } else {
        for (i = 0; i < array_n(del_set); i++) {
            dd_cell = array_fetch(ddivisor_cell_t *, del_set, i);
            divisor = DIVISOR(dd_cell);
            lsRemoveItem(dd_cell->handle, &data);
            divisor->weight_status = CHANGED;
            (void) ddivisor_cell_free(dd_cell);
        }
    }

    (void) array_free(del_set);

    if (DTYPE(div) == D112) {
        rect = D112_to_S2(B, div);
        lsRemoveItem(div->sthandle, &data);
        lsRemoveItem(div->handle, &data);
        (void) ddivisor_free(div);
        if (rect) {
            (void) sd_reexpress(B, rect, 1, node_index, &newcolumn);
            (void) sd_update_matrix(B, rect);
            (void) sd_modify(B, rect, ddivisor_set, newcolumn);
            (void) rect_free(rect);
        }
    } else {
        lsRemoveItem(div->sthandle, &data);
        lsRemoveItem(div->handle, &data);
        (void) ddivisor_free(div);
    }
    if (ONE_PASS == 0) {
        for (i = 0; i < array_n(newcubes); i++) {
            cube1    = array_fetch(sm_row *, newcubes, i);
            sm_cube1 = (sm_cube_t *) cube1->user_word;
            pnode    = sm_row_find(ddivisor_set->node_cube_set, sm_cube1->sis_id);
            if (lsLength((lsList) pnode->user_word) == 0) {
                lsNewEnd((lsList) pnode->user_word, (lsGeneric) cube1, &handle);
                sm_cube1->cubehandle = handle;
                continue;
            }
            lsForeachItem((lsList) pnode->user_word, gen1, cube2) {
                sm_cube2 = (sm_cube_t *) cube2->user_word;
                dd_cell  = ddivisor_cell_alloc();
                dd_cell->sis_id = sm_cube2->sis_id;
                if (cube1->length < cube2->length) {
                    divisor = gen_ddivisor(cube1, cube2, dd_cell);
                } else {
                    divisor = gen_ddivisor(cube2, cube1, dd_cell);
                }
                divisor1        = check_append(divisor, ddivisor_set, dd_cell);
                DIVISOR(dd_cell) = divisor1;
                lsNewEnd(sm_cube1->div_set, (lsGeneric) dd_cell, &handle1);
                dd_cell->handle1 = handle1;
                lsNewEnd(sm_cube2->div_set, (lsGeneric) dd_cell, &handle2);
                dd_cell->handle2 = handle2;
            }
            lsNewEnd((lsList) pnode->user_word, (lsGeneric) cube1, &handle);
            sm_cube1->cubehandle = handle;
        }
    } else {
        for (i = 0; i < array_n(newcubes); i++) {
            cube1    = array_fetch(sm_row *, newcubes, i);
            sm_cube1 = (sm_cube_t *) cube1->user_word;
            pnode    = sm_row_find(ddivisor_set->node_cube_set, sm_cube1->sis_id);
            lsNewEnd((lsList) pnode->user_word, (lsGeneric) cube1, &handle);
            sm_cube1->cubehandle = handle;
            for (j = i + 1; j < array_n(newcubes); j++) {
                cube2    = array_fetch(sm_row *, newcubes, j);
                sm_cube2 = (sm_cube_t *) cube2->user_word;
                if (sm_cube1->sis_id == sm_cube2->sis_id) {
                    dd_cell        = ddivisor_cell_alloc();
                    dd_cell->sis_id = sm_cube2->sis_id;
                    if (cube1->length < cube2->length) {
                        divisor = gen_ddivisor(cube1, cube2, dd_cell);
                    } else {
                        divisor = gen_ddivisor(cube2, cube1, dd_cell);
                    }
                    DTYPE(divisor) = decide_dtype(divisor);
                    found     = 0;
                    check_set = choose_check_set(divisor, ddivisor_set);
                    if (check_set) {
                        lsForeachItem(check_set, gen1, divisor1) {
                            if (check_exist(divisor, divisor1, &phase) == 1) {
                                dd_cell->phase = phase;
                                lsNewEnd(divisor1->div_index, (lsGeneric) dd_cell, &handle);
                                dd_cell->handle = handle;
                                DIVISOR(dd_cell) = divisor1;
                                lsNewEnd(sm_cube1->div_set, (lsGeneric) dd_cell, &handle1);
                                dd_cell->handle1 = handle1;
                                lsNewEnd(sm_cube2->div_set, (lsGeneric) dd_cell, &handle2);
                                dd_cell->handle2 = handle2;
                                found = 1;
                                lsFinish(gen1);
                                break;
                            }
                        }
                        if (found == 0)
                            (void) ddivisor_cell_free(dd_cell);
                        (void) ddivisor_free(divisor);
                    } else {
                        lsRemoveItem(divisor->sthandle, &data);
                        (void) ddivisor_free(divisor);
                        (void) ddivisor_cell_free(dd_cell);
                    }
                }
            }
        }
    }

    (void) array_free(newcubes);
}

/* Reconstruct old double-cube divisors after substituting a single-cube
 * divisor.
 */
static void sd_mod_old_divisor(ddivisor_t *divisor, ddset_t *ddivisor_set) {
    ddivisor_cell_t *cell;
    sm_row          *cube, *base;
    lsList          list;
    lsHandle        handle;
    char            *data;

    if (lsLength(divisor->div_index) == 0) {
        lsRemoveItem(divisor->handle, &data);
        (void) ddivisor_free(divisor);
    } else {
        lsFirstItem(divisor->div_index, (lsGeneric *) &cell, &handle);
        (void) sm_row_free(divisor->cube1);
        (void) sm_row_free(divisor->cube2);
        divisor->cube1 = sm_row_dup(cell->cube1);
        divisor->cube2 = sm_row_dup(cell->cube2);
        base = sm_row_and(divisor->cube1, divisor->cube2);
        (void) clear_row_element(divisor->cube1, base);
        (void) clear_row_element(divisor->cube2, base);
        (void) sm_row_free(base);
        if (divisor->cube1->length > divisor->cube2->length) {
            cube = divisor->cube1;
            divisor->cube1 = divisor->cube2;
            divisor->cube2 = cube;
        }
        DTYPE(divisor) = decide_dtype(divisor);
        list = choose_check_set(divisor, ddivisor_set);
        (void) hash_ddset_table(divisor, list);
    }
}

/* Create new double-cube divisors after substituting a single-cube divisor. */
static void sd_mod_new_divisor(ddivisor_t *divisor, ddset_t *ddivisor_set) {
    ddivisor_cell_t *cell;
    sm_row          *cube, *base;
    lsList          list;
    lsHandle        handle;

    if (lsLength(divisor->div_index) == 0) {
        lsDestroy(divisor->div_index, 0);
        FREE(divisor);
    } else {
        lsFirstItem(divisor->div_index, (lsGeneric *) &cell, &handle);
        divisor->cube1 = sm_row_dup(cell->cube1);
        divisor->cube2 = sm_row_dup(cell->cube2);
        base = sm_row_and(divisor->cube1, divisor->cube2);
        (void) clear_row_element(divisor->cube1, base);
        (void) clear_row_element(divisor->cube2, base);
        (void) sm_row_free(base);
        lsNewEnd(ddivisor_set->DDset, (lsGeneric) divisor, &handle);
        divisor->handle = handle;
        if (divisor->cube1->length > divisor->cube2->length) {
            cube = divisor->cube1;
            divisor->cube1 = divisor->cube2;
            divisor->cube2 = cube;
        }
        DTYPE(divisor) = decide_dtype(divisor);
        list = choose_check_set(divisor, ddivisor_set);
        (void) hash_ddset_table(divisor, list);
    }
}

/* The extracted single-cube divisor is in between base and divisor.
 * There are three possiblities for D223 and D223, i.e., the old divisor, and
 * two new divisors.
 * There are two possiblities for D112,D222 or Dother, i.e,, the old divisor,
 * and a new divisor.
 */
static void sd_mod_INBETWEEN(ddivisor_cell_t *dd_cell, ddset_t *ddivisor_set, int colnum) {
    ddivisor_t      *olddivisor, *divisor, *divisor1;
    lsHandle        handle;
    char            *data;
    lsGen           gen;
    ddivisor_cell_t *cell;

    olddivisor = DIVISOR(dd_cell);
    divisor    = ddivisor_alloc();
    divisor->div_index = lsCreate();
    STATUS(divisor) = INBETWEEN;
    olddivisor->weight_status = CHANGED;

    switch (DTYPE(olddivisor)) {
        case D222:
        case D223:divisor1 = ddivisor_alloc();
            divisor1->div_index = lsCreate();
            STATUS(divisor1) = INBETWEEN;
            lsForeachItem(olddivisor->div_index, gen, cell) {
                if ((sm_row_find(cell->cube1, colnum) != 0) ||
                    (sm_row_find(cell->cube2, colnum) != 0)) {
                    lsRemoveItem(cell->handle, &data);
                    (cell->baselength)--;
                    if (cell->phase == 0) {
                        DIVISOR(cell) = divisor;
                        lsNewEnd(divisor->div_index, (lsGeneric) cell, &handle);
                    } else {
                        DIVISOR(cell) = divisor1;
                        cell->phase = 0;
                        lsNewEnd(divisor1->div_index, (lsGeneric) cell, &handle);
                    }
                    cell->handle = handle;
                }
            }
            if (lsLength(olddivisor->div_index) == 0) {
                lsRemoveItem(olddivisor->sthandle, &data);
                lsRemoveItem(olddivisor->handle, &data);
                (void) ddivisor_free(olddivisor);
            }
            (void) sd_mod_new_divisor(divisor, ddivisor_set);
            (void) sd_mod_new_divisor(divisor1, ddivisor_set);
            return;
        default:lsForeachItem(olddivisor->div_index, gen, cell) {
                if ((sm_row_find(cell->cube1, colnum) != 0) ||
                    (sm_row_find(cell->cube2, colnum) != 0)) {
                    (cell->baselength)--;
                    lsRemoveItem(cell->handle, &data);
                    lsNewEnd(divisor->div_index, (lsGeneric) cell, &handle);
                    cell->handle = handle;
                    DIVISOR(cell) = divisor;
                }
            }
            if (lsLength(olddivisor->div_index) == 0) {
                lsRemoveItem(olddivisor->sthandle, &data);
                lsRemoveItem(olddivisor->handle, &data);
                (void) ddivisor_free(olddivisor);
            }
            (void) sd_mod_new_divisor(divisor, ddivisor_set);
            return;
    }
}

/* The extracted single-cube divisor is in divisor part.
 * There are two possiblities for D222, D223, i.e., two new divisors.
 * There are one possiblities for D224 or Dother, i.e., one new divisor.
 */
static void sd_mod_INDIVISOR(ddivisor_cell_t *dd_cell, ddset_t *ddivisor_set) {

    ddivisor_t      *divisor, *divisor1;
    lsGen           gen;
    lsHandle        handle;
    char            *data;
    ddivisor_cell_t *cell;

    divisor = DIVISOR(dd_cell);
    STATUS(divisor) = INDIVISOR;
    divisor1 = NIL(ddivisor_t);

    lsRemoveItem(divisor->sthandle, &data);
    divisor->weight_status = CHANGED;

    if ((DTYPE(divisor) == D222) || (DTYPE(divisor) == D223)) {
        divisor1 = ddivisor_alloc();
        divisor1->div_index = lsCreate();
        STATUS(divisor1) = INDIVISOR;
        lsForeachItem(divisor->div_index, gen, cell) {
            if (cell->phase) {
                lsRemoveItem(cell->handle, &data);
                lsNewEnd(divisor1->div_index, (lsGeneric) cell, &handle);
                cell->handle = handle;
                cell->phase  = 0;
                DIVISOR(cell) = divisor1;
            }
        }
        (void) sd_mod_new_divisor(divisor1, ddivisor_set);
    }
    (void) sd_mod_old_divisor(divisor, ddivisor_set);
}

/* Consider the possible effects after extracting single-cube divisor. */
int sd_check_affected(ddivisor_cell_t *dd_cell, ddset_t *ddivisor_set, int newcolumn) {
    ddivisor_t *divisor;
    sm_row     *base;
    int        con1, con2;

    divisor = DIVISOR(dd_cell);

    switch (STATUS(divisor)) {
        case NONCHECK:con1 = (sm_row_find(dd_cell->cube1, newcolumn) == 0) ? 0 : 1;
            con2           = (sm_row_find(dd_cell->cube2, newcolumn) == 0) ? 0 : 1;
            switch (con1 + con2) {
                case 2:
                    /* IN BASE */
                    (dd_cell->baselength)--;
                    STATUS(divisor) = INBASE;
                    (WEIGHT(divisor))--;
                    return 0;
                case 1:
                    /* IN BETWEEN or INDIVISOR */
                    base = sm_row_and(dd_cell->cube1, dd_cell->cube2);
                    if (base->length < dd_cell->baselength) {
                        STATUS(divisor) = INBETWEEN;
                        (void) sd_mod_INBETWEEN(dd_cell, ddivisor_set, newcolumn);
                    } else if (base->length == dd_cell->baselength) {
                        STATUS(divisor) = INDIVISOR;
                        (void) sd_mod_INDIVISOR(dd_cell, ddivisor_set);
                    } else {
                        fail("This is impossible\n");
                    }
                    (void) sm_row_free(base);
                    return 0;
                case 0: fail("This is not impossible \n");
            }
        case INBASE:(dd_cell->baselength)--;
            (WEIGHT(divisor))--;
            return 0;
        case INBETWEEN:
        case INDIVISOR:return 0;
        default:;
    }
}

/* Modification on double-cube divisor set after extracting an single-cube
 * divisor or complement divisor of D112.
 */
static void sd_modify(sm_matrix *B, rect_t *rect, ddset_t *ddivisor_set, int newcolumn) {
    lsGen           gen;
    lsHandle        handle;
    sm_cube_t       *sm_cube, *sm_cube1, *sm_cube2;
    ddivisor_cell_t *dd_cell;
    array_t         *del_set;
    char            *data;
    sm_element      *p;
    sm_row          *cube;
    int             i;

    del_set = array_alloc(ddivisor_cell_t *, 0);

    sm_foreach_col_element(rect->rows, p) {
        cube    = sm_get_row(B, p->row_num);
        sm_cube = (sm_cube_t *) cube->user_word;
        if (lsLength(sm_cube->div_set)) {
            lsForeachItem(sm_cube->div_set, gen, dd_cell) {
                lsRemoveItem(dd_cell->handle1, &data);
                lsRemoveItem(dd_cell->handle2, &data);
                (void) array_insert_last(ddivisor_cell_t *, del_set, dd_cell);
            }
        }
    }
    if (array_n(del_set) == 0) {
        (void) array_free(del_set);
        return;
    }

    for (i = 0; i < array_n(del_set); i++) {
        dd_cell  = array_fetch(ddivisor_cell_t *, del_set, i);
        sm_cube1 = (sm_cube_t *) dd_cell->cube1->user_word;
        sm_cube2 = (sm_cube_t *) dd_cell->cube2->user_word;
        lsNewEnd(sm_cube1->div_set, (lsGeneric) dd_cell, &handle);
        dd_cell->handle1 = handle;
        lsNewEnd(sm_cube2->div_set, (lsGeneric) dd_cell, &handle);
        dd_cell->handle2 = handle;
        (void) sd_check_affected(dd_cell, ddivisor_set, newcolumn);
    }

    (void) array_free(del_set);
}

/* Change sdivisor_t structure to rect_t structure */
static rect_t *sdiv_to_rect(sm_matrix *B, sdivisor_t *sdiv) {
    rect_t *rect;

    rect = ALLOC(rect_t, 1);
    rect->rows = sm_col_and(sm_get_col(B, sdiv->col1), sm_get_col(B, sdiv->col2));
    rect->cols = sm_row_alloc();
    sm_row_insert(rect->cols, sdiv->col1);
    sm_row_insert(rect->cols, sdiv->col2);
    FREE(sdiv);
    return rect;
}

/* main program */
int fast_extract(sm_matrix *B, ddset_t *ddivisor_set) {
    int        wdmax;
    ddivisor_t *divisor1;
    sdivisor_t *divisor2;
    sdset_t    *sdset;
    rect_t     *rect;

    int     index, loop, newcolumn;
    array_t *newcubes;

    index = 0;
    loop  = 1;
    sdset = sdset_alloc();

    do {

        /* Choose the best double-cube divisor. */
        if (lsLength(ddivisor_set->DDset)) {
            divisor1 = choose_best_divisor(B, ddivisor_set->DDset, &wdmax);
        } else {
            wdmax = -1;
        }

        /* Choose the best single-cube divisor. */
        divisor2 = extract_sdivisor(B, sdset, wdmax);

        /* Choose the best divisor. */
        if (divisor2 == NIL(sdivisor_t)) {
            if ((divisor1 == NIL(ddivisor_t)) || (wdmax < 0)) {
                loop = 0;
            } else {
                newcubes = dd_reexpress(B, divisor1, &index);
                (void) dd_modify(B, divisor1, index, newcubes, ddivisor_set);
            }
        } else {
            if ((COIN(divisor2) - 2) < 0) {
                loop = 0;
            } else {
                rect = sdiv_to_rect(B, divisor2);
                (void) sd_reexpress(B, rect, 0, index, &newcolumn);
                (void) sd_update_matrix(B, rect);
                (void) sd_modify(B, rect, ddivisor_set, newcolumn);
                (void) rect_free(rect);
            }
        }
    } while (loop);

    (void) sdset_free(sdset);
}
