
#ifdef SIS

#include "graph_int.h"
#include "graph_static_int.h"
#include "sis.h"

#define g_field(graph) ((g_field_t *)(graph)->user_data)

graph_t *g_alloc_static(int ng, int nv, int ne) {
    graph_t   *g;
    g_field_t *gf;;

    g  = g_alloc();
    gf = ALLOC(g_field_t, 1);
    gf->num_g_slots = ng;
    gf->num_v_slots = nv;
    gf->num_e_slots = ne;
    gf->user_data   = (gGeneric) ALLOC(gGeneric, ng);

    g->user_data = (gGeneric) gf;
    return (g);
}

void g_free_static(graph_t *g, void (*f_free_g)(), void (*f_free_v)(), void (*f_free_e)()) {
    vertex_t  *v;
    edge_t    *e;
    lsGen     gen;
    lsGeneric junk;

    if (g == NIL(graph_t)) {
        return;
    }
    if (f_free_g != (void (*)()) NULL) {
        (*f_free_g)(g_field(g)->user_data);
    }
    FREE(g_field(g)->user_data);
    FREE(g->user_data);

    foreach_vertex(g, gen, v) {
        if (f_free_v != (void (*)()) NULL) {
            (*f_free_v)(v->user_data);
        }
        FREE(v->user_data);
        (void) lsDestroy(g_get_in_edges(v), (void (*)()) NULL);
        (void) lsDestroy(g_get_out_edges(v), (void (*)()) NULL);
        (void) lsRemoveItem(((vertex_t_int *) v)->handle, &junk);
        FREE(v);
    }
    foreach_edge(g, gen, e) {
        if (f_free_e != (void (*)()) NULL) {
            (*f_free_e)(e->user_data);
        }
        FREE(e->user_data);
        (void) lsRemoveItem(((edge_t_int *) e)->handle, &junk);
        FREE(e);
    }
    g_free(g, (void (*)()) NULL, (void (*)()) NULL, (void (*)()) NULL);
}

static graph_t *theGraph;

static gGeneric copy_v_slots(gGeneric user_data) {
    int      i;
    int      num_v_slots = g_field(theGraph)->num_v_slots;
    gGeneric *new        = ALLOC(gGeneric, num_v_slots);

    for (i = 0; i < num_v_slots; i++) {
        new[i] = ((gGeneric *) user_data)[i];
    }
    return ((gGeneric) new);
}

static gGeneric copy_e_slots(gGeneric user_data) {
    int      i;
    int      num_e_slots = g_field(theGraph)->num_e_slots;
    gGeneric *new        = ALLOC(gGeneric, num_e_slots);

    for (i = 0; i < num_e_slots; i++) {
        new[i] = ((gGeneric *) user_data)[i];
    }
    return ((gGeneric) new);
}

graph_t *g_dup_static(graph_t *g, gGeneric (*f_copy_g)(), gGeneric (*f_copy_v)(), gGeneric (*f_copy_e)()) {
    g_field_t *gf, *gf2;
    graph_t   *g2;
    gGeneric  *new;
    int       i;

    if (f_copy_v == (gGeneric(*)()) NULL) {
        theGraph = g;
        f_copy_v = copy_v_slots;
    }
    if (f_copy_e == (gGeneric(*)()) NULL) {
        theGraph = g;
        f_copy_e = copy_e_slots;
    }
    g2 = g_dup(g, (gGeneric(*)()) NULL, f_copy_v, f_copy_e);
    if (g == NIL(graph_t)) {
        return (g2);
    }

    gf  = g_field(g);
    gf2 = ALLOC(g_field_t, 1);
    gf2->num_g_slots = gf->num_g_slots;
    gf2->num_v_slots = gf->num_v_slots;
    gf2->num_e_slots = gf->num_e_slots;
    if (f_copy_g == (gGeneric(*)()) NULL) {
        new    = ALLOC(gGeneric, gf->num_g_slots);
        for (i = gf->num_g_slots - 1; i >= 0; i--) {
            new[i] = ((gGeneric *) gf->user_data)[i];
        }
        gf2->user_data = (gGeneric) new;
    } else {
        gf2->user_data = (*f_copy_g)(gf->user_data);
    }
    g2->user_data    = (gGeneric) gf2;
    return (g2);
}

void g_set_g_slot_static(graph_t *g, int i, gGeneric val) {
    if (g == NIL(graph_t)) {
        fail("g_set_g_slot_static: Null graph");
    }
    ((gGeneric *) g_field(g)->user_data)[i] = val;
    return;
}

gGeneric g_get_g_slot_static(graph_t *g, int i) {
    if (g == NIL(graph_t)) {
        fail("g_get_g_slot_static: Null graph");
    }
    return ((gGeneric *) g_field(g)->user_data)[i];
}

void g_copy_g_slots_static(graph_t *g1, graph_t *g2, gGeneric (*f_copy_g)()) {
    g_field_t *gf1, *gf2;
    gGeneric  slots1, *slots2;
    int       n;

    if (g1 == NIL(graph_t) || g2 == NIL(graph_t)) {
        fail("g_copy_g_slots_static: Null graph");
    }
    gf1 = g_field(g1);
    gf2 = g_field(g2);
    n   = gf1->num_g_slots;

    if (n != gf2->num_g_slots) {
        fail("g_copy_g_slots_static: Graphs have different numbers of slots");
    }
    slots1 = gf1->user_data;
    slots2 = (gGeneric *) gf2->user_data;
    if (f_copy_g == (gGeneric(*)()) NULL) {
        for (n--; n >= 0; n--) {
            slots2[n] = ((gGeneric *) slots1)[n];
        }
    } else {
        FREE(slots2);
        gf2->user_data = (*f_copy_g)(slots1);
    }
}

edge_t *g_add_edge_static(vertex_t *v1, vertex_t *v2) {
    edge_t    *e;
    g_field_t *gf;

    if (v1 == NIL(vertex_t) || v2 == NIL(vertex_t)) {
        fail("g_add_edge_static: Null vertex");
    }
    e  = g_add_edge(v1, v2);
    gf = g_field(g_edge_graph(e));
    e->user_data = (gGeneric) ALLOC(gGeneric, gf->num_e_slots);
    return (e);
}

void g_delete_edge_static(edge_t *e, void (*f_free_e)()) {
    if (e == NIL(edge_t)) {
        fail("g_delete_edge_static: Null edge");
    }
    if (f_free_e != (void (*)()) NULL) {
        (*f_free_e)(e->user_data);
    }
    FREE(e->user_data);
    g_delete_edge(e, (void (*)()) NULL);
}

void g_set_e_slot_static(edge_t *e, int i, gGeneric val) {
    if (e == NIL(edge_t)) {
        fail("g_set_e_slot_static: Null edge");
    }
    ((gGeneric *) e->user_data)[i] = val;
}

gGeneric g_get_e_slot_static(edge_t *e, int i) {
    if (e == NIL(edge_t)) {
        fail("g_get_e_slot_static: Null edge");
    }
    return ((gGeneric *) e->user_data)[i];
}

void g_copy_e_slots_static(edge_t *e1, edge_t *e2, gGeneric (*f_copy_e)()) {
    int      n;
    gGeneric slots1, *slots2;

    if (e1 == NIL(edge_t) || e2 == NIL(edge_t)) {
        fail("g_copy_e_slots_static: Null edge");
    }
    n = g_field(g_edge_graph(e1))->num_e_slots;

    if (n != g_field(g_edge_graph(e2))->num_e_slots) {
        fail("g_copy_e_slots_static: Edges have differing numbers of slots");
    }
    slots1 = e1->user_data;
    slots2 = (gGeneric *) e2->user_data;
    if (f_copy_e == (gGeneric(*)()) NULL) {
        for (n--; n >= 0; n--) {
            slots2[n] = ((gGeneric *) slots1)[n];
        }
    } else {
        FREE(slots2);
        e2->user_data = (*f_copy_e)(slots1);
    }
}

vertex_t *g_add_vertex_static(graph_t *g) {
    g_field_t *gf;
    vertex_t  *v;

    if (g == NIL(graph_t)) {
        fail("g_add_vertex_static: Null graph");
    }
    gf = g_field(g);
    v  = g_add_vertex(g);
    v->user_data = (gGeneric) ALLOC(gGeneric, gf->num_v_slots);
    return (v);
}

void g_delete_vertex_static(vertex_t *v, void (*f_free_v)(), void (*f_free_e)()) {
    edge_t *e;
    lsGen  gen;

    if (v == NIL(vertex_t)) {
        fail("g_delete_vertex_static: Null vertex");
    }
    foreach_in_edge(v, gen, e) {
        if (f_free_e != (void (*)()) NULL) {
            (*f_free_e)(e->user_data);
        }
        FREE(e->user_data);
    }
    foreach_out_edge(v, gen, e) {
        if (f_free_e != (void (*)()) NULL) {
            (*f_free_e)(e->user_data);
        }
        FREE(e->user_data);
    }
    if (f_free_v != (void (*)()) NULL) {
        (*f_free_v)(v->user_data);
    }
    FREE(v->user_data);
    g_delete_vertex(v, (void (*)()) NULL, (void (*)()) NULL);
}

void g_set_v_slot_static(vertex_t *v, int i, gGeneric val) {
    if (v == NIL(vertex_t)) {
        fail("g_set_v_slot_static: Null vertex");
    }
    ((gGeneric *) v->user_data)[i] = val;
}

gGeneric g_get_v_slot_static(vertex_t *v, int i) {
    if (v == NIL(vertex_t)) {
        fail("g_get_v_slot_static: Null vertex");
    }
    return ((gGeneric *) v->user_data)[i];
}

void g_copy_v_slots_static(vertex_t *v1, vertex_t *v2, gGeneric (*f_copy_v)()) {
    int      n;
    gGeneric slots1, *slots2;

    if (v1 == NIL(vertex_t) || v2 == NIL(vertex_t)) {
        fail("g_copy_v_slots_static: Null vertex");
    }
    n = g_field(g_vertex_graph(v1))->num_v_slots;

    if (n != g_field(g_vertex_graph(v2))->num_v_slots) {
        fail("g_copy_v_slots_static: Vertices have differing numbers of slots");
    }
    slots1 = v1->user_data;
    slots2 = (gGeneric *) v2->user_data;
    if (f_copy_v == (gGeneric(*)()) NULL) {
        for (n--; n >= 0; n--) {
            slots2[n] = ((gGeneric *) slots1)[n];
        }
    } else {
        FREE(slots2);
        v2->user_data = (*f_copy_v)(slots1);
    }
}

#endif /* SIS */
