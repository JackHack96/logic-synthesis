/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/graph/graph_static_int.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:27 $
 *
 */
/******************************** graph_static_int.h ********************/

typedef struct g_field_struct {
    int num_g_slots;
    int num_v_slots;
    int num_e_slots;
    gGeneric user_data;
} g_field_t;
