
#ifndef MAXFLOW_H
#define MAXFLOW_H

#define CUTSET 1

/* Global variable */
extern int maxflow_debug;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*
 * Define the data structure 
 */
typedef struct MF_NODE   mf_node_t;
typedef struct MF_EDGE   mf_edge_t;
typedef struct MF_GRAPH  mf_graph_t;
typedef struct MF_CUTSET mf_cutset_t;
typedef struct MF_NODE   *mfnptr;
typedef struct MF_EDGE   *mfeptr;
typedef struct MF_GRAPH  *mfgptr;
typedef struct MF_CUTSET *mfcptr;

struct MF_NODE {
    char   *name;           /* asciz name of node */
    mfeptr *in_edge;      /* edges incident to this node */
    mfeptr *out_edge;     /* edges incident from this node */
    int    increment_flow;   /* the increment flow */
    mfnptr path_node;     /* augmenting path node */
    mfeptr path_edge;     /* augmenting path edge */
    mfnptr pnext;         /* general usage link */
    short  nin;            /* number of in_edges */
    short  nout;           /* number of out_edges */
    short  direction;      /* direction of the link: 1 = from; -1 = to */
    short  flag;           /* flag word */
    short  max_nout;       /* total no. of available out_edge */
    short  nfict;          /* no. of fictitious nodes using same name */
    char   *fname;          /* store the fictitious node name pointer */
};

struct MF_EDGE {
    mfnptr inode;         /* edge incident from this node */
    mfnptr onode;         /* edge incident to this node */
    int    capacity;         /* capacity of the edge */
    int    flow;             /* flow of the edge */
    int    flag;             /* flag word */
};

struct MF_GRAPH {
    mfnptr   source_node;         /* source node */
    mfnptr   sink_node;           /* sink node */
    mfnptr   *nlist;              /* node list */
    st_table *node_table;    /* hash table of nodes */
    mfnptr   first_label_element; /* first element in the scan list */
    mfnptr   last_label_element;  /* last element in the scan list */
    int      num_of_node;            /* total no. of nodes in the circuit */
    int      max_num_of_nptr;        /* total no. of available node pointers */
};

struct MF_CUTSET {
    mfgptr graph;
    char   **from_node;
    char   **to_node;
    int    *capacity;
    int    narcs;
};

/*
 * Declare the exported routines 
 */

extern int mf_sizeof_cutset(mf_graph_t *);

extern int mf_remove_node(mf_graph_t *, char *);

extern int mf_reread_edge(mf_graph_t *, char *, char *, int);

extern void maxflow(mf_graph_t *, int);

extern void mf_read_node(mf_graph_t *, char *, int);

extern void mf_read_edge(mf_graph_t *, char *, char *, int);

extern void mf_free_cutset(mf_cutset_t *);

extern void mf_free_graph(mf_graph_t *);

extern void mf_display_graph(FILE *, mf_graph_t *);

extern void mf_display_flow(FILE *, mf_graph_t *);

extern void mf_display_cutset(FILE *, mf_graph_t *);

extern array_t *cutset(network_t *, st_table *);

extern array_t *cutset_interface(network_t *, st_table *, int);

extern array_t *mf_build_node_cutset(mf_graph_t *, st_table *);

extern mf_node_t *mf_get_node(mf_graph_t *, char *);

extern mf_graph_t *mf_alloc_graph(void);

extern mf_graph_t *mf_create_flow_network(network_t *, st_table *, int, st_table **);

extern mf_cutset_t *mf_get_cutset(mf_graph_t *, array_t **, array_t **, array_t **);

/*
 * Access macros 
 */
#define mf_foreach_node(graph, i, p)                    \
    for(i = 0;                                \
    i < (graph)->num_of_node && (p = (graph)->nlist[i]);        \
        i++)

#define mf_foreach_fanout(node, i, e)                    \
    for(i = 0;                                \
    i < (node)->nout && (e = (node)->out_edge[i]);            \
        i++)

#define mf_foreach_fanin(node, i, e)                    \
    for(i = 0;                                \
    i < (node)->nin && (e = (node)->in_edge[i]);            \
        i++)

/*
 * Query macros 
 */

#define mf_node_name(node)                        \
    ((node)->name)

#define mf_num_nodes(graph)                        \
    ((graph)->num_of_node)

#define mf_num_fanin(node)                        \
    ((node)->in_edge == NIL(mf_edge_t *) ? 0 : (node)->nin)

#define mf_num_fanout(node)                        \
    ((node)->out_edge == NIL(mf_edge_t *) ? 0 : (node)->nout)

#define mf_get_sink_node(graph)                        \
    (graph)->sink_node

#define mf_get_source_node(graph)                    \
    (graph)->source_node

#define mf_get_edge_flow(edge)                        \
    (edge)->flow

#define mf_get_edge_capacity(edge)                    \
    (edge)->capacity

#define mf_modify_edge_capacity(edge, capacity)                \
    ((edge)->capacity = capacity)

#define mf_tail_of_edge(edge)                        \
    ((edge)->inode)

#define mf_head_of_edge(edge)                        \
    ((edge)->onode)

#define mf_is_edge_on_mincut(edge)                    \
    ((edge)->flag & CUTSET)

#endif
