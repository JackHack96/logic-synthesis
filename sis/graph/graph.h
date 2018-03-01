/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/graph/graph.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:26 $
 *
 */
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

EXTERN graph_t *g_alloc ARGS((void));
EXTERN void g_free ARGS((graph_t *, void(*)(), void(*)(), void(*)()));
EXTERN void g_check ARGS((graph_t *));
EXTERN graph_t *g_dup ARGS((graph_t *, gGeneric(*)(), gGeneric(*)(), gGeneric(*)()));

EXTERN lsList g_get_vertices ARGS((graph_t *));

#define foreach_vertex(g, lgen, v)				\
	for (lgen = lsStart(g_get_vertices(g));			\
		lsNext(lgen, (lsGeneric *) &v, LS_NH) == LS_OK	\
		   || ((void) lsFinish(lgen), 0); )

#define foreach_edge(g, lgen, e)				\
	for (lgen = lsStart(g_get_edges(g));			\
		lsNext(lgen, (lsGeneric *) &e, LS_NH) == LS_OK	\
		   || ((void) lsFinish(lgen), 0); )

EXTERN vertex_t *g_add_vertex ARGS((graph_t *));
EXTERN void g_delete_vertex ARGS((vertex_t *, void (*)(), void (*)()));
EXTERN graph_t *g_vertex_graph ARGS((vertex_t *));

EXTERN lsList g_get_edges ARGS((graph_t *));
EXTERN lsList g_get_in_edges ARGS((vertex_t *));
EXTERN lsList g_get_out_edges ARGS((vertex_t *));

#define foreach_in_edge(v, lgen, e)				\
	for (lgen = lsStart(g_get_in_edges(v));			\
		lsNext(lgen, (lsGeneric *) &e, LS_NH) == LS_OK	\
		   || ((void) lsFinish(lgen), 0); )

#define foreach_out_edge(v, lgen, e)				\
	for (lgen = lsStart(g_get_out_edges(v));		\
		lsNext(lgen, (lsGeneric *) &e, LS_NH) == LS_OK	\
		   ||  ((void) lsFinish(lgen), 0); )

EXTERN edge_t *g_add_edge ARGS((vertex_t *, vertex_t *));
EXTERN void g_delete_edge ARGS((edge_t *, void (*)()));
EXTERN graph_t *g_edge_graph ARGS((edge_t *));
EXTERN vertex_t *g_e_source ARGS((edge_t *));
EXTERN vertex_t *g_e_dest ARGS((edge_t *));

EXTERN array_t *g_dfs ARGS((graph_t *));
EXTERN int g_is_acyclic ARGS((graph_t *));
EXTERN array_t *g_graph_sort ARGS((graph_t *, int (*)()));
