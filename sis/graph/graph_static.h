
/******************************* graph_static.h ************************/

extern graph_t *g_alloc_static(int, int, int);

extern void g_free_static(graph_t *, void (*)(), void (*)(), void (*)());

extern graph_t *g_dup_static(graph_t *, char *(*)(), char *(*)(), char *(*)());

extern void g_set_g_slot_static(graph_t *, int, char *);

extern char *g_get_g_slot_static(graph_t *, int);

extern void g_copy_g_slots_static(graph_t *, graph_t *, char *(*)());

extern vertex_t *g_add_vertex_static(graph_t *);

extern void g_delete_vertex_static(vertex_t *, void (*)(), void (*)());

extern void g_set_v_slot_static(vertex_t *, int, char *);

extern char *g_get_v_slot_static(vertex_t *, int);

extern void g_copy_v_slots_static(vertex_t *, vertex_t *, char *(*)());

extern edge_t *g_add_edge_static(vertex_t *, vertex_t *);

extern void g_delete_edge_static(edge_t *, void (*)());

extern void g_set_e_slot_static(edge_t *, int, char *);

extern char *g_get_e_slot_static(edge_t *, int);

extern void g_copy_e_slots_static(edge_t *, edge_t *, char *(*)());
