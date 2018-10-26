
#include "phase_int.h"
#include "sis.h"

#define THRESH 0.00001

#define inv_save_output(r)                                                     \
  ((r)->pos_used == 0 ? -1 : ((r)->neg_used == 0 ? 1 : 0))
#define inv_save_input(r, e)                                                   \
  ((e)->phase == 0 && (r)->pos_used == 0                                       \
       ? -1                                                                    \
       : ((e)->phase == 1 && (r)->pos_used == 1 ? 1 : 0))

static void inv_save_setup();

static int inv_save_comp();

static row_data_t *row_data_dup();

static element_data_t *element_data_dup();

static void check_phase();

static void phase_do_invert();

static bool fanout_to_po();

static void phase_dec();

static void phase_inc();

static void phase_count_comp();

bool phase_trace = FALSE;
bool phase_check = FALSE;

static void inv_save_setup(net_phase_t *net_phase) {
    node_phase_t *node_phase;
    row_data_t   *rd;
    int          i;

    for (i = 0; i < array_n(net_phase->rows); i++) {
        node_phase = array_fetch(node_phase_t *, net_phase->rows, i);
        rd         = sm_get(row_data_t *, node_phase->row);
        rd->inv_save = inv_save_comp(net_phase, node_phase->row);
    }
}

static int inv_save_comp(net_phase_t *net_phase, sm_row *row) {
    register sm_col         *col;
    register sm_element     *element;
    register row_data_t     *rd, *rd1;
    register element_data_t *ed;
    register int            count;

    rd    = sm_get(row_data_t *, sm_get_row(net_phase->matrix, row->row_num));
    count = inv_save_output(rd);

    col = sm_get_col(net_phase->matrix, row->row_num);
    if (col != NIL(sm_col)) {
        sm_foreach_col_element(col, element) {
            ed  = sm_get(element_data_t *, element);
            row = sm_get_row(net_phase->matrix, element->row_num);
            rd1 = sm_get(row_data_t *, row);
            count += inv_save_input(rd1, ed);
        }
    }

    return count;
}

void phase_invert(net_phase_t *net_phase, node_phase_t *node_phase) {
    register sm_row     *row, *fanin;
    register sm_col     *col;
    register sm_element *element;
    register row_data_t *rd;
    double              cost_save;

    row       = node_phase->row;
    col       = sm_get_col(net_phase->matrix, row->row_num);
    rd        = sm_get(row_data_t *, row);
    cost_save = net_phase->cost;

    if (!rd->invertible) {
        fail("phase_invert: try to invert an non-invertible node");
    }

    net_phase->cost -= phase_value(node_phase);

    /* decrease the inverter savings of each fanout resulting from this node */
    phase_dec(net_phase, row);

    /* increase the inverter savings of each fanout resulting from this node */
    if (col != NIL(sm_col)) { /* warning: node may be a constant (no fanins) */
        sm_foreach_col_element(col, element) {
            fanin = sm_get_row(net_phase->matrix, element->row_num);
            phase_dec(net_phase, fanin);
        }
    }

    /* toggle the node and modify the row and col entries */
    phase_do_invert(net_phase, row);

    /* increase the inverter savings of each fanout resulting from this node */
    phase_inc(net_phase, row);

    /* increase the inverter savings of each fanout resulting from this node */
    if (col != NIL(sm_col)) { /* warning: node may be a constant (no fanins) */
        sm_foreach_col_element(col, element) {
            fanin = sm_get_row(net_phase->matrix, element->row_num);
            phase_inc(net_phase, fanin);
        }
    }

    if (phase_trace) {
        (void) fprintf(misout, "inverting %-10s: ", node_name(rd->node));
        (void) fprintf(misout, "cost: %4.0f", cost_save);
        (void) fprintf(misout, " -> %4.0f\n", net_phase->cost);
    }

    if (phase_check) {
        check_phase(net_phase);
    }
}

static void phase_do_invert(net_phase_t *net_phase, sm_row *row) {
    register sm_col         *col;
    register sm_row         *fanin;
    register sm_element     *element;
    register element_data_t *ed;
    register row_data_t     *rd;
    int                     temp;

    rd = sm_get(row_data_t *, row);
    rd->inverted = 1 - rd->inverted; /* toggle the phase */
    temp = rd->neg_used;
    rd->neg_used = rd->pos_used;
    rd->pos_used = temp;

    sm_foreach_row_element(row, element) {
        ed = sm_get(element_data_t *, element);
        if (ed->phase != 2) {
            ed->phase = 1 - ed->phase;
        }
    }

    col = sm_get_col(net_phase->matrix, row->row_num);
    if (col != NIL(sm_col)) { /* warning: node may be a constant (no fanins) */
        sm_foreach_col_element(col, element) {
            fanin = sm_get_row(net_phase->matrix, element->row_num);
            rd    = sm_get(row_data_t *, fanin);
            ed    = sm_get(element_data_t *, element);
            switch (ed->phase) {
                case 0:ed->phase = 1;
                    rd->pos_used++;
                    rd->neg_used--;
                    break;
                case 1:ed->phase = 0;
                    rd->pos_used--;
                    rd->neg_used++;
                    break;
                case 2:break;
                default: fail("phase_do_invert: wrong phase at a element");
            }
        }
    }
}

node_phase_t *phase_get_best(net_phase_t *net_phase) {
    register node_phase_t *node_phase, *best_phase;
    register int          i;
    register row_data_t   *rd;
    double                best, value;

    best_phase = NIL(node_phase_t);
    best       = -INFINITY;
    for (i     = 0; i < array_n(net_phase->rows); i++) {
        node_phase = array_fetch(node_phase_t *, net_phase->rows, i);
        rd         = sm_get(row_data_t *, node_phase->row);
        if (rd->invertible && !rd->marked) {
            value = phase_value(node_phase);
            if (value > best) {
                best_phase = node_phase;
                best       = value;
            }
        }
    }

    return best_phase;
}

double network_cost(net_phase_t *net_phase) { return net_phase->cost; }

void phase_unmark_all(net_phase_t *net_phase) {
    register node_phase_t *node_phase;
    register row_data_t   *rd;
    register int          i;

    for (i = 0; i < array_n(net_phase->rows); i++) {
        node_phase = array_fetch(node_phase_t *, net_phase->rows, i);
        rd         = sm_get(row_data_t *, node_phase->row);
        rd->marked = FALSE;
    }
}

void phase_mark(node_phase_t *node_phase) {
    row_data_t *rd;

    rd = sm_get(row_data_t *, node_phase->row);
    rd->marked = TRUE;
}

net_phase_t *phase_dup(net_phase_t *net_phase) {
    register net_phase_t *new;
    register sm_row      *row, *new_row;
    register sm_element  *element, *new_element;
    node_phase_t         *node_phase, *temp;
    row_data_t           *rd;
    element_data_t       *ed;
    int                  i;

    new = ALLOC(net_phase_t, 1);

    new->matrix = sm_dup(net_phase->matrix);
    sm_foreach_row(new->matrix, new_row) {
        row = sm_get_row(net_phase->matrix, new_row->row_num);
        rd  = sm_get(row_data_t *, row);
        sm_put(new_row, row_data_dup(rd));
        sm_foreach_row_element(new_row, new_element) {
            element = sm_row_find(row, new_element->col_num);
            ed      = sm_get(element_data_t *, element);
            sm_put(new_element, element_data_dup(ed));
        }
    }

    new->rows = array_alloc(node_phase_t *, array_n(net_phase->rows));
    for (i = 0; i < array_n(net_phase->rows); i++) {
        temp       = array_fetch(node_phase_t *, net_phase->rows, i);
        node_phase = ALLOC(node_phase_t, 1);
        node_phase->row = sm_get_row(new->matrix, temp->row->row_num);
        array_insert_last(node_phase_t *, new->rows, node_phase);
    }

    new->cost = net_phase->cost;
    return new;
}

void phase_replace(net_phase_t *old, net_phase_t *new) {
    sm_matrix *matrix;
    array_t   *rows;

    matrix = old->matrix;
    rows   = old->rows;

    old->matrix = new->matrix;
    old->rows   = new->rows;
    old->cost   = new->cost;

    new->matrix = matrix;
    new->rows   = rows;
    phase_free(new);
}

static row_data_t *row_data_dup(row_data_t *row_data) {
    row_data_t *new;

    new = ALLOC(row_data_t, 1);
    new->pos_used   = row_data->pos_used;
    new->neg_used   = row_data->neg_used;
    new->inv_save   = row_data->inv_save;
    new->marked     = row_data->marked;
    new->invertible = row_data->invertible;
    new->inverted   = row_data->inverted;
    new->po         = row_data->po;
    new->node       = row_data->node;

    return new;
}

static element_data_t *element_data_dup(element_data_t *element_data) {
    element_data_t *new;

    new = ALLOC(element_data_t, 1);
    new->phase = element_data->phase;

    return new;
}

void phase_free(net_phase_t *net_phase) {
    int            i;
    node_phase_t   *node_phase;
    row_data_t     *rd;
    element_data_t *ed;
    sm_row         *row;
    sm_element     *element;

    for (i = 0; i < array_n(net_phase->rows); i++) {
        node_phase = array_fetch(node_phase_t *, net_phase->rows, i);
        FREE(node_phase);
    }
    array_free(net_phase->rows);

    sm_foreach_row(net_phase->matrix, row) {
        sm_foreach_row_element(row, element) {
            ed = sm_get(element_data_t *, element);
            FREE(ed);
        }
        rd = sm_get(row_data_t *, row);
        FREE(rd);
    }

    sm_free(net_phase->matrix);
    FREE(net_phase);
}

void phase_print(net_phase_t *net_phase) {
    sm_row         *row, *row1;
    row_data_t     *rd;
    sm_element     *element;
    element_data_t *ed;

    (void) fprintf(misout, "\n");
    (void) fprintf(misout, "r# name pos,neg,save,");
    (void) fprintf(misout, "inverted,invertible,marked elements\n");
    sm_foreach_row(net_phase->matrix, row) {
        (void) fprintf(misout, "%2d", row->row_num);
        rd = sm_get(row_data_t *, row);
        (void) fprintf(misout, " %5s ", node_name(rd->node));
        (void) fprintf(misout, " %1d", rd->pos_used);
        (void) fprintf(misout, ",%1d", rd->neg_used);
        (void) fprintf(misout, ",%2d", rd->inv_save);
        if (rd->inverted) {
            (void) fprintf(misout, ",i");
        } else {
            (void) fprintf(misout, ",n");
        }
        if (rd->invertible) {
            (void) fprintf(misout, ",i");
        } else {
            (void) fprintf(misout, ",n");
        }
        if (rd->marked) {
            (void) fprintf(misout, ",m");
        } else {
            (void) fprintf(misout, ",u  ");
        }
        sm_foreach_row_element(row, element) {
            ed   = sm_get(element_data_t *, element);
            row1 = sm_get_row(net_phase->matrix, element->col_num);
            if (row1 != NIL(sm_row)) {
                rd = sm_get(row_data_t *, row1);
                (void) fprintf(misout, " %1d(%s)", ed->phase, node_name(rd->node));
            }
        }
        (void) fprintf(misout, "\n");
    }
    (void) fprintf(misout, "cost = %6.0f\n", net_phase->cost);
}

void phase_check_set() { phase_check = TRUE; }

void phase_check_unset() { phase_check = FALSE; }

void phase_trace_set() { phase_trace = TRUE; }

void phase_trace_unset() { phase_trace = FALSE; }

static void check_phase(net_phase_t *net_phase) {
    sm_row     *row;
    row_data_t *rd;
    int        save;
    bool       pass;

    pass = TRUE;

    if (fabs(net_phase->cost - cost_comp(net_phase)) > (double) THRESH) {
        (void) fprintf(miserr, "Error: inverter savings are not correct\n");
        (void) fprintf(miserr, "Computed cost: %f\n", cost_comp(net_phase));
        (void) fprintf(miserr, "Recorded cost: %f\n", net_phase->cost);
        pass = FALSE;
    }

    sm_foreach_row(net_phase->matrix, row) {
        rd   = sm_get(row_data_t *, row);
        save = inv_save_comp(net_phase, row);
        if (rd->invertible && rd->node->type != INTERNAL) {
            (void) fprintf(miserr, "Error, %s ", node_name(rd->node));
            (void) fprintf(miserr, "is not INTERNAL, but invertible\n");
            pass = FALSE;
        }
        if (rd->inv_save != save) {
            (void) fprintf(miserr, "inv_save(%-10s) ", node_name(rd->node));
            (void) fprintf(miserr, "recorded: %d, ", rd->inv_save);
            (void) fprintf(miserr, "computed: %d\n", save);
            pass = FALSE;
        }
    }

    if (!pass) {
        (void) fprintf(miserr, "Did not pass phase consistency check\n");
        exit(-1);
    }
}

/*
 *  1. decrease the output inverter saving of row.
 *  2. decrease the input inverter saving of row for each fanout of row.
 */
static void phase_dec(net_phase_t *net_phase, sm_row *row) {
    sm_row         *fanout;
    sm_element     *element;
    row_data_t     *rd, *rd1;
    element_data_t *ed;

    /* step 1 */
    rd = sm_get(row_data_t *, row);
    rd->inv_save -= inv_save_output(rd);

    /* step 2 */
    sm_foreach_row_element(row, element) {
        fanout = sm_get_row(net_phase->matrix, element->col_num);
        if (fanout != NIL(sm_row)) {
            ed  = sm_get(element_data_t *, element);
            rd1 = sm_get(row_data_t *, fanout);
            rd1->inv_save -= inv_save_input(rd, ed);
        }
    }
}

/*
 *  1. increase the output inverter saving of row.
 *  2. increase the input inverter saving of row for each fanout of row.
 */
static void phase_inc(net_phase_t *net_phase, sm_row *row) {
    sm_row         *fanout;
    sm_element     *element;
    row_data_t     *rd, *rd1;
    element_data_t *ed;

    /* step 1 */
    rd = sm_get(row_data_t *, row);
    rd->inv_save += inv_save_output(rd);

    /* step 2 */
    sm_foreach_row_element(row, element) {
        fanout = sm_get_row(net_phase->matrix, element->col_num);
        if (fanout != NIL(sm_row)) {
            ed  = sm_get(element_data_t *, element);
            rd1 = sm_get(row_data_t *, fanout);
            rd1->inv_save += inv_save_input(rd, ed);
        }
    }
}

static bool fanout_to_po(node_t *f) {
    node_t          *np;
    lsGen           gen;
    node_function_t func;

    foreach_fanout(f, gen, np) {
        if (np->type == PRIMARY_OUTPUT) {
            (void) lsFinish(gen);
            return TRUE;
        }
        func = node_function(np);
        if (func == NODE_INV || func == NODE_BUF) {
            if (fanout_to_po(np)) {
                (void) lsFinish(gen);
                return TRUE;
            }
        }
    }
    return FALSE;
}

void phase_random_assign(net_phase_t *net_phase) {
    node_phase_t *node_phase;
    row_data_t   *rd;
    int          i;

    for (i = 0; i < array_n(net_phase->rows); i++) {
        node_phase = array_fetch(node_phase_t *, net_phase->rows, i);
        rd         = sm_get(row_data_t *, node_phase->row);
        if (rd->invertible) {
            if ((rand() % 16384) >= 8192) {
                phase_invert(net_phase, node_phase);
            }
        }
    }
}

net_phase_t *phase_setup(network_t *network) {
    node_t      *np;
    net_phase_t *net_phase;
    nodeindex_t *table;
    lsGen       gen;

    /* initialization */
    net_phase = ALLOC(net_phase_t, 1);
    net_phase->matrix = sm_alloc();
    net_phase->rows   = array_alloc(node_phase_t *, 0);
    table = nodeindex_alloc();
    phase_lib_setup(network);

    /* setup the matrix */
    foreach_node(network, gen, np) { phase_node_setup(np, table, net_phase); }

    inv_save_setup(net_phase);
    nodeindex_free(table);
    net_phase->cost = cost_comp(net_phase);

    if (phase_check) {
        check_phase(net_phase);
    }

    return net_phase;
}

void phase_node_setup(node_t *np, nodeindex_t *table, net_phase_t *net_phase) {
    node_function_t func;
    node_phase_t    *node_phase;
    row_data_t      *row_data;
    int             rowi;

    func = node_function(np);

    if (func == NODE_PO || func == NODE_INV || func == NODE_BUF) {
        return;
    }

    if (func == NODE_PI && node_num_fanout(np) == 0) {
        return;
    }

    /* setup the matrix entries for this row */
    rowi = nodeindex_insert(table, np);
    phase_fanout_setup(rowi, np, table, net_phase, 0);

    /* setup the row_data for this row */
    row_data   = ALLOC(row_data_t, 1);
    node_phase = ALLOC(node_phase_t, 1);
    node_phase->row = sm_get_row(net_phase->matrix, rowi);
    sm_put(node_phase->row, row_data);
    array_insert_last(node_phase_t *, net_phase->rows, node_phase);
    row_data->node     = np;
    row_data->inverted = FALSE;
    row_data->marked   = FALSE;
    row_data->po       = fanout_to_po(np);
    phase_invertible_set(row_data);
    phase_count_comp(np, table, net_phase);
}

void phase_fanout_setup(int rowi, node_t *np, nodeindex_t *table, net_phase_t *net_phase, int phase) {
    node_t          *fanout;
    lsGen           gen;
    node_function_t func;
    sm_element      *element;
    element_data_t  *element_data;
    int             coli;

    foreach_fanout(np, gen, fanout) {
        func = node_function(fanout);
        if (func == NODE_INV) {
            phase_fanout_setup(rowi, fanout, table, net_phase, 1 - phase);
        } else if (func == NODE_BUF) {
            phase_fanout_setup(rowi, fanout, table, net_phase, phase);
        } else {
            coli         = nodeindex_insert(table, fanout);
            element      = sm_insert(net_phase->matrix, rowi, coli);
            element_data = sm_get(element_data_t *, element);
            if (element_data == NIL(element_data_t)) {
                element_data = ALLOC(element_data_t, 1);
                sm_put(element, element_data);
                element_data->phase = -1;
            }
            if (phase != element_data->phase) {
                if (element_data->phase == -1) {
                    element_data->phase = phase;
                } else {
                    element_data->phase = 2;
                }
            }
        }
    }
}

static void phase_count_comp(node_t *np, nodeindex_t *table, net_phase_t *net_phase) {
    sm_row         *row;
    int            rowi;
    sm_element     *element;
    element_data_t *element_data;
    row_data_t     *row_data;

    rowi     = nodeindex_indexof(table, np);
    row      = sm_get_row(net_phase->matrix, rowi);
    row_data = sm_get(row_data_t *, row);
    row_data->pos_used = row_data->neg_used = 0;
    sm_foreach_row_element(row, element) {
        element_data = sm_get(element_data_t *, element);
        switch (element_data->phase) {
            case 0:row_data->neg_used++;
                break;
            case 1:row_data->pos_used++;
                break;
            case 2:row_data->pos_used++;
                row_data->neg_used++;
                break;
            default: fail("phase_count_comp: wrong phase at the element");
        }
    }
}
