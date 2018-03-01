/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/graph/graph_dfs.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:26 $
 *
 */
#ifdef SIS
#include "sis.h"
#include "graph_int.h"

static vertex_t *
find_an_end_vertex(v,visit_list)
vertex_t *v;
st_table *visit_list;
{
    edge_t *e;
    vertex_t *dest;
    lsGen gen;

    if (lsLength(g_get_out_edges(v)) == 0) {
        return(v);
    }
    foreach_out_edge (v,gen,e) {
	(void) lsFinish(gen);
	dest = g_e_dest(e);
        if (st_insert(visit_list,(char *) dest,(char *) 0) == 1) {
	    return(NIL(vertex_t));
	}
	return(find_an_end_vertex(dest,visit_list));
        /* NOTREACHED */
    }
    /* no free out_edges */
    return(NIL(vertex_t));
}

static int
dfs_recurr(v, dfs_list, dfs_array) 
vertex_t *v;
st_table *dfs_list;
array_t *dfs_array;
{
    edge_t *e;
    lsGen gen;
    int val;

    if (st_lookup_int(dfs_list,(char *) v, &val)) {
        return(val == 0);
    }
    (void) st_insert(dfs_list,(char *) v,(char *) 1);

    foreach_in_edge (v,gen,e) {
        if (!dfs_recurr(g_e_source(e),dfs_list,dfs_array)) {
	    return(0);
	}
    }
    (void) st_insert(dfs_list,(char *) v,(char *) 0);
    array_insert_last(vertex_t *,dfs_array,v);
    return(1);
}

static array_t *
g_dfs_int(g)
graph_t *g;
{
    vertex_t *v;
    lsGen gen;
    array_t *dfs_array;
    st_table *visit_list,*dfs_list;
    int cycle = FALSE;

    dfs_array = array_alloc(vertex_t *,0);
    visit_list = st_init_table(st_ptrcmp,st_ptrhash);
    dfs_list = st_init_table(st_ptrcmp,st_ptrhash);

    foreach_vertex (g,gen,v) {
        if (!st_is_member(dfs_list,(char *) v)) {
	    (void) st_insert(visit_list,(char *) v,(char *) 0);
	    v = find_an_end_vertex(v,visit_list);
	    if (v == NIL(vertex_t) || !dfs_recurr(v,dfs_list,dfs_array)) {
	        cycle = TRUE;
		(void) lsFinish(gen);
		break;
	    }
	}
    }

    st_free_table(visit_list);
    st_free_table(dfs_list);
    if (cycle == TRUE) {
        array_free(dfs_array);
        return(NIL(array_t));
    }
    return(dfs_array);
}

array_t *
g_dfs(g)
graph_t *g;
{
    array_t *x;

    x = g_dfs_int(g);
    if (x == NIL(array_t)) {
        fail("g_dfs: Graph has cycle");
    }
    return(x);
}

int
g_is_acyclic(g)
graph_t *g;
{
    array_t *x;

    x = g_dfs_int(g);
    if (x) {
        array_free(x);
	return(TRUE);
    }
    return(FALSE);
}
#endif /* SIS */

