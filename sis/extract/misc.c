
#include "extract_int.h"
#include "sis.h"

/*
 *  return a vector of the unique fanouts involved in a rectangle
 *
 *  That is, find all unique references to a sis_index inside the value
 *  cell for every element in the rectangle
 */

array_t *find_rectangle_fanout(sm_matrix *A, rect_t *rect) {
    register sm_element *p1, *p2, *p;
    st_table            *fanout_table;
    st_generator        *gen;
    array_t             *fanouts;
    value_cell_t        *value_cell;
    char                *key;

    /*
     *  Find all of the functions it fans out to
     */
    fanout_table = st_init_table(st_numcmp, st_numhash);
    sm_foreach_col_element(rect->rows, p1) {
        sm_foreach_row_element(rect->cols, p2) {
            p          = sm_find(A, p1->row_num, p2->col_num);
            value_cell = (value_cell_t *) p->user_word;
            key        = (char *) value_cell->sis_index;
            (void) st_insert(fanout_table, key, NIL(char));
        }
    }

    /*
     *  now pull the unique functions out and put them in an array
     */
    fanouts = array_alloc(char *, 0);
    st_foreach_item(fanout_table, gen, &key, NIL(
            char *)) {
        array_insert_last(char *, fanouts, key);
    }
    st_free_table(fanout_table);

    return fanouts;
}

/*
 *  return a vector of the unique fanouts involved in a rectangle, and also
 *  return a vector of the cubes which are involved in this rectangle
 *
 *  That is, find all unique references to a sis_index inside the value
 *  cell for every element in the rectangle
 */

array_t *find_rectangle_fanout_cubes(sm_matrix *A, rect_t *rect, array_t **cubes) {
    register sm_element *p1, *p2, *p;
    sm_row              *row;
    st_table            *fanout_table;
    st_generator        *gen;
    array_t             *fanouts;
    value_cell_t        *value_cell;
    char                *key, *value;

    /*
     *  Find all of the functions (and cubes) it fans out to
     */
    fanout_table = st_init_table(st_numcmp, st_numhash);
    sm_foreach_col_element(rect->rows, p1) {
        sm_foreach_row_element(rect->cols, p2) {
            p          = sm_find(A, p1->row_num, p2->col_num);
            value_cell = (value_cell_t *) p->user_word;
            key        = (char *) value_cell->sis_index;
            if (st_lookup(fanout_table, key, &value)) {
                row = (sm_row *) value;
            } else {
                row   = sm_row_alloc();
                value = (char *) row;
                (void) st_insert(fanout_table, key, value);
            }

            (void) sm_row_insert(row, value_cell->cube_number);
        }
    }

    /*
     *  put the fanout functions into an array along with the covered cubes
     */
    fanouts = array_alloc(int, 0);
    *cubes = array_alloc(sm_row *, 0);
    st_foreach_item(fanout_table, gen, &key, &value) {
        array_insert_last(char *, fanouts, key);
        array_insert_last(char *, *cubes, value);
    }
    st_free_table(fanout_table);

    return fanouts;
}
