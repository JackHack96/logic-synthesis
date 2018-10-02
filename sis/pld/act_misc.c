/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/pld/act_misc.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:55 $
 *
 */
/* changed July 29, 1991 - to make lint happy */

#include "pld_int.h"
#include "sis.h"

/*	hash table for partial results*/
st_table *misc_table;

/*extern ACT_VERTEX_PTR p_dagCopy();
extern ACT_VERTEX_PTR p_dagComplement();
extern void applyCreate();
extern void actDestroy();
extern void actCreate4Set();*/

void p_act_node_free(node_t *node) { p_actDestroy(&ACT_SET(node)->GLOBAL_ACT, 0); }

ACT_VERTEX_PTR p_rootCopy(ACT_VERTEX_PTR u) {
    ACT_VERTEX_PTR v, p_dagCopy();

    misc_table = st_init_table(st_ptrcmp, st_ptrhash);
    v          = p_dagCopy(u);
    st_free_table(misc_table);
    return (v);
}

ACT_VERTEX_PTR p_dagCopy(ACT_VERTEX_PTR u) {
    char           *dummy;
    ACT_VERTEX_PTR v;

    if (st_lookup(misc_table, (char *) u, &dummy)) {
        return ((ACT_VERTEX_PTR) dummy);
    }
    v = ALLOC(ACT_VERTEX, 1);
    v->id         = u->id;
    v->value      = u->value;
    v->index      = u->index;
    v->index_size = u->index_size;
    v->mark       = u->mark;
    if (u->value == NO_VALUE) {
        v->low  = p_dagCopy(u->low);
        v->high = p_dagCopy(u->high);
    } else {
        v->low  = NIL(act_t);
        v->high = NIL(act_t);
    }

    (void) st_insert(misc_table, (char *) u, (char *) v);
    return (v);
}

ACT_VERTEX_PTR p_rootComplement(ACT_VERTEX_PTR u) {
    ACT_VERTEX_PTR v, p_dagComplement();

    misc_table = st_init_table(st_ptrcmp, st_ptrhash);
    v          = p_dagComplement(u);
    st_free_table(misc_table);
    return (v);
}

ACT_VERTEX_PTR p_dagComplement(ACT_VERTEX_PTR u) {
    char           *dummy;
    ACT_VERTEX_PTR v;

    if (st_lookup(misc_table, (char *) u, &dummy)) {
        return ((ACT_VERTEX_PTR) dummy);
    }
    v = ALLOC(ACT_VERTEX, 1);
    v->id         = u->id;
    switch (u->value) {
        case NO_VALUE:v->value = NO_VALUE;
            break;
        case 1:v->value = 0;
            break;
        case 0:v->value = 1;
            break;
    }
    v->index      = u->index;
    v->index_size = u->index_size;
    v->mark       = u->mark;
    if (u->value == NO_VALUE) {
        v->low  = p_dagComplement(u->low);
        v->high = p_dagComplement(u->high);
    } else {
        v->low  = NIL(act_t);
        v->high = NIL(act_t);
    }
    (void) st_insert(misc_table, (char *) u, (char *) v);
    return (v);
}

ACT_VERTEX_PTR p_act_construct(node_t *node, array_t *order_list, int locality) {
    array_t *node_vec;

    if (!locality) {
        p_applyCreate(node, order_list);
        return (ACT_SET(node)->GLOBAL_ACT->act->root);
    } else {
        node_vec = array_alloc(node_t *, 0);
        array_insert_last(node_t *, node_vec, node);
        /* changed July 29, 1991 - to make lint happy */
        /* (void)p_actCreate4Set(node_vec, order_list, locality,
                            FANIN); */
        (void) p_actCreate4Set(node_vec, order_list, locality, FANIN, (float) 0.0,
                               NIL(network_t), NIL(array_t), NIL(st_table));
        array_free(node_vec);
        return (ACT_SET(node)->LOCAL_ACT->act->root);
    }
}

int p_act_size(ACT_VERTEX_PTR act) {
    switch (act->id) {
        case 0:return act->value;
        default:return (act->id + 1);
    }
}
