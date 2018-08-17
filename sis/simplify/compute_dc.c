
#include "sis.h"
#include "simp_int.h"

static double_node_t *compute_mspf();

static node_t *compute_cspf();

static st_table *cspf_inout_table();

static node_t *cube_to_node();

void simplify_without_odc();

void simplify_with_odc();

void cspf_alloc();

void cspf_free();

bdd_t *cspf_bdd_dc();

static void update_cspf_of_fanins();

void odc_alloc();

void odc_free();

int level_node_cmp1();

int level_node_cmp2();

int level_node_cmp3();

void find_odc_level();

int odc_value();

/*
 * simplifies each node using the local don't cares and a subset of satisfiability don't cares
 * generated for the nodes that have the same support as node being simplified. No observability
 * don't care is computed, therefore, the local don't care is incomplete.
 */
void simplify_without_odc(network, accept, method, dctype, filter, level_table, mg, leaves)
        network_t *network;
        sim_accept_t accept;
        sim_method_t method;
        sim_dctype_t dctype;
        sim_filter_t filter;
        st_table *level_table;
        bdd_manager *mg;
        st_table *leaves; /*cutset of nodes in the network, used for BDD construction*/
{
    array_t *network_node_list;/*an array of the nodes in the network*/
    int     array_size;
    int     i, j;
    array_t *fanout_list;
    node_t  *node, *fanout;
    bdd_t   *bdd, *newbdd;

    network_node_list = network_dfs_from_input(network);
    array_size        = array_n(network_node_list);

    for (i = 0; i < array_size; i++) {
        node = array_fetch(node_t * , network_node_list, i);
        if (node_function(node) == NODE_PI)
            continue;
        if (node_function(node) == NODE_PO)
            continue;
        if (node_num_fanin(node) == 0) {
            CSPF(node)->bdd = bdd_zero(mg);
            continue;
        }
        fanout_list = network_tfo(node, 1);
        newbdd = bdd_one(mg);
        for (j = 0; j < array_n(fanout_list); j++) {
            fanout = array_fetch(node_t * , fanout_list, j);
            bdd    = bdd_and(CSPF(fanout)->bdd, newbdd, 1, 1);
            bdd_free(newbdd);
            newbdd = bdd;
        }
        array_free(fanout_list);
        CSPF(node)->bdd = newbdd;
        if (node->type == INTERNAL) {
            simplify_cspf_node(node, method, dctype, accept, filter, level_table, mg, leaves);
        }
    }
    array_free(network_node_list);
}
/*
 * Computes compatible observability don't cares for each node. This is used later
 * to find the local don't cares (don't cares in terms of fanins of each node).
 * Also the satisfiability don't cares for a subset of nodes that can be
 * substituted in the current node is generated. The don't cares are
 * used to simplify the node using a two-level minimizer.
 */

void simplify_with_odc(network, accept, method, dctype, filter, level_table, mg, leaves)
        network_t *network;
        sim_accept_t accept;
        sim_method_t method;
        sim_dctype_t dctype;
        sim_filter_t filter;
        st_table *level_table;
        bdd_manager *mg;
        st_table *leaves; /*cutset of nodes in the network, used for BDD construction*/
{
    array_t       *network_node_list;/*an array of the nodes in the network*/
    array_t       *fanin_list, *fanout_list;
    array_t       *node_list, *dc_list;
    int           numin, array_size;
    int           i, j, k, m;
    int           index;
    node_t        *p, *q, *node, *fanout;
    node_t        *pfanin, *nd;
    node_t        *dcnode;
    node_t        *fanin;
    node_t        *pi;
    double_node_t *mspf_node;
    bdd_t         *bdd, *dcbdd, *FDC;
    node_t        *ANDpi; /* ANDing of all the primary inputs*/
    st_table      *tfotfi_table, *fis_table;
    st_table      *fofis_table;
    lsGen         gen;

    network_node_list = network_dfs_from_input(network);
    array_size        = array_n(network_node_list);

/*create a new node by ANDing all the PIs. This is used for filtering*/
    ANDpi  = node_constant(1);
    foreach_primary_input(network, gen, pi)
    {
        p = node_literal(pi, 1);
        q = node_and(p, ANDpi);
        node_free(p);
        node_free(ANDpi);
        ANDpi = q;
    }
/*compute CSPF and simplify each node*/
    for (i = 0; i < array_size; i++) {
        node = array_fetch(node_t * , network_node_list, i);
        if (node_function(node) == NODE_PI)
            continue;
        if (node_function(node) == NODE_PO)
            continue;
        if (node_num_fanin(node) == 0) {
            CSPF(node)->bdd = bdd_zero(mg);
            continue;
        }
        fanout_list = network_tfo(node, 1);

        tfotfi_table = cspf_inout_table(node, INFINITY, 1);
        node_list    = network_tfi(node, 1);
        fis_table = st_init_table(st_ptrcmp, st_ptrhash);
        for (m = 0; m < array_n(node_list); m++) {
            nd = array_fetch(node_t * , node_list, m);
            (void) st_insert(fis_table, (char *) nd, NIL(
            char));
        }

        FDC = NIL(bdd_t);

/* compute CSPF for each of the fanout edges; intersect them to
 * find the CSPF for the node.
 */
        for (j = 0; j < array_n(fanout_list); j++) {
            fanout = array_fetch(node_t * , fanout_list, j);
            if (node_function(fanout) == NODE_PO) {
                dcnode = node_constant(0);
            } else {
                index = node_get_fanin_index(fanout, node);
                dc_list = CSPF(fanout)->list;
                mspf_node = array_fetch(double_node_t * , dc_list, index);
                dcnode = node_dup(mspf_node->pos);
                dcnode = simp_obsdc_filter(dcnode, tfotfi_table, 0, ANDpi);
            }

            if ((node_function(fanout) != NODE_PO) || (node_num_fanout(fanout) > 0)) {
                fanin_list = network_tfi(fanout, 1);

/* make the mspf compatible with the cspf computed for previous edges. */
                fofis_table = st_init_table(st_ptrcmp, st_ptrhash);
                for (m = 0; m < array_n(fanin_list); m++) {
                    nd = array_fetch(node_t * , fanin_list, m);
                    (void) st_insert(fofis_table, (char *) nd, NIL(
                    char));
                }
                for (k = 0; k < array_n(fanin_list); k++) {
                    pfanin = array_fetch(node_t * , fanin_list, k);
                    index = node_get_fanin_index(fanout, pfanin);
                    dc_list = CSPF(fanout)->list;
                    mspf_node = array_fetch(double_node_t * , dc_list, index);
                    if (node_function(pfanin) != NODE_PI)
                        dcnode = compute_cspf(dcnode, pfanin, mspf_node->neg, fis_table,
                                              ANDpi, fofis_table);
                }
                st_free_table(fofis_table);
                array_free(fanin_list);
            }

/* OR the computed CSPF with the CSPF of the fanout node.*/
            dcbdd = bdd_dup(ntbdd_node_to_bdd(dcnode, mg, leaves));
            if (CSPF(fanout)->bdd != NIL(bdd_t)) {
                bdd = CSPF(fanout)->bdd;
                bdd = bdd_or(bdd, dcbdd, 1, 1);
                bdd_free(dcbdd);
                dcbdd = bdd;
            }
            ntbdd_free_at_node(dcnode);
            node_free(dcnode);

/* Filter the CSPF and collapse all the nodes that are not transitive
 * fanins of the node being simplified.
 */

/* Do the intersection to find the CSPF of the node */
            if (FDC == NIL(bdd_t)) {
                FDC = dcbdd;
            } else {
                bdd = bdd_and(FDC, dcbdd, 1, 1);
                bdd_free(dcbdd);
                bdd_free(FDC);
                FDC = bdd;
            }
        }

/* store the CSPF */
        if (FDC != NIL(bdd_t)) {
            CSPF(node)->bdd = FDC;
        } else {
            CSPF(node)->bdd = bdd_zero(mg);
        }

        st_free_table(tfotfi_table);
        st_free_table(fis_table);
        array_free(node_list);
        array_free(fanout_list);

/* simplify the node */
        if (node->type == INTERNAL) {
            simplify_cspf_node(node, method, dctype, accept, filter, level_table, mg, leaves);
            numin = node_num_fanin(node);
            CSPF(node)->list = array_alloc(node_t * , 0);
            dc_list = CSPF(node)->list;
            for (k = 0; k < numin; k++) {
                fanin = node_get_fanin(node, k);
                if ((node_function(fanin) == NODE_PI) ||
                    ((numin > 15) && (numin * node_num_cube(node) >= 300))) {
                    mspf_node = ALLOC(double_node_t, 1);
                    mspf_node->pos = node_constant(0);
                    mspf_node->neg = node_constant(1);
                } else {
                    mspf_node = compute_mspf(node, fanin);
                }
                array_insert_last(double_node_t * , dc_list, mspf_node);
            }
            update_cspf_of_fanins(node, ANDpi, mg, leaves);
        }
    }
    foreach_node(network, gen, node)
    {
        if ((node->type == INTERNAL) && (node_num_fanin(node) != 0)) {
            dc_list = CSPF(node)->list;
            for (i = 0; i < array_n(dc_list); i++) {
                mspf_node = array_fetch(double_node_t * , dc_list, i);
                node_free(mspf_node->pos);
                node_free(mspf_node->neg);
                FREE(mspf_node);
            }
        }
    }


/* free the space allocated for CSPFs*/
    node_free(ANDpi);
    array_free(network_node_list);
}

/*
 * Because of the use of subset support filter it is possible to add new fanins to a node.
 * If the ODC for such fanins has been already computed, it is necessary to update that
 * ODC. This updating is done here.
 */
static void update_cspf_of_fanins(node, ANDpi, mg, leaves)
        node_t *node;
        node_t *ANDpi;
        bdd_manager *mg;
        st_table *leaves;
{
    int           numin, k, j, m;
    node_t        *fanin, *nd, *tmpnode;
    double_node_t *mspf_node;
    node_t        *dcnode, *pfanin;
    bdd_t         *bdd, *dcbdd;
    array_t       *fanin_list, *node_list, *dc_list;
    st_table      *fofis_table, *fis_table;


    numin = node_num_fanin(node);
    dc_list = CSPF(node)->list;
    node_list = network_tfi(node, 1);

/* make the mspf compatible with the cspf computed for previous edges. */
    fofis_table = st_init_table(st_ptrcmp, st_ptrhash);
    for (m = 0; m < array_n(node_list); m++) {
        nd = array_fetch(node_t * , node_list, m);
        (void) st_insert(fofis_table, (char *) nd, NIL(
        char));
    }
    array_free(node_list);
    for (k = 0; k < numin; k++) {
        fanin = node_get_fanin(node, k);
        if (node_function(fanin) == NODE_PI)
            continue;
        if ((CSPF(fanin)->bdd) == NIL(bdd_t))
            continue;
        fanin_list = network_tfi(fanin, 1);
        fis_table = st_init_table(st_ptrcmp, st_ptrhash);
        for (m = 0; m < array_n(fanin_list); m++) {
            tmpnode = array_fetch(node_t * , fanin_list, m);
            (void) st_insert(fis_table, (char *) tmpnode, NIL(
            char));
        }
        array_free(fanin_list);
        mspf_node = array_fetch(double_node_t * , dc_list, k);
        dcnode    = node_dup(mspf_node->pos);
        for (j = 0; j < k; j++) {
            pfanin = node_get_fanin(node, j);
            mspf_node = array_fetch(double_node_t * , dc_list, j);
            if (node_function(pfanin) != NODE_PI) {
                dcnode = compute_cspf(dcnode, pfanin, mspf_node->neg, fis_table,
                                      ANDpi, fofis_table);
            }
        }
        dcbdd  = bdd_dup(ntbdd_node_to_bdd(dcnode, mg, leaves));
        if (CSPF(node)->bdd != NIL(bdd_t)) {
            bdd = CSPF(node)->bdd;
            bdd = bdd_or(bdd, dcbdd, 1, 1);
            bdd_free(dcbdd);
            dcbdd = bdd;
        }
        ntbdd_free_at_node(dcnode);
        node_free(dcnode);
        bdd = bdd_and(CSPF(fanin)->bdd, dcbdd, 1, 1);
        bdd_free(dcbdd);
        bdd_free(CSPF(fanin)->bdd);
        CSPF(fanin)->bdd = bdd;
        st_free_table(fis_table);
    }
    st_free_table(fofis_table);
}

/* 
 * allocate space for CSPFs.
 */
void
cspf_alloc(node)
        node_t *node;
{
    node->OBS = (char *) ALLOC(cspf_type_t, 1);
    CSPF(node)->level = 0;
    CSPF(node)->node  = NIL(node_t);
    CSPF(node)->list  = NIL(array_t);
    CSPF(node)->bdd   = NIL(bdd_t);
    CSPF(node)->set   = NIL(var_set_t);
}


/* free space allocated for CSPFs.
 *
 */
void
cspf_free(node)
        node_t *node;
{
    FREE(node->OBS);
}


/* compute maximum set of permissible functions for an edge.
 *
 */
static double_node_t *
compute_mspf(node, fanin)
        node_t *node;
        node_t *fanin;
{
    node_t        *f_x, *f_xbar, *h, *hbar;
    node_t        *p, *pnot, *node_cube, *tmpnode;
    double_node_t *mspf_node;
    int           index;
    pset          last, setp;

    mspf_node = ALLOC(double_node_t, 1);
    if ((node_function(node) == NODE_PO) || (node_function(fanin) == NODE_PI)) {
        mspf_node->pos = node_constant(0);
        mspf_node->neg = node_constant(1);
        return (mspf_node);
    }
    f_x    = node_constant(0);
    f_xbar = node_constant(0);
    h      = node_constant(0);
    index  = node_get_fanin_index(node, fanin);
    foreach_set(node->F, last, setp)
    {
        node_cube = cube_to_node(node, setp, index);
        if (GETINPUT(setp, index) == TWO) {
            tmpnode = node_or(node_cube, h);
            node_free(h);
            node_free(node_cube);
            h = tmpnode;
            continue;
        }
        if (GETINPUT(setp, index) == ONE) {
            tmpnode = node_or(node_cube, f_x);
            node_free(f_x);
            node_free(node_cube);
            f_x = tmpnode;
            continue;
        }
        assert(GETINPUT(setp, index) == ZERO);
        tmpnode = node_or(node_cube, f_xbar);
        node_free(f_xbar);
        node_free(node_cube);
        f_xbar = tmpnode;
    }
    p      = node_xnor(f_x, f_xbar);
    pnot   = node_not(p);
    mspf_node->pos = node_or(p, h);
    hbar = node_not(h);
    mspf_node->neg = node_and(pnot, hbar);
    node_free(f_x);
    node_free(f_xbar);
    node_free(h);
    node_free(hbar);
    node_free(p);
    node_free(pnot);
    return (mspf_node);
}

/*
 * Generate a node representing the cube which is a part of f.
 * If index is positive, also smooth that particular variable from
 * the cube.
*/
static node_t *cube_to_node(node, setp, index)
        node_t *node;
        pset setp;
        int index;
{
    node_t *n1, *n2, *n3;
    node_t *fanin;
    int    k, j;

    n1 = node_constant(1);
    for (k = 0; k < node->nin; k++) {
        if (k == index)
            continue;
        if ((j = GETINPUT(setp, k)) == TWO)
            continue;
        if (j == ONE) {
            fanin = node_get_fanin(node, k);
            n2    = node_literal(fanin, 1);
            n3    = node_and(n1, n2);
            node_free(n2);
            node_free(n1);
            n1 = n3;
            continue;
        }
        assert(j == ZERO);
        fanin = node_get_fanin(node, k);
        n2    = node_literal(fanin, 0);
        n3    = node_and(n1, n2);
        node_free(n2);
        node_free(n1);
        n1 = n3;
    }
    return (n1);
}










/* Makes the don't care computed for an edge compatible with a previous
 * edge. If the obs. don't care for current edge is D and for the previous
 * edge DP, and the Boolean variable at the node that previous edge
 * originates from is x, then we find   Dx . Dx' + D . DP'.
 */
static node_t *
compute_cspf(DC, pfanin, bool_diff, fis_table, ANDpi, fofis_table)
        node_t *DC;
        node_t *pfanin;
        node_t *bool_diff;
        st_table *fis_table; /* used for filtering. */
        node_t *ANDpi; /* ANDing of the PIs used for filtering. */
        st_table *fofis_table; /* used for collapsing. */
{
    node_t *q, *n0, *n1, *f0, *f1, *ndc;
    bdd_t  *bdd;
    int    l, numin;
    node_t *dcfanin;
    char   *dummy;


    if ((bdd = CSPF(pfanin)->bdd) == NIL(bdd_t)) {
        return (DC);
    }

    if (bdd_is_tautology(bdd, 0)) {
        return (DC);
    }

/*finds Dx . Dx', given x is the previous fanin*/
    f0 = node_literal(pfanin, 0);
    f1 = node_literal(pfanin, 1);
    n0 = node_cofactor(DC, f0);
    n1 = node_cofactor(DC, f1);
    q  = node_and(n0, n1);


    node_free(n0);
    node_free(n1);
    node_free(f0);
    node_free(f1);

    n0 = node_dup(bool_diff);

/* filter DP before computing DP' . D. */
    if (node_num_fanin(n0) > 0) {
        n0 = simp_obsdc_filter(n0, fis_table, 3, ANDpi);
        numin = node_num_fanin(n0);
        for (l = 0; l < numin; l++) {
            dcfanin = node_get_fanin(n0, l);
            if (!st_lookup(fis_table, (char *) dcfanin, &dummy)) {
                if (node_function(dcfanin) != NODE_PI) {
                    if (st_lookup(fofis_table, (char *) dcfanin, &dummy)) {
                        (void) node_collapse(n0, dcfanin);
                        l--;
                        numin = node_num_fanin(n0);
                    }
                }
            }
        }
    }
    n1 = node_and(DC, n0);
    node_free(n0);

    ndc = node_or(q, n1);
    node_free(q);
    node_free(n1);
    node_free(DC);
    return (ndc);
}

/* Returns the CSPF for a node if it exists.
 *
 */
bdd_t *cspf_bdd_dc(f, mg)
        node_t *f;
        bdd_manager *mg;
{
    bdd_t *p;

    if ((p = CSPF(f)->bdd) != NIL(bdd_t)) {
        return (bdd_dup(p));
    } else {
        (void) fprintf(siserr, "cspf does not exist\n");
        return (bdd_zero(mg));
    }
}





/* A table of transitive fanouts of transitive fanins of a node.
 *
 */
static st_table *
cspf_inout_table(f, il, ol)
        node_t *f;
        int il, ol;
{
    st_table *finout_table;
    array_t  *fanin_list, *fanout_list;
    node_t   *fanin, *np;
    int      i, j;

    finout_table = st_init_table(st_ptrcmp, st_ptrhash);
    fanin_list = network_tfi(f, il);
    for (i     = 0; i < array_n(fanin_list); i++) {
        fanin = array_fetch(node_t * , fanin_list, i);
        (void) st_insert(finout_table, (char *) fanin, NIL(
        char));
        fanout_list = network_tfo(fanin, ol);
        for (j      = 0; j < array_n(fanout_list); j++) {
            np = array_fetch(node_t * , fanout_list, j);
            (void) st_insert(finout_table, (char *) np, NIL(
            char));
        }
        array_free(fanout_list);
    }
    array_free(fanin_list);
    return (finout_table);
}


/*
 *
 */
void odc_alloc(node)
        node_t *node;
{
    node->OBS = (char *) ALLOC(odc_type_t, 1);
    ODC(node)->order = 0;
    ODC(node)->level = 0;
    ODC(node)->value = 0;
    ODC(node)->f     = NIL(bdd_t);
    ODC(node)->var   = NIL(bdd_t);
    ODC(node)->vodc = NIL(array_t);
}


/*
 *
 */
void
odc_free(node)
        node_t *node;
{
    FREE(node->OBS);
}

void find_odc_level(network)
        network_t *network;
{
    array_t *nodevec;
    int     i, j;
    node_t  *np, *fanin;
    int     level, tmp;

    nodevec = network_dfs(network);
    for (i = 0; i < array_n(nodevec); i++) {
        np = array_fetch(node_t * , nodevec, i);
        if (node_function(np) == NODE_PI) {
            ODC(np)->level = 0;
            continue;
        }
        level = 0;
        foreach_fanin(np, j, fanin)
        {
            tmp       = ODC(fanin)->level;
            if (level < tmp)
                level = tmp;
        }
        level++;
        ODC(np)->level = level;
    }
    array_free(nodevec);
}

int level_node_cmp1(obj1, obj2)
        char **obj1;
        char **obj2;
{
    node_t *node1, *node2;

    node1 = *((node_t **) obj1);
    node2 = *((node_t **) obj2);
    if (ODC(node1)->level > ODC(node2)->level)
        return (-1);
    if (ODC(node1)->level < ODC(node2)->level)
        return (1);

    if (ODC(node1)->value > ODC(node2)->value)
        return (-1);
    if (ODC(node1)->value < ODC(node2)->value)
        return (1);

    if (ODC(node1)->order > ODC(node2)->order)
        return (-1);
    if (ODC(node1)->order < ODC(node2)->order)
        return (1);
    return (0);
}

int level_node_cmp2(obj1, obj2)
        char **obj1;
        char **obj2;
{
    node_t *node1, *node2;

    node1 = *((node_t **) obj1);
    node2 = *((node_t **) obj2);
    if (ODC(node1)->level > ODC(node2)->level)
        return (-1);
    if (ODC(node1)->level < ODC(node2)->level)
        return (1);

    if (ODC(node1)->order > ODC(node2)->order)
        return (-1);
    if (ODC(node1)->order < ODC(node2)->order)
        return (1);
    return (0);
}
int level_node_cmp3(obj1, obj2)
        char **obj1;
        char **obj2;
{
    node_t *node1, *node2;

    node1 = *((node_t **) obj1);
    node2 = *((node_t **) obj2);
    if (ODC(node1)->level < ODC(node2)->level)
        return (-1);
    if (ODC(node1)->level > ODC(node2)->level)
        return (1);
    if (node_num_cube(node1) < node_num_cube(node2))
        return (-1);
    if (node_num_cube(node1) > node_num_cube(node2))
        return (1);

    return (0);
}

/*
 *
 */
int odc_value(nodep)
        node_t *nodep;
{
    node_t       *np;
    int          value;
    lsGen        gen;
    st_generator *sgen;
    st_table     *table;

    /* if all outputs of this nodes are primary output, its value is oo */
    value = INFINITY;
    if (nodep->type == PRIMARY_OUTPUT) {
        return value;
    }
    foreach_fanout(nodep, gen, np)
    {
        if (np->type != PRIMARY_OUTPUT) {
            value = 0;
        }
    }
    if (value != 0) {
        return value;
    }

    /* compute the number of times the function is used */
    table = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_fanout(nodep, gen, np)
    {
        st_insert(table, (char *) np, NIL(
        char));
    }
    st_foreach_item(table, sgen, (char **) &np, NIL(
    char *)) {
        if (np->type != PRIMARY_OUTPUT) {
            value += factor_num_used(np, nodep);
        }
    }
    st_free_table(table);
    return (value);
}
