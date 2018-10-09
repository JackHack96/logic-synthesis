#ifndef GRAPH_STATIC_INT_H
#define GRAPH_STATIC_INT_H
/******************************** graph_static_int.h ********************/

#include "graph.h"

typedef struct g_field_struct {
    int      num_g_slots;
    int      num_v_slots;
    int      num_e_slots;
    gGeneric user_data;
} g_field_t;
#endif