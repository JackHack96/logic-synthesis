
/* file @(#)prim.c	1.5 */
/* last modified on 8/8/91 at 17:02:12 */
#include "sis.h"
#include "../include/map_int.h"


static prim_t *make_primitive();

static int make_edge_list();

static void reorder();

static st_table *arc_hash_init();

static int arc_find_or_add();

static void arc_hash_end();

static prim_t *prim_alloc();

static node_t *prim_first_po();

static int prim_feeds_only_real_po();

static char errmsg[1024];


int
map_network_to_prim(network, prim_p)
        network_t *network;
        prim_t **prim_p;
{
    prim_t   *prim;
    st_table *isomorphic_sons;

    /* find which nodes have isomorphic children */
    isomorphic_sons = map_find_isomorphisms(network);

    /* Map the network into the 'prim' structure used for matching */
    prim = make_primitive(network, isomorphic_sons);
    st_free_table(isomorphic_sons);

    *prim_p = prim;
    return prim != 0;
}

/*
 *  Map a network into a primitive pattern used for matching
 */

static prim_t *
make_primitive(network, isomorphic_sons)
        network_t *network;
        st_table *isomorphic_sons;
{
    node_t      *node, *po;
    prim_t      *prim;
    prim_node_t *prim_node;
    lsGen       gen;
    int         i;

    /* give each node of 'network' a map record -- we use 'binding' */
    map_setup_network(network);

    /* Allocate a header for the primitive information */
    prim = prim_alloc();

#ifdef SIS
    prim->latch_type = network_gate_type(network);
    if (prim->latch_type == (int) UNKNOWN) {
      FREE(prim);
      return NIL(prim_t);
    }
#endif /* SIS */

    /* only single-output gates for now */
    assert(network_num_po(network) == 1);

    /* Create a primitive node for each node in the network */
    foreach_node(network, gen, node)
    {
#ifdef SIS
        if (node->type == INTERNAL || node->type == PRIMARY_INPUT ||
        (node->type == PRIMARY_OUTPUT && network_gate_type(network) != (int) COMBINATIONAL)) {
#else
        if (node->type == INTERNAL || node->type == PRIMARY_INPUT) {
#endif /* SIS */

            prim_node = ALLOC(prim_node_t, 1);
            prim_node->type    = node->type;
            prim_node->nfanin  = node_num_fanin(node);
            prim_node->nfanout = node_num_fanout(node);
            prim_node->binding = 0;

            /* Label PO/PI nodes.
             * Combinational gate - internal node which feeds PO is labeled as PO
             * Latch - a latch input node is labeled as PO.
             */
#ifdef SIS
            if (node_type(node) == PRIMARY_OUTPUT && network_gate_type(network) != (int) COMBINATIONAL) {
              prim_node->type = PRIMARY_OUTPUT;
              prim_node->name = util_strsav(node->name);
            } else if (prim_feeds_only_real_po(node, &po)) {
#else
            if (prim_feeds_only_real_po(node, &po)) {
#endif /* SIS */
                prim_node->type = PRIMARY_OUTPUT;
                prim_node->name = util_strsav(po->name);
            } else if (prim_node->type == PRIMARY_INPUT) {
                prim_node->name = util_strsav(node->name);
            } else {
                /* only required for debugging ... */
                prim_node->name = util_strsav(node->name);
            }

            prim_node->isomorphic_sons = 0;
#ifdef SIS
            /* do not check for isomorphic sons for latches */
            if (prim->latch_type == (int) COMBINATIONAL) {
#endif /* SIS */
            if (st_lookup(isomorphic_sons, (char *) node, NIL(char *))) {
                prim_node->isomorphic_sons = 1;
            }
#ifdef SIS
            }
            /* check for latch outputs */
            prim_node->latch_output = 0;
            if ( IS_LATCH_END(node) && node_type(node) == PRIMARY_INPUT ) {
          prim_node->latch_output = 1;
            }
#endif /* SIS */

            /* record the association between 'node' and 'prim_node' */
            MAP(node)->binding = prim_node;

            /* add to the linked list of nodes for the primitive */
            prim_node->next = prim->nodes;
            prim->nodes     = prim_node;
        }
    }

    /* save direct pointers to the inputs */
    prim->inputs = ALLOC(prim_node_t * , network_num_pi(network));
    i = 0;
    foreach_primary_input(network, gen, node)
    {
        prim->inputs[i++] = MAP(node)->binding;
    }
    prim->ninputs = i; /* sequential support */

    /* save direct pointers to the outputs */
    /* We only support single-output gates now*/
    prim->outputs  = ALLOC(prim_node_t * , 1);
    prim->noutputs = 1;
    foreach_primary_output(network, gen, node)
    {
#ifdef SIS
        if (prim->latch_type == (int) COMBINATIONAL) {
#endif /* SIS */
        prim->outputs[0] = MAP(node_get_fanin(node, 0))->binding;
#ifdef SIS
        } else {
          prim->outputs[0] = MAP(node)->binding;
        }
#endif /* SIS */
    }

    if (!make_edge_list(network, prim)) {
        prim_free(prim);
        return NIL(prim_t);
    } else {
        return prim;
    }
}

static prim_edge_t *last_edge;

static int
make_edge_list(network, prim)
        network_t *network;
        prim_t *prim;
{
    st_table    *visited, *arc_table;
    prim_edge_t edge;
    node_t      *po, *node;
    int         retcode;
    lsGen       gen;

    if (network_num_po(network) < 1) {
        (void) sprintf(errmsg, "network '%s' has no outputs ?",
                       network_name(network));
        read_error(errmsg);
        return 0;
    }

    /* get the first output.
     */
    po = prim_first_po(network);

    /* visiting the first output should reach all nodes ... */
    visited   = st_init_table(st_ptrcmp, st_ptrhash);
    arc_table = arc_hash_init();
    last_edge = &edge;
    reorder(po, NIL(node_t), DIR_IN, arc_table, visited);
    prim->edges = edge.next;
    arc_hash_end(arc_table);

    /* Check that all nodes were reached from the first output */
    retcode = 1;
    foreach_node(network, gen, node)
    {
        if (!st_lookup(visited, (char *) node, NIL(char *))) {
            (void) sprintf(errmsg, "network '%s' is not connected",
                           network_name(network));
            read_error(errmsg);
            retcode = 0; /* some node not visited */
            LS_ASSERT(lsFinish(gen));
            break;
        }
    }
    st_free_table(visited);

    return retcode;
}

static void
reorder(node, prev, dir, arc_table, visited)
        node_t *node, *prev;
        edge_dir_t dir;
        st_table *arc_table;
        st_table *visited;
{
    int         i;
    prim_edge_t *edge;
    node_t      *fanin, *fanout;
    lsGen       gen;

    /* Record new edge */
#ifdef SIS
    if ((node->type != PRIMARY_OUTPUT) ||
    (node->type == PRIMARY_OUTPUT && network_gate_type(node->network) != (int) COMBINATIONAL)) {
#else
    if (node->type != PRIMARY_OUTPUT) {
#endif /* SIS */
        edge = ALLOC(prim_edge_t, 1);
        edge->this_node      = MAP(node)->binding;
        edge->connected_node = prev == NIL(node_t) ? 0 : MAP(prev)->binding;
        edge->dir            = dir;
        edge->next           = NIL(prim_edge_t);
        last_edge->next      = edge;
        last_edge = last_edge->next;
    }

    /* Only need to visit all arcs from a node once */
    if (!st_find_or_add(visited, (char *) node, NIL(char **))) {

        /* Traverse all in-coming arcs (that haven't been visited yet) */
        foreach_fanin(node, i, fanin)
        {
            if (!arc_find_or_add(arc_table, fanin, node)) {
                reorder(fanin, node, DIR_IN, arc_table, visited);
            }
        }

        /* Traverse all out-going arcs (that haven't been visited yet) */
        foreach_fanout(node, gen, fanout)
        {
            if (!arc_find_or_add(arc_table, node, fanout)) {
                reorder(fanout, node, DIR_OUT, arc_table, visited);
            }
        }
    }
}

/* Create a hash table to identify the arcs which have been visited */
typedef struct arc_struct arc_t;
struct arc_struct {
    node_t *node1, *node2;
};


static int
arc_compare(arc1_p, arc2_p)
        char *arc1_p, *arc2_p;
{
    arc_t *arc1 = (arc_t *) arc1_p, *arc2 = (arc_t *) arc2_p;

    if (arc1->node1 == arc2->node1) {
        return (int) arc1->node2 - (int) arc2->node2;
    } else {
        return (int) arc1->node1 - (int) arc2->node1;
    }
}


static int
arc_hash(arc_p, modulus)
        char *arc_p;
        int modulus;
{
    arc_t *arc = (arc_t *) arc_p;
    return ((unsigned) arc->node1 + (unsigned) arc->node2) % modulus;
}


static st_table *
arc_hash_init() {
    return st_init_table(arc_compare, arc_hash);
}


static int
arc_find_or_add(table, node1, node2)
        st_table *table;
        node_t *node1, *node2;
{
    arc_t *arc;

    arc = ALLOC(arc_t, 1);
    arc->node1 = node1;
    arc->node2 = node2;
    if (st_find_or_add(table, (char *) arc, NIL(char **))) {
        FREE(arc);
        return 1;        /* already in table */
    } else {
        return 0;        /* not already in table */
    }
}


static void
arc_hash_end(table)
        st_table *table;
{
    st_generator *gen;
    arc_t        *arc;

    st_foreach_item(table, gen, (char **) &arc, NIL(
    char *)) {
        FREE(arc);
    }
    st_free_table(table);
}

static prim_t *
prim_alloc() {
    prim_t *prim;

    prim = ALLOC(prim_t, 1);
    prim->ninputs  = 0;
    prim->inputs   = NIL(prim_node_t * );
    prim->noutputs = 0;
    prim->outputs  = NIL(prim_node_t * );
    prim->nodes    = NIL(prim_node_t);
    prim->edges    = NIL(prim_edge_t);
    prim->gates    = lsCreate();
#ifdef SIS
    prim->latch_type = (int) UNKNOWN;
#endif /* SIS */
    return prim;
}


void
prim_free(prim)
        prim_t *prim;
{
    prim_edge_t *pe, *pe_next;
    prim_node_t *pn, *pn_next;

    for (pn = prim->nodes; pn != 0; pn = pn_next) {
        pn_next = pn->next;
        FREE(pn->name);
        FREE(pn);
    }

    for (pe = prim->edges; pe != 0; pe = pe_next) {
        pe_next = pe->next;
        FREE(pe);
    }

    FREE(prim->inputs);
    FREE(prim->outputs);
    LS_ASSERT(lsDestroy(prim->gates, (void (*)()) 0));
    FREE(prim);
}


void
prim_clear_binding(prim)
        prim_t *prim;
{
    register prim_node_t *pn;

    for (pn = prim->nodes; pn != NIL(prim_node_t); pn = pn->next) {
        pn->binding = NIL(node_t);
    }
}

static void
map_prim_dump_node(fp, pn)
        FILE *fp;
        prim_node_t *pn;
{
    (void) fprintf(fp,
                   "\tname=%-10s iso=%d nfanin=%2d nfanout=%2d type=%-3s binding=%08x\n",
                   pn->name == 0 ? "none" : pn->name,
                   pn->isomorphic_sons, pn->nfanin, pn->nfanout,
                   pn->type == PRIMARY_INPUT ? "PI" :
                   pn->type == PRIMARY_OUTPUT ? "PO" : "INT",
                   pn->binding);

#ifdef SIS
    (void) fprintf(fp, "\tlatch_output=%d\n", pn->latch_output);
#else
    (void) fprintf(fp, "\n");
#endif /* SIS */
}


static void
map_prim_dump_edge(fp, pe)
        FILE *fp;
        prim_edge_t *pe;
{
    (void) fprintf(fp, "    this_node=%08x connected=%08x dir=%s\n",
                   pe->this_node, pe->connected_node,
                   pe->dir == DIR_IN ? "IN" : "OUT");
    if (pe->this_node != 0) map_prim_dump_node(fp, pe->this_node);
    if (pe->connected_node != 0) map_prim_dump_node(fp, pe->connected_node);
}


void
map_prim_dump(fp, prim, detail)
        FILE *fp;
        prim_t *prim;
        int detail;
{
    lsGen       gen;
    lib_gate_t  *gate;
    int         i;
    prim_node_t *pn;
    prim_edge_t *pe;

    (void) fprintf(fp, "matches:");
    lsForeachItem(prim->gates, gen, gate)
    {
        (void) fprintf(fp, " %s", gate->name);
    }
#ifdef SIS
    /* sequential support */
    switch(prim->latch_type) {
    case (int) ACTIVE_HIGH: (void) fprintf(fp, " [type=ACTIVE_HIGH]\n"); break;
    case (int) ACTIVE_LOW: (void) fprintf(fp, " [type=ACTIVE_LOW]\n"); break;
    case (int) RISING_EDGE: (void) fprintf(fp, " [type=RISING_EDGE]\n"); break;
    case (int) FALLING_EDGE: (void) fprintf(fp, " [type=FALLING_EDGE]\n"); break;
    case (int) COMBINATIONAL: (void) fprintf(fp, " [type=COMBINATIONAL]\n"); break;
    case (int) ASYNCH: (void) fprintf(fp, " [type=ASYNCH]\n"); break;
    default: (void)  fprintf(fp, " [type=UNKNOWN]\n"); break;
    }
#else
    (void) fprintf(fp, "\n");
#endif /* SIS */

    if (detail) {
        (void) fprintf(fp, "ninputs=%d", prim->ninputs);
        for (i = 0; i < prim->ninputs; i++) {
            map_prim_dump_node(fp, prim->inputs[i]);
        }

        (void) fprintf(fp, "noutputs=%d\n", prim->noutputs);
        for (i = 0; i < prim->noutputs; i++) {
            map_prim_dump_node(fp, prim->outputs[i]);
        }

        (void) fprintf(fp, "nodes ...\n");
        for (pn = prim->nodes; pn != 0; pn = pn->next) {
            map_prim_dump_node(fp, pn);
        }
        (void) fprintf(fp, "edges ...\n");
        for (pe = prim->edges; pe != 0; pe = pe->next) {
            map_prim_dump_edge(fp, pe);
        }
    }
}
/*
 *  given a primitive, find the 'root' of the matching graph node
 */

node_t *
map_prim_get_root(prim)
        prim_t *prim;
{
    return prim->edges->this_node->binding;
}


/* check if a node only feeds a real PO */
static int
prim_feeds_only_real_po(node, po)
        node_t *node;
        node_t **po;
{
    lsGen  gen;
    node_t *fanout;

    *po = NIL(node_t);
    foreach_fanout(node, gen, fanout)
    {
        if (fanout->type == PRIMARY_OUTPUT) {
#ifdef SIS
            if (network_gate_type(node->network) != (int) COMBINATIONAL) {
          LS_ASSERT(lsFinish(gen));
          return 0;
            } else {
#endif /* SIS */
            /* we should have only one real po */
            assert(*po == NIL(node_t));
            *po = fanout;
#ifdef SIS
            }
#endif /* SIS */
        }
    }
    if (*po != NIL(node_t)) return 1;
    return 0;
}

static node_t *
prim_first_po(network)
        network_t *network;
{
    lsGen  gen;
    node_t *po;

    foreach_primary_output(network, gen, po)
    {
        LS_ASSERT(lsFinish(gen));
        break;
        /* NOTREACHED */
    }
    return po;
}

