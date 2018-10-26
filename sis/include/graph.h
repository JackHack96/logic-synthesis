#ifndef GRAPH_H
#define GRAPH_H

#include "list.h"
#include "array.h"

typedef char *gGeneric;

typedef struct graph_struct {
  gGeneric user_data;
} graph_t;

typedef struct vertex_struct {
  gGeneric user_data;
} vertex_t;

typedef struct edge_struct {
  gGeneric user_data;
} edge_t;

typedef void (*GRAPH_PFV)();

typedef gGeneric (*GRAPH_PFG)();

graph_t *g_alloc(void);

void g_free(graph_t *, void (*)(), void (*)(), void (*)());

void g_check(graph_t *);

graph_t *g_dup(graph_t *, gGeneric (*)(), gGeneric (*)(),
                      gGeneric (*)());

lsList g_get_vertices(graph_t *);

#define foreach_vertex(g, lgen, v)                                             \
  for (lgen = lsStart(g_get_vertices(g));                                      \
       lsNext(lgen, (lsGeneric *)&v, LS_NH) == LS_OK ||                        \
       ((void)lsFinish(lgen), 0);)

#define foreach_edge(g, lgen, e)                                               \
  for (lgen = lsStart(g_get_edges(g));                                         \
       lsNext(lgen, (lsGeneric *)&e, LS_NH) == LS_OK ||                        \
       ((void)lsFinish(lgen), 0);)

vertex_t *g_add_vertex(graph_t *);

void g_delete_vertex(vertex_t *, void (*)(), void (*)());

graph_t *g_vertex_graph(vertex_t *);

lsList g_get_edges(graph_t *);

lsList g_get_in_edges(vertex_t *);

lsList g_get_out_edges(vertex_t *);

#define foreach_in_edge(v, lgen, e)                                            \
  for (lgen = lsStart(g_get_in_edges(v));                                      \
       lsNext(lgen, (lsGeneric *)&e, LS_NH) == LS_OK ||                        \
       ((void)lsFinish(lgen), 0);)

#define foreach_out_edge(v, lgen, e)                                           \
  for (lgen = lsStart(g_get_out_edges(v));                                     \
       lsNext(lgen, (lsGeneric *)&e, LS_NH) == LS_OK ||                        \
       ((void)lsFinish(lgen), 0);)

edge_t *g_add_edge(vertex_t *, vertex_t *);

void g_delete_edge(edge_t *, void (*)());

graph_t *g_edge_graph(edge_t *);

vertex_t *g_e_source(edge_t *);

vertex_t *g_e_dest(edge_t *);

array_t *g_dfs(graph_t *);

int g_is_acyclic(graph_t *);

array_t *g_graph_sort(graph_t *, int (*)());

int init_graph();

int end_graph();

#endif