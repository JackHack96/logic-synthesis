#ifndef GRAPH_STATIC_H
#define GRAPH_STATIC_H

/******************************* graph_static.h ************************/

#include "graph.h"

graph_t *g_alloc_static(int, int, int);

void g_free_static(graph_t *, void (*)(), void (*)(), void (*)());

graph_t *g_dup_static(graph_t *, char *(*)(), char *(*)(), char *(*)());

void g_set_g_slot_static(graph_t *, int, char *);

char *g_get_g_slot_static(graph_t *, int);

void g_copy_g_slots_static(graph_t *, graph_t *, char *(*)());

vertex_t *g_add_vertex_static(graph_t *);

void g_delete_vertex_static(vertex_t *, void (*)(), void (*)());

void g_set_v_slot_static(vertex_t *, int, char *);

char *g_get_v_slot_static(vertex_t *, int);

void g_copy_v_slots_static(vertex_t *, vertex_t *, char *(*)());

edge_t *g_add_edge_static(vertex_t *, vertex_t *);

void g_delete_edge_static(edge_t *, void (*)());

void g_set_e_slot_static(edge_t *, int, char *);

char *g_get_e_slot_static(edge_t *, int);

void g_copy_e_slots_static(edge_t *, edge_t *, char *(*)());

#endif