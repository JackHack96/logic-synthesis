
#include "simp_int.h"
#include "sis.h"

int simp_fanin_level;
int simp_fanin_fanout_level;
bool simp_debug = FALSE;
bool simp_trace = FALSE;

static st_table *simp_table();

static array_t *simp_nodefanout_vec();

static void nodefanout_print();

static void fanout_table_print();

static array_t *get_sub_list();

typedef struct nodefanout_struct {
    node_t *node;
    node_t *fanout;
}   nodefanout_t;

node_t *simp_DI(node_t *f) {
    node_t *DC, *flit;

    flit = node_literal(f, 1);
    DC   = node_xor(f, flit);
    node_free(flit);
    return DC;
}

node_t *unused_simp_fanin_dc(node_t *f) {
    st_table     *table;
    node_t       *x, *DC, *x_xor_xlit, *DC_temp;
    st_generator *gen;
    char         *dummy;

    /* build a table of "useful" nodes for generating the don't-care set */
    table = simp_table(f);

    /* for each node in the table, generate the don't care set */
    DC = node_constant(0);
    st_foreach_item(table, gen, &dummy, NIL(
            char *)) {
        x = (node_t *) dummy;
        if (x != f) {
            x_xor_xlit = simp_DI(x);
            DC_temp    = node_or(DC, x_xor_xlit);
            node_free(DC);
            node_free(x_xor_xlit);
            DC = DC_temp;
        }
    }

    st_free_table(table);
    return DC;
}

static st_table *simp_table(node_t *f) {
    array_t      *nodefanout_vec, *temp;
    node_t       *fanin;
    nodefanout_t curr, next;
    st_table     *table;
    int          i;

    /* put fanins of f and their fanins in the node-fanout array */
    nodefanout_vec = simp_nodefanout_vec(f);
    foreach_fanin(f, i, fanin) {
        temp = simp_nodefanout_vec(fanin);
        array_append(nodefanout_vec, temp);
        array_free(temp);
    }

    /* sort the node-fanout array */
    array_sort(nodefanout_vec, node_compare_id);
    if (simp_debug) {
        nodefanout_print(nodefanout_vec);
    }

    /* for each duplicated entry, put the fanout into a hash table */
    table  = st_init_table(st_ptrcmp, st_ptrhash);
    for (i = 0; i < array_n(nodefanout_vec) - 1; i++) {
        curr = array_fetch(nodefanout_t, nodefanout_vec, i);
        next = array_fetch(nodefanout_t, nodefanout_vec, i + 1);
        if (curr.node == next.node) {
            (void) st_insert(table, (char *) curr.fanout, NIL(char));
            (void) st_insert(table, (char *) next.fanout, NIL(char));
        }
    }
    array_free(nodefanout_vec);

    if (simp_debug) {
        fanout_table_print(table);
    }

    return table;
}

static array_t *simp_nodefanout_vec(node_t *f) {
    nodefanout_t nodefanout;
    array_t      *nodefanout_vec;
    node_t       *fanin;
    int          i;

    nodefanout_vec = array_alloc(nodefanout_t, 0);

    foreach_fanin(f, i, fanin) {
        nodefanout.node   = fanin;
        nodefanout.fanout = f;
        array_insert_last(nodefanout_t, nodefanout_vec, nodefanout);
    }

    return nodefanout_vec;
}

static void nodefanout_print(array_t *vec) {
    int          i;
    nodefanout_t nf;

    (void) fprintf(sisout, "nodefanout_vec:\n");
    for (i = 0; i < array_n(vec); i++) {
        nf = array_fetch(nodefanout_t, vec, i);
        (void) fprintf(sisout, " %s->", node_name(nf.node));
        (void) fprintf(sisout, "%s", node_name(nf.fanout));
    }
    (void) fprintf(sisout, "\n");
}

static void fanout_table_print(st_table *table) {
    char         *dummy;
    st_generator *gen;
    node_t       *x;

    (void) fprintf(sisout, "fanout_table:\n");
    st_foreach_item(table, gen, &dummy, NIL(
            char *)) {
        x = (node_t *) dummy;
        (void) fprintf(sisout, " %s", node_name(x));
    }
    (void) fprintf(sisout, "\n\n");
}

node_t *simp_fanout_dc(node_t *f) {
    lsGen  gen;
    node_t *DC, *fanout, *f0, *f1, *p, *q, *t1, *t2;

    DC = node_constant(1);
    f1 = node_literal(f, 1);
    f0 = node_literal(f, 0);
    foreach_fanout(f, gen, fanout) {

        if (node_function(fanout) == NODE_PO) {
            node_free(DC);
            DC = node_constant(0);
            (void) lsFinish(gen);
            break;
        }

        p  = node_cofactor(fanout, f1);
        q  = node_cofactor(fanout, f0);
        t1 = node_xnor(p, q);
        t2 = node_and(DC, t1);
        node_free(p);
        node_free(q);
        node_free(t1);
        node_free(DC);
        DC = t2;

        if (node_function(DC) == NODE_0) {
            (void) lsFinish(gen);
            break;
        }
    }

    node_free(f1);
    node_free(f0);

    return DC;
}

node_t *simp_inout_dc(node_t *f, int il, int iol) {
    node_t *DC, *faninDC, *fanoutDC;

    faninDC  = simp_tfanin_dc(f, il, iol);
    fanoutDC = simp_fanout_dc(f);
    DC       = node_or(faninDC, fanoutDC);
    node_free(faninDC);
    node_free(fanoutDC);
    return DC;
}

node_t *simp_tfanin_dc(node_t *f, int il, int ol) {
    st_table     *f_fanout_table, *dc_table;
    st_generator *gen;
    array_t      *f_fanout, *n_fanout, *f_fanin;
    node_t       *DC, *DC1, *DC_temp, *fanout, *fanin, *np;
    int          i, j;
    char         *dummy;

    /* build the transitive-fanout-cone table */
    f_fanout_table = st_init_table(st_ptrcmp, st_ptrhash);
    f_fanout       = network_tfo(f, INFINITY);
    for (i         = 0; i < array_n(f_fanout); i++) {
        fanout = array_fetch(node_t *, f_fanout, i);
        (void) st_insert(f_fanout_table, (char *) fanout, NIL(char));
    }
    (void) st_insert(f_fanout_table, (char *) f, NIL(char));
    array_free(f_fanout);

    /* build transitive-fanin_cone table */
    dc_table = st_init_table(st_ptrcmp, st_ptrhash);
    f_fanin  = network_tfi(f, il);
    for (i   = 0; i < array_n(f_fanin); i++) {
        fanin = array_fetch(node_t *, f_fanin, i);
        (void) st_insert(dc_table, (char *) fanin, NIL(char));
        n_fanout = network_tfo(fanin, ol);
        for (j   = 0; j < array_n(n_fanout); j++) {
            np = array_fetch(node_t *, n_fanout, j);
            if (!st_lookup(f_fanout_table, (char *) np, &dummy)) {
                (void) st_insert(dc_table, (char *) np, NIL(char));
            }
        }
        array_free(n_fanout);
    }
    array_free(f_fanin);
    st_free_table(f_fanout_table);

    /* build the don't-care set for each node in the dc_table */
    DC = node_constant(0);
    st_foreach_item(dc_table, gen, &dummy, NIL(
            char *)) {
        np = (node_t *) dummy;
        if (np->type == INTERNAL) {
            DC1     = simp_DI(np);
            DC_temp = node_or(DC, DC1);
            node_free(DC1);
            node_free(DC);
            DC = DC_temp;
        }
    }
    st_free_table(dc_table);

    return DC;
}

node_t *simp_all_dc(node_t *f) {
    node_t *fanin_dc, *fanout_dc, *DC;

    fanin_dc  = simp_tfanin_dc(f, INFINITY, INFINITY);
    fanout_dc = simp_fanout_dc(f);
    DC        = node_or(fanin_dc, fanout_dc);
    node_free(fanin_dc);
    node_free(fanout_dc);

    return DC;
}

node_t *simp_sub_fanin_dc(node_t *f) {
    node_t  *np, *DC, *DC1, *DC_temp;
    array_t *dc_list;
    int     i, j;
    int num_cube_cmp();

    dc_list = get_sub_list(f);

    /* build the don't-care set for each node in the dc_table */
    DC = node_constant(0);
    array_sort(dc_list, num_cube_cmp);
    for (i = 0, j = 0; i < array_n(dc_list); i++) {
        np = array_fetch(node_t *, dc_list, i);
        if (node_num_literal(np) == 1)
            continue;
        if (node_num_cube(np) > 100)
            continue;
        if (node_num_cube(np) > 2 * node_num_cube(f))
            continue;
        if ((node_num_fanin(f) > 15) && ((j * node_num_fanin(f)) > 6000)) {
            break;
        }
        if ((node_num_fanin(f) >= 50) && ((j * node_num_fanin(f)) > 3000)) {
            break;
        }
        if (np->type == INTERNAL) {
            DC1     = simp_DI(np);
            DC_temp = node_or(DC, DC1);
            node_free(DC1);
            node_free(DC);
            DC = DC_temp;
            j  = node_num_literal(DC);
        }
    }
    array_free(dc_list);
    return DC;
}

static array_t *get_sub_list(node_t *f) {
    st_table *f_table, *sup_table;
    array_t  *f_fanin, *f_fanout, *nlist, *sup_list;
    node_t   *fanin, *fanout, *np;
    bool     same_sup;
    int      i, j;
    char     *dummy;

    /* get a list of the immediate fanin */
    f_table = st_init_table(st_ptrcmp, st_ptrhash);
    f_fanin = network_tfi(f, 1);
    nlist   = array_alloc(node_t *, 0);
    for (i  = 0; i < array_n(f_fanin); i++) {
        fanin = array_fetch(node_t *, f_fanin, i);
        if (!st_lookup(f_table, (char *) fanin, &dummy)) {
            (void) array_insert_last(node_t *, nlist, fanin);
            (void) st_insert(f_table, (char *) fanin, NIL(char));
        }
        f_fanout = network_tfo(fanin, INFINITY);
        for (j   = 0; j < array_n(f_fanout); j++) {
            fanout = array_fetch(node_t *, f_fanout, j);
            if (fanout->type != INTERNAL)
                continue;
            if (!st_lookup(f_table, (char *) fanout, &dummy)) {
                (void) array_insert_last(node_t *, nlist, fanout);
                (void) st_insert(f_table, (char *) fanout, NIL(char));
            }
        }
        array_free(f_fanout);
    }
    array_free(f_fanin);
    st_free_table(f_table);

    /* retain all nodes with the same or subset support of f
     * assumes minimum base for the network nodes
     */
    sup_table = st_init_table(st_ptrcmp, st_ptrhash);
    sup_list  = array_alloc(node_t *, 0);
    foreach_fanin(f, i, fanin)
        (void)
                st_insert(sup_table, (char *) fanin, NIL(char));
    for (i = 0; i < array_n(nlist); i++) {
        np = array_fetch(node_t *, nlist, i);
        if (np == f)
            continue;
        same_sup = TRUE;
        foreach_fanin(np, j, fanin) {
            if (!st_lookup(sup_table, (char *) fanin, &dummy)) {
                same_sup = FALSE;
            }
        }
        if (same_sup) {
            (void) st_insert(sup_table, (char *) np, NIL(char));
            (void) array_insert_last(node_t *, sup_list, np);
        }
    }
    st_free_table(sup_table);
    array_free(nlist);
    return (sup_list);
}

node_t *simp_level_dc(node_t *f, st_table *level_table) {
    node_t  *np, *DC, *DC1, *DC_temp;
    array_t *dc_list;
    int     i, j;
    int     dlevel, flevel;
    char    *dummy;

    dc_list    = get_sub_list(f);
    if (st_lookup(level_table, (char *) f, &dummy))
        flevel = (int) dummy;

    /* build the don't-care set for each node in the dc_table */
    DC     = node_constant(0);
    for (i = 0, j = 0; i < array_n(dc_list); i++) {
        np = array_fetch(node_t *, dc_list, i);
        if (node_num_literal(np) == 1)
            continue;
        if (node_num_cube(np) > 100)
            continue;
        if (node_num_cube(np) > 2 * node_num_cube(f))
            continue;
        if ((node_num_fanin(f) > 15) && ((j * node_num_fanin(f)) > 6000)) {
            break;
        }
        if (np->type == INTERNAL) {
            if (st_lookup(level_table, (char *) np, &dummy))
                dlevel = (int) dummy;
            if (dlevel < flevel) {
                DC1     = simp_DI(np);
                DC_temp = node_or(DC, DC1);
                node_free(DC1);
                node_free(DC);
                DC = DC_temp;
                j  = node_num_literal(DC);
            }
        }
    }
    array_free(dc_list);
    return DC;
}
