
#include "sis.h"

typedef struct cpexdc_struct {
    node_t *node;
} cpexdc_type_t;

#define OBS cspf
#define CPEXDC(node)  ((cpexdc_type_t *) (node)->OBS)

static void duplicate_list();

static void copy_list();

static void reset_io();

extern void network_rehash_names();

static void
cpexdc_alloc(node)
        node_t *node;
{
    node->OBS = (char *) ALLOC(cpexdc_type_t, 1);
    CPEXDC(node)->node = NIL(node_t);
}

static void
cpexdc_free(node)
        node_t *node;
{
    FREE(node->OBS);
}


network_t *
network_alloc() {
    network_t *net;

    net = ALLOC(network_t, 1);
    net->net_name = NIL(
    char);
    net->name_table       = st_init_table(strcmp, st_strhash);
    net->short_name_table = st_init_table(strcmp, st_strhash);
    net->nodes            = lsCreate();
    net->pi               = lsCreate();
    net->po               = lsCreate();
    net->original         = NIL(network_t);
    net->dc_network       = NIL(network_t);
    net->area_given       = 0;
    net->area             = 0.0;
    net->bdd_list         = lsCreate();
    net->default_delay    = NIL(
    char);
#ifdef SIS
    net->latch_table = st_init_table(st_ptrcmp, st_ptrhash);
    net->latch = lsCreate();
    net->stg = NIL(graph_t);
    net->astg = NIL(astg_t);
    network_clock_alloc(net);
#endif /* SIS */
    return net;
}


void
network_free(net)
        network_t *net;
{
    if (net != NIL(network_t)) {
        FREE(net->net_name);
        st_free_table(net->name_table);
        st_free_table(net->short_name_table);
        LS_ASSERT(lsDestroy(net->pi, (void (*)()) 0));
        LS_ASSERT(lsDestroy(net->po, (void (*)()) 0));
        LS_ASSERT(lsDestroy(net->nodes, node_free));
        network_free(net->original);
        network_free(net->dc_network);
        LS_ASSERT(lsDestroy(net->bdd_list, (void (*)()) 0));
        delay_network_free(net);
#ifdef SIS
        st_free_table(net->latch_table);
        LS_ASSERT(lsDestroy(net->latch, free));
        stg_free(net->stg);
        net->stg = NIL(graph_t);
        network_clock_free(net);
    astg_free (net->astg);
#endif /* SIS */
        FREE(net);
    }
}


network_t *
network_dup(old)
        network_t *old;
{
    network_t *new;

    if (old == NIL(network_t)) {
        return (NIL(network_t));
    }
    new = network_alloc();

    if (old->net_name != NIL(char)) {
        new->net_name = util_strsav(old->net_name);
    }

    duplicate_list(old->nodes, new->nodes, new);
    copy_list(old->pi, new->pi);
    copy_list(old->po, new->po);

    network_rehash_names(new, /* long */ 1, /* short */ 1);

    /* this makes sure fanout pointers are up-to-date */
    reset_io(new->nodes);

    /* This makes sure that the global delay information is copied */
    delay_network_dup(new, old);

    /* this makes sure that MAP(node)->save_binding pointers are up-to-date */
    map_network_dup(new);

    new->original   = network_dup(old->original);
    new->dc_network = network_dup(old->dc_network);
    new->area_given = old->area_given;
    new->area       = old->area;
#ifdef SIS
    copy_latch_info(old->latch, new->latch, new->latch_table);
    new->stg = stg_dup(old->stg);
    new->astg = astg_dup(old->astg);
    network_clock_dup(old, new);
#endif /* SIS */
    return new;
}

network_num_pi(network)
network_t *network;
{
return
lsLength(network
->pi);
}


network_num_po(network)
network_t *network;
{
return
lsLength(network
->po);
}


network_num_internal(network)
network_t *network;
{
return
lsLength(network
->nodes) -
(
lsLength(network
->pi) +
lsLength(network
->po));
}


node_t *
network_get_pi(network, index)
        network_t *network;
        int index;
{
    int    i;
    lsGen  gen;
    node_t *node;

    i = 0;
    foreach_primary_input(network, gen, node)
    {
        if (i++ == index) {
            (void) lsFinish(gen);
            return node;
        }
    }
    return 0;
}


node_t *
network_get_po(network, index)
        network_t *network;
        int index;
{
    int    i;
    lsGen  gen;
    node_t *node;

    i = 0;
    foreach_primary_output(network, gen, node)
    {
        if (i++ == index) {
            (void) lsFinish(gen);
            return node;
        }
    }
    return 0;
}

void
network_add_primary_input(network, node)
        network_t *network;
        node_t *node;
{
    node->type = PRIMARY_INPUT;
    node->nin  = 0;
    FREE(node->fanin);
    node->fanin = 0;
    network_add_node(network, node);
}


node_t *
network_add_primary_output(network, node)
        network_t *network;
        node_t *node;
{
    node_t *output;

    output = node_alloc();
    output->type  = PRIMARY_OUTPUT;
    output->nin   = 1;
    output->fanin = ALLOC(node_t * , 1);
    output->fanin[0] = node;
    network_add_node(network, output);
    network_swap_names(network, node, output);
    return output;
}


void
network_add_node(network, node)
        network_t *network;
        node_t *node;
{
    int       value;
    lsHandle  handle;
    network_t *dc_net;

    if (node->type == UNASSIGNED) node->type = INTERNAL;

    if (node->name == NIL(char)) node_assign_name(node);
    if (node->short_name == NIL(char)) node_assign_short_name(node);

    /*
     *  force made-up names to be unique when added to network
     */
    if (node_is_madeup_name(node->name, &value)) {

        dc_net = network->dc_network;
        while (((dc_net != NIL(network_t) && st_lookup(dc_net->name_table, node->name, NIL(char *))))
        || st_lookup(network->name_table, node->name, NIL(
        char *))) {
            node_assign_name(node);
        }
        if (network->dc_network != NIL(network_t)) {
            while (st_lookup(network->name_table, node->name, NIL(char *))) {
                node_assign_name(node);
            }
        }
    }
    if (st_insert(network->name_table, node->name, (char *) node)) {
        fail("attempt to add node with same name as some existing node");
    }


    /*
     *  force short names to be unique when added to network
     */
    while (st_lookup(network->short_name_table, node->short_name, NIL(char *))){
        node_assign_short_name(node);
    }
    if (st_insert(network->short_name_table, node->short_name, (char *) node)) {
        fail("attempt to add node with same short_name as some existing node");
    }


    if (node->type == PRIMARY_INPUT) {
        LS_ASSERT(lsNewEnd(network->pi, (lsGeneric) node, LS_NH));
    }
    if (node->type == PRIMARY_OUTPUT) {
        LS_ASSERT(lsNewEnd(network->po, (lsGeneric) node, LS_NH));
    }
    LS_ASSERT(lsNewEnd(network->nodes, (lsGeneric) node, &handle));
    node->network    = network;
    node->net_handle = handle;

    /* patch the fanout lists for our fanin's */
    fanin_add_fanout(node);
}

void
network_delete_node(network, node)
        network_t *network;
        node_t *node;
{
    char  *data;
    lsGen gen;

    if (node->network != network) {
        fail("network_delete_node: attempt to delete node not in network");
    }

    gen = lsGenHandle(node->net_handle, &data, LS_AFTER);
    assert((node_t *) data == node);        /* Paranoid */
    network_delete_node_gen(network, gen);
    LS_ASSERT(lsFinish(gen));
}


void
network_delete_node_gen(network, gen)
        network_t *network;
        lsGen gen;
{
    char   *key;
    node_t *node;

    /* Unlink from the node list */
    LS_ASSERT(lsDelBefore(gen, (lsGeneric * ) & node));

    /* force deletion from PI/PO lists */
    network_change_node_type(network, node, INTERNAL);

    key = node->name;
    if (!st_delete(network->name_table, &key, NIL(char *))) {
        fail("network_delete_node_gen: node name not in name table");
    }
    assert(key == node->name);

    key = node->short_name;
    if (!st_delete(network->short_name_table, &key, NIL(char *))) {
        fail("network_delete_node_gen: node short name not in name table");
    }
    assert(key == node->short_name);

    /* patch the fanout lists for our fanins */
    fanin_remove_fanout(node);

    node->network    = 0;        /* avoid recursion ... */
    node->net_handle = 0;
    node_free(node);
}

node_t *
network_find_node(network, name)
        network_t *network;
        char *name;
{
    char *dummy;

    if (st_lookup(network->name_table, name, &dummy)) {
        return (node_t *) dummy;
    } else {
        return 0;
    }
}

static int
delete_from_list(list, node)
        lsList list;
        node_t *node;
{
    lsGen  gen;
    node_t *data;

    lsForeachItem(list, gen, data)
    {
        if (data == node) {
            LS_ASSERT(lsDelBefore(gen, (lsGeneric * ) & data));
            LS_ASSERT(lsFinish(gen));
            return 1;
        }
    }
    return 0;
}



void
network_change_node_type(network, node, new_type)
        network_t *network;
        node_t *node;
        node_type_t new_type;
{
    if (node->type == PRIMARY_INPUT) {
        if (!delete_from_list(network->pi, node)) {
            fail("network_change_node_type: PI node not in PI list");
        }
    } else if (node->type == PRIMARY_OUTPUT) {
        if (!delete_from_list(network->po, node)) {
            fail("network_change_node_type: PO node not in PO list");
        }
    }

    node->type = new_type;
    if (node->type == PRIMARY_INPUT) {
        LS_ASSERT(lsNewEnd(network->pi, (lsGeneric) node, LS_NH));
    } else if (node->type == PRIMARY_OUTPUT) {
        LS_ASSERT(lsNewEnd(network->po, (lsGeneric) node, LS_NH));
    }
}

char *
network_name(network)
        network_t *network;
{
    if (network->net_name == NIL(char)) {
        return "unknown";
    } else {
        return network->net_name;
    }
}


void
network_set_name(network, name)
        network_t *network;
        char *name;
{
    FREE(network->net_name);
    network->net_name = util_strsav(name);
}

static void
reset_io(list)
        lsList list;
{
    lsGen  gen;
    node_t *node, *fanin;
    int    i;

    lsForeachItem(list, gen, node)
    {
        foreach_fanin(node, i, fanin)
        {
            node->fanin[i] = fanin->copy;
        }
        fanin_add_fanout(node);
    }
}


static void
copy_list(list, newlist)
        lsList list, newlist;
{
    lsGen  gen;
    node_t *node;

    lsForeachItem(list, gen, node)
    {
        LS_ASSERT(lsNewEnd(newlist, (lsGeneric) node->copy, LS_NH));
    }
}


static void
duplicate_list(list, newlist, newnetwork)
        lsList list, newlist;
        network_t *newnetwork;
{
    lsGen    gen;
    node_t   *node, *newnode;
    lsHandle handle;

    lsForeachItem(list, gen, node)
    {
        newnode = node_dup(node);
        node->copy = newnode;
        LS_ASSERT(lsNewEnd(newlist, (lsGeneric) newnode, &handle));
        newnode->network    = newnetwork;
        newnode->net_handle = handle;
    }
}


#ifdef SIS

void
copy_latch_info(list, newlist, newtable)
lsList list, newlist;
st_table *newtable;
{
    lsGen gen;
    latch_t *latch, *l;
    node_t *input, *output;

    lsForeachItem(list, gen, latch) {
        l = latch_alloc();
        input = (latch_get_input(latch))->copy;
        output = (latch_get_output(latch))->copy;
        latch_set_input(l, input);
        latch_set_output(l, output);
        latch_set_initial_value(l, latch_get_initial_value(latch));
        latch_set_current_value(l, latch_get_current_value(latch));
        latch_set_type(l, latch_get_type(latch));
        latch_set_gate(l, latch_get_gate(latch));
        if (latch_get_control(latch) != NIL(node_t)) {
            latch_set_control(l, (latch_get_control(latch)->copy));
        }
        (void) st_insert(newtable, (char *) input, (char *) l);
        (void) st_insert(newtable, (char *) output, (char *) l);
        LS_ASSERT(lsNewEnd(newlist, (lsGeneric) l, LS_NH));
    }
    return;
}

#endif /* SIS */

void
network_change_node_name(network, node, new_name)
        network_t *network;
        node_t *node;
        char *new_name;
{
    char *key;

    key = node->name;
    if (!st_delete(network->name_table, &key, NIL(char *))) {
        fail("network_change_name: node not found in name table");
    }
    assert(key == node->name);

    FREE(node->name);
    node->name = new_name;
    if (st_insert(network->name_table, node->name, (char *) node)) {
        fail("network_change_name: node with same name already exists");
    }
}



void
network_change_node_short_name(network, node, new_name)
        network_t *network;
        node_t *node;
        char *new_name;
{
    char *key;

    key = node->short_name;
    if (!st_delete(network->short_name_table, &key, NIL(char *))) {
        fail("change_short_name: node not found in name table");
    }
    assert(key == node->short_name);

    node->short_name = new_name;
    if (st_insert(network->short_name_table, node->short_name, (char *) node)) {
        fail("change_short_name: node with same name already exists");
    }
}

void
network_swap_names(network, node1, node2)
        network_t *network;
        node_t *node1;
        node_t *node2;
{
    char *key;

    key = node1->name;
    assert(st_delete(network->name_table, &key, NIL(
    char *)));
    key = node2->name;
    assert(st_delete(network->name_table, &key, NIL(
    char *)));
    key = node1->short_name;
    assert(st_delete(network->short_name_table, &key, NIL(
    char *)));
    key = node2->short_name;
    assert(st_delete(network->short_name_table, &key, NIL(
    char *)));

    key = node1->name;
    node1->name = node2->name;
    node2->name = key;

    key = node1->short_name;
    node1->short_name = node2->short_name;
    node2->short_name = key;

    assert(!st_insert(network->name_table, node1->name, (char *) node1));
    assert(!st_insert(network->name_table, node2->name, (char *) node2));
    assert(!st_insert(network->short_name_table, node1->short_name, (char *) node1));
    assert(!st_insert(network->short_name_table, node2->short_name, (char *) node2));
}

lsList
network_bdd_list(network)
        network_t *network;
{
    return (network->bdd_list);
}

lsHandle
network_add_bdd(network, bdd)
        network_t *network;
        bdd_manager *bdd;
{
    lsHandle h;
    (void) lsNewBegin(network->bdd_list, (lsGeneric) bdd, &h);
    return (h);
}


network_t *
network_dc_network(network)
        network_t *network;
{
    return (network->dc_network);
}

/* connects primary inputs and primary outputs of the two network through
 * a hash table.
 */
st_table *
attach_dcnetwork_to_network(network)
        network_t *network;
{
    st_table  *node_exdc_table;
    node_t    *node, *nodedc;
    network_t *DC_network;
    int       i, j;

    if (network == NULL) {
        return NIL(st_table);
    }
    DC_network = network_dc_network(network);
    if (DC_network == NULL) {
        return NIL(st_table);
    }
    node_exdc_table = st_init_table(st_ptrcmp, st_ptrhash);

    for (i = 0; i < network_num_po(network); i++) {
        node = network_get_po(network, i);
        for (j = 0; j < network_num_po(DC_network); j++) {
            nodedc = network_get_po(DC_network, j);
            if (strcmp(nodedc->name, node->name) == 0)
                (void) st_insert(node_exdc_table, (char *) node, (char *) nodedc);
        }
    }
    for (i = 0; i < network_num_pi(network); i++) {
        node = network_get_pi(network, i);
        for (j = 0; j < network_num_pi(DC_network); j++) {
            nodedc = network_get_pi(DC_network, j);
            if (strcmp(nodedc->name, node->name) == 0) {
                (void) st_insert(node_exdc_table, (char *) nodedc, (char *) node);
            }
        }
    }
    return node_exdc_table;
}

network_t *
or_net_dcnet(network)
        network_t *network;
{
    node_t    *node, *dcnode, *dcfanin;
    node_t    *n1, *n2, *n3, *n4;
    node_t    *tempnode;
    int       i, j, k;
    pset      last, setp;
    network_t *DC_network;
    array_t   *dc_list;
    st_table  *node_exdc_table;
    node_t    *po;
    network_t *net;
    char      *dummy;

    net = network_dup(network);
    node_exdc_table = attach_dcnetwork_to_network(net);
    DC_network      = network_dc_network(net);

    dc_list = network_dfs(DC_network);
    for (i = 0; i < array_n(dc_list); i++) {
        dcnode = array_fetch(node_t * , dc_list, i);
        cpexdc_alloc(dcnode);
        if (node_function(dcnode) == NODE_PI) {
            assert(st_lookup(node_exdc_table, (char *) dcnode, &dummy));
            node = (node_t *) dummy;
            CPEXDC(dcnode)->node = node;
            continue;
        }
        if (node_function(dcnode) == NODE_PO) {
            dcfanin = node_get_fanin(dcnode, 0);
            CPEXDC(dcnode)->node = CPEXDC(dcfanin)->node;
            continue;
        }

        n3 = node_constant(0);
        foreach_set(dcnode->F, last, setp)
        {
            n1 = node_constant(1);
            for (k = 0; k < dcnode->nin; k++) {
                if ((j = GETINPUT(setp, k)) != TWO) {
                    if (j == ONE) {
                        dcfanin  = node_get_fanin(dcnode, k);
                        tempnode = CPEXDC(dcfanin)->node;
                        n4       = node_literal(tempnode, 1);
                        n2       = node_and(n1, n4);
                        node_free(n4);
                        node_free(n1);
                        n1 = n2;
                    } else {
                        dcfanin  = node_get_fanin(dcnode, k);
                        tempnode = CPEXDC(dcfanin)->node;
                        n4       = node_literal(tempnode, 0);
                        n2       = node_and(n1, n4);
                        node_free(n4);
                        node_free(n1);
                        n1 = n2;
                    }
                }
            }
            n2     = node_or(n1, n3);
            node_free(n1);
            node_free(n3);
            n3 = n2;
        }
        CPEXDC(dcnode)->node = n3;
        network_add_node(net, n3);
    }

    for (i = 0; i < network_num_po(net); i++) {
        po = network_get_po(net, i);
        assert(st_lookup(node_exdc_table, (char *) po, &dummy));
        node = (node_t *) dummy;
        n1   = CPEXDC(node)->node;
        n2   = node_get_fanin(po, 0);
        if (node_function(n2) == NODE_PI) {
            n4 = node_literal(n2, 1);
            n3 = node_or(n1, n4);
            node_free(n4);
        } else {
            n3 = node_or(n1, n2);
        }
        network_add_node(net, n3);
        node_patch_fanin(po, n2, n3);
    }
    for (i = 0; i < array_n(dc_list); i++) {
        dcnode = array_fetch(node_t * , dc_list, i);
        cpexdc_free(dcnode);
    }
    st_free_table(node_exdc_table);
    array_free(dc_list);
    return (net);
}

/* find the external don't care for each primary output and express it in
 * terms of the primary inputs of the care network.
 * each primary output or primary input of the don't care network corresponds
 * to a primary ouput or primary input of the care network through a hash
 * table.
 */
node_t *
find_ex_dc(ponode, node_exdc_table)
        node_t *ponode; /*primary output of the care network*/
        st_table *node_exdc_table;

{
    node_t *dcnodep;
    node_t *n1, *n2, *n3, *n4;
    node_t *tempnode;
    int    j, k;
    pset   last, setp;
    char   *dummy;

/* find the corresponding PO in the don't care network from the hash table*/
    if (node_exdc_table == NIL(st_table)) {
        return (node_constant(0));
    }
    if (!st_lookup(node_exdc_table, (char *) ponode, &dummy))
        return (node_constant(0));
    tempnode = (node_t *) dummy;
    dcnodep  = node_get_fanin(tempnode, 0);

/* generate external don't care function for PO in the care network*/
    n3 = node_constant(0);
    if (dcnodep->F == 0) return n3;
    foreach_set(dcnodep->F, last, setp)
    {
        n1 = node_constant(1);
        for (k = 0; k < dcnodep->nin; k++) {
            if ((j = GETINPUT(setp, k)) != TWO) {
                if (j == ONE) {
                    tempnode = node_get_fanin(dcnodep, k);
                    (void) st_lookup(node_exdc_table, (char *) tempnode, &dummy);
                    tempnode = (node_t *) dummy;
                    n4       = node_literal(tempnode, 1);
                    n2       = node_and(n1, n4);
                    node_free(n4);
                    node_free(n1);
                    n1 = n2;
                } else {
                    tempnode = node_get_fanin(dcnodep, k);
                    (void) st_lookup(node_exdc_table, (char *) tempnode, &dummy);
                    tempnode = (node_t *) dummy;
                    n4       = node_literal(tempnode, 0);
                    n2       = node_and(n1, n4);
                    node_free(n4);
                    node_free(n1);
                    n1 = n2;
                }
            }
        }
        n2     = node_or(n1, n3);
        node_free(n1);
        node_free(n3);
        n3 = n2;
    }
    return (n3);
}


