#ifndef GRAPH_H
#define GRAPH_H

typedef char *gGeneric;

typedef struct graph_struct {
    gGeneric user_data;
}            graph_t;

typedef struct vertex_struct {
    gGeneric user_data;
}            vertex_t;

typedef struct edge_struct {
    gGeneric user_data;
}            edge_t;

typedef void     (*GRAPH_PFV)();

typedef gGeneric (*GRAPH_PFG)();

extern graph_t *g_alloc(void);

extern void g_free(graph_t *, void(*)(), void(*)(), void(*)());

extern void g_check(graph_t *);

extern graph_t *g_dup(graph_t *, gGeneric(*)(), gGeneric(*)(), gGeneric(*)());

extern lsList g_get_vertices(graph_t *);

#define foreach_vertex(g, lgen, v)                \
    for (lgen = lsStart(g_get_vertices(g));            \
        lsNext(lgen, (lsGeneric *) &v, LS_NH) == LS_OK    \
           || ((void) lsFinish(lgen), 0); )

#define foreach_edge(g, lgen, e)                \
    for (lgen = lsStart(g_get_edges(g));            \
        lsNext(lgen, (lsGeneric *) &e, LS_NH) == LS_OK    \
           || ((void) lsFinish(lgen), 0); )

extern vertex_t *g_add_vertex(graph_t *);

extern void g_delete_vertex(vertex_t *, void (*)(), void (*)());

extern graph_t *g_vertex_graph(vertex_t *);

extern lsList g_get_edges(graph_t *);

extern lsList g_get_in_edges(vertex_t *);

extern lsList g_get_out_edges(vertex_t *);

#define foreach_in_edge(v, lgen, e)                \
    for (lgen = lsStart(g_get_in_edges(v));            \
        lsNext(lgen, (lsGeneric *) &e, LS_NH) == LS_OK    \
           || ((void) lsFinish(lgen), 0); )

#define foreach_out_edge(v, lgen, e)                \
    for (lgen = lsStart(g_get_out_edges(v));        \
        lsNext(lgen, (lsGeneric *) &e, LS_NH) == LS_OK    \
           ||  ((void) lsFinish(lgen), 0); )

extern edge_t *g_add_edge(vertex_t *, vertex_t *);

extern void g_delete_edge(edge_t *, void (*)());

extern graph_t *g_edge_graph(edge_t *);

extern vertex_t *g_e_source(edge_t *);

extern vertex_t *g_e_dest(edge_t *);

extern array_t *g_dfs(graph_t *);

extern int g_is_acyclic(graph_t *);

extern array_t *g_graph_sort(graph_t *, int (*)());

#endif