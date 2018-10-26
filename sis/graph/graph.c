
#ifdef SIS
#include "sis.h"
#include "graph_int.h"

int g_unique_id;

graph_t *
g_alloc()
{
    graph_t_int *graph = ALLOC(graph_t_int,1);
    
    graph->user_data = (gGeneric) NULL;
    graph->v_list = lsCreate();
    graph->e_list = lsCreate();

    return((graph_t *) graph);
}

void
g_free(g,f_free_g,f_free_v,f_free_e)
graph_t *g;
void (*f_free_g)();
void (*f_free_v)();
void (*f_free_e)();
{
    lsGen gen;
    vertex_t *v;
    edge_t *e;

    if (g == NIL(graph_t)) {
        return;
    }
    if (f_free_g != (void (*)()) NULL) {
        (*f_free_g)(g->user_data);
    }
    foreach_vertex (g,gen,v) {
        if (f_free_v != (void (*)()) NULL) {
	    (*f_free_v)(v->user_data);
	}
	(void) lsDestroy(g_get_in_edges(v),(void (*)()) NULL);
	(void) lsDestroy(g_get_out_edges(v),(void (*)()) NULL);
	FREE(v);
    }
    foreach_edge (g,gen,e) {
        if (f_free_e != (void (*)()) NULL) {
	    (*f_free_e)(e->user_data);
	}
	FREE(e);
    }
    (void) lsDestroy(g_get_vertices(g),(void (*)()) NULL);
    (void) lsDestroy(g_get_edges(g),(void (*)()) NULL);
    FREE(g);
}

void
g_check(g)
graph_t *g;
{
    vertex_t *v, *source, *dest;
    edge_t *e, *test;
    lsGen gen, gen2;
    int found;

    if (g == NIL(graph_t)) {
        return;
    }
    foreach_edge (g,gen,e) {
        source = g_e_source(e);
	dest = g_e_dest(e);
        if (source == NIL(vertex_t)) {
	    fail("g_check: Edge has no source");
	}
	if (dest == NIL(vertex_t)) {
	    fail("g_check: Edge has no destination");
	}
	if (g_vertex_graph(source) != g_vertex_graph(dest)) {
	    fail("g_check: Edge connects different graphs");
	}
	found = FALSE;
	foreach_out_edge (source,gen2,test) {
	    if (test == e) {
	        found = TRUE;
		(void) lsFinish(gen2);
		break;
	    }
	}
	if (found == FALSE) {
	    fail("g_check: Vertex does not point back to edge");
	}
	found = FALSE;
	foreach_in_edge (dest,gen2,test) {
	    if (test == e) {
	        found = TRUE;
		(void) lsFinish(gen2);
		break;
	    }
	}
	if (found == FALSE) {
	    fail("g_check: Vertex does not point back to edge");
	}
    }
    foreach_vertex (g,gen,v) {
        if (g_vertex_graph(v) != g) {
	    fail("g_check: Vertex not a member of graph");
	}
        if (lsLength(g_get_in_edges(v)) + lsLength(g_get_out_edges(v)) == 0) {
	    (void) fprintf(miserr,"Warning: g_check: Unconnected vertex\n");
	    continue;
	}
	foreach_in_edge(v,gen2,test) {
	    if (g_e_dest(test) != v) {
	        fail("g_check: Edge does not point back to vertex");
	    }
	}
	foreach_out_edge(v,gen2,test) {
	    if (g_e_source(test) != v) {
	        fail("g_check: Edge does not point back to vertex");
	    }
	}
    }
}

graph_t *
g_dup(g,f_copy_g,f_copy_v,f_copy_e)
graph_t *g;
gGeneric (*f_copy_g)();
gGeneric (*f_copy_v)();
gGeneric (*f_copy_e)();
{
    graph_t *new;
    vertex_t *v, *new_v, *from, *to;
    edge_t *e, *new_e;
    st_table *ptrtable = st_init_table(st_ptrcmp,st_ptrhash);
    lsGen gen;

    new = g_alloc();
    if (g == NIL(graph_t)) {
        return(new);
    }

    if (f_copy_g == (gGeneric (*)()) NULL) {
        new->user_data = g->user_data;
    }
    else {
        new->user_data = (*f_copy_g)(g->user_data);
    }
    foreach_vertex (g,gen,v) {
        new_v = g_add_vertex(new);
	if (f_copy_v == (gGeneric (*)()) NULL) {
	    new_v->user_data = v->user_data;
	}
	else {
	    new_v->user_data = (*f_copy_v)(v->user_data);
	}
	(void) st_insert(ptrtable,(char *) v,(char *) new_v);
    }
    foreach_edge (g,gen,e) {
        (void) st_lookup(ptrtable,(char *) g_e_source(e),(char **) &from);
	(void) st_lookup(ptrtable,(char *) g_e_dest(e),(char **) &to);
	new_e = g_add_edge(from,to);
	if (f_copy_e == (gGeneric (*)()) NULL) {
	    new_e->user_data = e->user_data;
	}
	else {
	    new_e->user_data = (*f_copy_e)(e->user_data);
	}
    }
    st_free_table(ptrtable);
    return(new);
}

array_t *
g_graph_sort(g,cmp)
graph_t *g;
int (*cmp)();
{
    int i;
    lsGen gen;
    vertex_t *v;
    array_t *v_array;

    i = 0;
    v_array = array_alloc(vertex_t *,0);

    foreach_vertex (g,gen,v) {
        array_insert(vertex_t *,v_array,i++,v);
    }
    array_sort(v_array,cmp);
    return(v_array);
}

lsList
g_get_edges(g)
graph_t *g;
{
    if (g == NIL(graph_t)) {
        fail("g_get_edges: Null graph");
    }
    return(((graph_t_int *) g)->e_list);
}

lsList
g_get_in_edges(v)
vertex_t *v;
{
    if (v == NIL(vertex_t)) {
        fail("g_get_in_edges: Null vertex");
    }
    return(((vertex_t_int *) v)->in_list);
}

lsList
g_get_out_edges(v)
vertex_t *v;
{
    if (v == NIL(vertex_t)) {
        fail("g_get_out_edges: Null vertex");
    }
    return(((vertex_t_int *) v)->out_list);
}

edge_t *
g_add_edge(v1,v2)
vertex_t *v1, *v2;
{
    edge_t_int *edge;
    lsHandle handle;
    graph_t *g;

    if (v1 == NIL(vertex_t) || v2 == NIL(vertex_t)) {
        fail("g_add_edge: Null vertex");
    }
    g = g_vertex_graph(v1);
    if (g != g_vertex_graph(v2)) {
        fail("g_add_edge: Edge connects different graphs");
    }
    edge = ALLOC(edge_t_int,1);
    (void) lsNewEnd(g_get_edges(g),(lsGeneric) edge,&handle);
    edge->user_data = (gGeneric) NULL;
    edge->from = (vertex_t_int *) v1;
    edge->to = (vertex_t_int *) v2;
    edge->id = g_unique_id++;
    edge->handle = handle;
    (void) lsNewEnd(g_get_out_edges(v1),(lsGeneric) edge,LS_NH);
    (void) lsNewEnd(g_get_in_edges(v2),(lsGeneric) edge,LS_NH);

    return((edge_t *) edge);
}

static void
g_del_from_list(list,item)
lsList list;
lsGeneric item;
{
    lsGen gen;
    lsGeneric looking,dummy;
    lsHandle handle;

    gen = lsStart(list);
    while (lsNext(gen,&looking,&handle) != LS_NOMORE) {
        if (item == looking) {
	    if (lsRemoveItem(handle,&dummy) != LS_OK) {
	        fail("g_del_from_list: Can't remove edge");
	    }
	    break;
	}
    }
    (void) lsFinish(gen);
}

void
g_delete_edge(e,f_free_e)
edge_t *e;
void (*f_free_e)();
{
    lsGeneric junk;

    if (e == NIL(edge_t)) {
        fail("g_delete_edge: Null edge");
    }
    g_del_from_list(g_get_out_edges(g_e_source(e)),(lsGeneric) e);
    g_del_from_list(g_get_in_edges(g_e_dest(e)),(lsGeneric) e);

    (void) lsRemoveItem(((edge_t_int *) e)->handle,&junk);
    if (f_free_e != (void (*)()) NULL) {
        (*f_free_e)(e->user_data);
    }
    FREE(e);
}

graph_t *
g_edge_graph(e)
edge_t *e;
{
    if (e == NIL(edge_t)) {
        fail("g_edge_graph: Null edge");
    }
    return((graph_t *) (((edge_t_int *) e)->to->g));
}

vertex_t *
g_e_source(e)
edge_t *e;
{
    if (e == NIL(edge_t)) {
        fail("g_e_source: Null edge");
    }
    return((vertex_t *) (((edge_t_int *) e)->from));
}

vertex_t *
g_e_dest(e)
edge_t *e;
{
    if (e == NIL(edge_t)) {
        fail("g_e_dest: Null edge");
    }
    return((vertex_t *) (((edge_t_int *) e)->to));
}


lsList
g_get_vertices(g)
graph_t *g;
{
    if (g == NIL(graph_t)) {
        fail("g_get_vertices: Null graph");
    }
    return(((graph_t_int *) g)->v_list);
}

vertex_t *
g_add_vertex(g)
graph_t *g;
{
    lsHandle handle;
    vertex_t_int *vert;

    if (g == NIL(graph_t)) {
        fail("g_add_vertex: Null graph");
    }
    vert = ALLOC(vertex_t_int,1);
    if (lsNewEnd(g_get_vertices(g),(lsGeneric) vert,&handle) != LS_OK) {
        fail("g_add_vertex: Can't add vertex");
    }    
    vert->user_data = (gGeneric) NULL;
    vert->g = (graph_t_int *) g;
    vert->in_list = lsCreate();
    vert->out_list = lsCreate();
    vert->id = g_unique_id++;
    vert->handle = handle;
    return((vertex_t *) vert);
}

void
g_delete_vertex(v,f_free_v,f_free_e)
vertex_t *v;
void (*f_free_v)();
void (*f_free_e)();
{
    edge_t *e;
    lsGeneric junk;
    lsGen gen;

    if (v == NIL(vertex_t)) {
        fail("g_delete_vertex: Null vertex");
    }
    if (f_free_v != (void (*)()) NULL) {
        (*f_free_v)(v->user_data);
    }
    foreach_in_edge (v,gen,e) {
        g_del_from_list(g_get_out_edges(g_e_source(e)),(lsGeneric) e);
	if (f_free_e != (void (*)()) NULL) {
	    (*f_free_e)(e->user_data);
	}
	if (lsRemoveItem(((edge_t_int *) e)->handle,&junk) != LS_OK) {
	    fail("g_delete_vertex: Can't remove edge from graph");
	}
	FREE(e);
    }
    foreach_out_edge (v,gen,e) {
        g_del_from_list(g_get_in_edges(g_e_dest(e)),(lsGeneric) e);
	if (f_free_e != (void (*)()) NULL) {
	    (*f_free_e)(e->user_data);
	}
	if (lsRemoveItem(((edge_t_int *) e)->handle,&junk) != LS_OK) {
	    fail("g_delete_vertex: Can't remove edge from graph");
	}
	FREE(e);
    }
    (void) lsDestroy(g_get_out_edges(v),(void (*)()) NULL);
    (void) lsDestroy(g_get_in_edges(v),(void (*)()) NULL);
    (void) lsRemoveItem(((vertex_t_int *) v)->handle,&junk);
    FREE(v);
}

graph_t *
g_vertex_graph(v)
vertex_t *v;
{
    if (v == NIL(vertex_t)) {
        fail("g_vertex_graph: Null vertex");
    }
    return((graph_t *) ((vertex_t_int *) v)->g);
}
#endif /* SIS */

