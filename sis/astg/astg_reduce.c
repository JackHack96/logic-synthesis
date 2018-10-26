
/* -------------------------------------------------------------------- *\
   reduce.c -- generate net reductions

   Uses a brute force approach to enumerate all MG allocations.
   Set of transitions with indegree > 1 are the key.
   Product of all indegrees is upper bound on # SM components
   But there can be duplicates if this is used as an exact number.

   Need to establish link from stg to smc and vice versa.  And
   also to find a better way to answer the 1-token SM restriction.

   References:
   [CHU87] p. 50-51 Hack's reduction algorithms
           p. 95 persistency/u.s.c. for each MG component

   Usage of vertex fields:
       flag0: 0=in MG allocation, 1=not
       flag1: 1=deleted during MG reduction, 0=not
       flag2: 1=covered in some allocation, 0=not
\* ---------------------------------------------------------------------*/

#ifdef SIS

#include "astg_core.h"
#include "astg_int.h"

#define covered flag2
#define dead flag1
#define not_in_allocation flag0

int astg_print_component(int i, array_t *varray, FILE *stream) {
    astg_vertex *v;
    int         n;

    fprintf(stream, "   %d)", i);
    for (n = 0; n < array_n(varray); n++) {
        v = array_fetch(astg_vertex *, varray, n);
        fprintf(stream, " %s", astg_v_name(v));
    }
    fputs("\n", stream);
}

static array_t *get_key_vertices(astg_graph *stg, int type, int *ubound) {
    astg_generator gen;
    array_t        *key_vertices;
    astg_vertex    *v;
    astg_edge      *e;
    int            degree;

    key_vertices = array_alloc(astg_vertex *, 0);
    *ubound = 1;

    astg_foreach_vertex(stg, gen, v) {
        if (v->vtype != type)
            continue;
        degree = (type == ASTG_TRANS) ? astg_in_degree(v) : astg_out_degree(v);
        if (degree < 2)
            continue;
        *ubound *= degree;
        array_insert_last(astg_vertex *, key_vertices, v);
        if (type == ASTG_TRANS) {
            astg_foreach_in_edge(v, gen, e) {
                astg_tail(e)->not_in_allocation = ASTG_TRUE;
            }
        } else {
            astg_foreach_out_edge(v, gen, e) {
                astg_head(e)->not_in_allocation = ASTG_TRUE;
            }
        }
    }

    dbg(1, printf("bound of %d components\n", *ubound));
    return key_vertices;
}

static int n_undead_fanin(astg_vertex *v) {
    astg_generator gen;
    int            rc = 0;
    astg_edge      *e;

    astg_foreach_in_edge(v, gen, e) if (astg_tail(e)->dead == ASTG_FALSE) rc++;
    return rc;
}

static int n_undead_fanout(astg_vertex *v) {
    astg_generator gen;
    int            rc = 0;
    astg_edge      *e;

    astg_foreach_out_edge(v, gen, e) if (astg_head(e)->dead == ASTG_FALSE) rc++;
    return rc;
}

static void del_mg_trans(astg_graph *stg, astg_vertex *trans, int depth) {
    astg_edge      *e1, *e2;
    astg_vertex    *place;
    astg_generator gen1, gen2;

    if (trans == NULL || trans->dead)
        return;
    if (++depth > 40) fail("infinite loop");
    trans->dead = ASTG_TRUE;

    astg_foreach_out_edge(trans, gen1, e1) {
        place = astg_head(e1);
        if (place->dead)
            continue;
        if (n_undead_fanin(place) == 0) {
            astg_foreach_out_edge(place, gen2, e2) {
                del_mg_trans(stg, astg_head(e2), depth);
            }
            place->dead = ASTG_TRUE;
        }
    }
}

static void del_sm_place(astg_graph *stg, astg_vertex *place, int depth) {
    astg_generator gen1, gen2;
    astg_edge      *e1, *e2;
    astg_vertex    *trans;

    if (place == NULL || place->dead)
        return;
    if (++depth > 40) fail("infinite loop");
    place->dead = ASTG_TRUE;

    astg_foreach_in_edge(place, gen1, e1) {
        trans = astg_tail(e1);
        if (trans->dead)
            continue;
        if (n_undead_fanout(trans) == 0) {
            astg_foreach_in_edge(trans, gen2, e2) {
                del_sm_place(stg, astg_tail(e2), depth);
            }
            trans->dead = ASTG_TRUE;
        }
    }
}

static int cmp_vertex(char *obj1, char *obj2) {
    astg_vertex *v1 = *((astg_vertex **) obj1);
    astg_vertex *v2 = *((astg_vertex **) obj2);

    return (v2->weight1.i - v1->weight1.i);
}

static int cmp_component(array_t *comp1, array_t *comp2) {
    /* These are already in canonical form. */
    /* Only need equal/not equal test, no ordering. */
    int n = array_n(comp1);

    if (n != array_n(comp2))
        return -1;

    while (n--) {
        if (array_fetch(astg_vertex *, comp1, n) !=
            array_fetch(astg_vertex *, comp2, n))
            return 1;
    }

    /* These are equal. */
    return 0;
}

static int g_type;
static int n_tried;

static int add_component(array_t *cc, int n, void *fdata) {
    /* callback for astg_connected_comp. */
    array_t   *component_array = (array_t *) fdata;
    array_t   *component;
    astg_bool unique           = ASTG_TRUE;
    int       i;

    /* Copy the component and make canonical. */
    component = array_dup(cc);
    array_sort(component, cmp_vertex);

    if ((++n_tried % 25000) == 0)
        dbg(1, printf("checking component %d/%d\n", n_tried,
                      array_n(component_array)));

    for (i = array_n(component_array); i--;) {
        if (!cmp_component(component, array_fetch(array_t *, component_array, i))) {
            array_free(component);
            unique = ASTG_FALSE;
            break;
        }
    }

    if (unique) {
        dbg(3, astg_print_component(array_n(component_array), component, stdout));
        array_insert_last(array_t *, component_array, component);
    }
}

static void do_sm_reduce(astg_graph *stg, array_t *component_array) {
    astg_vertex    *v;
    astg_generator gen1, gen2;
    astg_vertex    *trans, *place;
    astg_edge      *e;

    astg_foreach_vertex(stg, gen1, v) v->dead = ASTG_FALSE;

    astg_foreach_vertex(stg, gen1, trans) {
        if (trans->dead)
            continue;
        astg_foreach_in_edge(trans, gen2, e) {
            place = astg_tail(e);
            if (place->dead)
                continue;
            if (place->not_in_allocation) {
                del_sm_place(stg, place, 0);
            }
        }
    }

    astg_foreach_vertex(stg, gen1, v) v->unprocessed = !v->dead;
    g_type = ASTG_PLACE;
    astg_connected_comp(stg, add_component, (void *) component_array, ASTG_SUBSET);
}

static void do_mg_reduce(astg_graph *stg, array_t *component_array) {
    astg_vertex    *v;
    astg_generator gen1, gen2;
    astg_vertex    *place, *trans;
    astg_edge      *e;

    astg_foreach_vertex(stg, gen1, v) v->dead = ASTG_FALSE;

    astg_foreach_vertex(stg, gen1, place) {
        if (place->dead)
            continue;
        astg_foreach_out_edge(place, gen2, e) {
            trans = astg_head(e);
            if (trans->dead)
                continue;
            if (trans->not_in_allocation) {
                del_mg_trans(stg, trans, 0);
            }
        }
    }

    astg_foreach_vertex(stg, gen1, v) v->unprocessed = !v->dead;
    g_type = ASTG_TRANS;
    astg_connected_comp(stg, add_component, (void *) component_array, ASTG_SUBSET);
}

static void gen_sm_allocation(astg_graph *stg, array_t *component_array, array_t *t, int n) {
    astg_vertex    *trans, *place;
    astg_generator gen;
    astg_edge      *e;

    if (n-- == 0) {
        do_sm_reduce(stg, component_array);
    } else {
        trans = array_fetch(astg_vertex *, t, n);
        assert(trans->vtype == ASTG_TRANS);
        astg_foreach_in_edge(trans, gen, e) {
            place = astg_tail(e);
            place->not_in_allocation = ASTG_FALSE;
            gen_sm_allocation(stg, component_array, t, n);
            place->not_in_allocation = ASTG_TRUE;
        }
    }
}

static void gen_mg_allocation(astg_graph *stg, array_t *component_array, array_t *t, int n) {
    astg_vertex    *place, *trans;
    astg_generator gen;
    astg_edge      *e;

    if (n-- == 0) {
        do_mg_reduce(stg, component_array);
    } else {
        place = array_fetch(astg_vertex *, t, n);
        assert(place->vtype == ASTG_PLACE);
        astg_foreach_out_edge(place, gen, e) {
            trans = astg_head(e);
            trans->not_in_allocation = ASTG_FALSE;
            gen_mg_allocation(stg, component_array, t, n);
            trans->not_in_allocation = ASTG_TRUE;
        }
    }
}

static void find_reductions(astg_graph *stg, int type) {
    array_t        *b;
    astg_vertex    *v;
    int            ubound;
    int            n  = 0;
    astg_graph     *g = stg;
    astg_generator gen;
    array_t        *component_array;

    n_tried = 0;
    /* Unique int ID used for sorting in add_component. */
    astg_foreach_vertex(g, gen, v) {
        v->weight1.i         = n++;
        v->dead              = ASTG_FALSE;
        v->not_in_allocation = ASTG_FALSE;
    }

    dbg(3,
        printf("generating %s components\n", type == ASTG_PLACE ? "MG" : "SM"));
    component_array = array_alloc(array_t *, 0);
    b               = get_key_vertices(g, type, &ubound);
    if (type == ASTG_TRANS) {
        gen_sm_allocation(g, component_array, b, array_n(b));
        stg->sm_comp = component_array;
    } else {
        gen_mg_allocation(g, component_array, b, array_n(b));
        stg->mg_comp = component_array;
    }
    array_free(b);

    dbg(2,
        printf("%d %s components (%d maximum) in %s\n", array_n(component_array),
               type == ASTG_TRANS ? "SM" : "MG", ubound, astg_name(stg)));
}

static astg_retval check_reductions(astg_graph *stg, int type, astg_bool verbose) {
    astg_retval    status      = ASTG_OK;
    int            i, j, n_bad = 0;
    astg_vertex    *v;
    astg_graph     *g          = stg;
    astg_generator gen;
    array_t        *component_array;
    array_t        *component;

    component_array = (type == ASTG_TRANS) ? stg->sm_comp : stg->mg_comp;

    astg_foreach_vertex(g, gen, v) { v->covered = ASTG_FALSE; }

    for (i = 0; i < array_n(component_array); i++) {
        component = array_fetch(array_t *, component_array, i);
        /* Make sure the component is strongly connected. */
        astg_foreach_vertex(g, gen, v) v->subset = ASTG_FALSE;
        /* Mark all vertices covered by some component. */
        for (j = 0; j < array_n(component); j++) {
            v = array_fetch(astg_vertex *, component, j);
            v->covered = ASTG_TRUE;
            v->subset  = ASTG_TRUE;
        }
        if (astg_debug_flag >= 4) {
            /* Print all components for debugging. */
            astg_print_component(i, array_fetch(array_t *, component_array, i),
                                 stdout);
        }
        if (astg_strong_comp(g, ASTG_NO_CALLBACK, ASTG_NO_DATA, ASTG_SUBSET) != 1) {
            if (verbose && status == ASTG_OK) {
                printf("warning: %s component(s)",
                       type == ASTG_TRANS ? "state machine" : "marked graph");
            }
            if (verbose)
                printf(" %d", i + 1);
            status = ASTG_NOT_COVER;
            n_bad++;
        }
    }

    if (verbose && n_bad != 0) {
        printf(" %s not strongly connected.\n", (n_bad == 1) ? "is" : "are");
    }

    /* Check if the components cover the net. */
    astg_sel_new(stg, "uncovered vertices", ASTG_FALSE);
    astg_foreach_vertex(g, gen, v) {
        if (!v->covered) {
            status = ASTG_NOT_COVER;
            astg_sel_vertex(v, ASTG_TRUE);
            dbg(1, msg("warning: %s not covered by reductions\n", astg_v_name(v)));
        }
    }

    return status;
}

astg_retval get_sm_comp(astg_graph *stg, astg_bool verbose) {
    long stg_count = astg_change_count(stg);

    if (stg_count != stg->smc_count) {
        find_reductions(stg, ASTG_TRANS);
        stg->smc_count = stg_count;
    }
    return check_reductions(stg, ASTG_TRANS, verbose);
}

astg_retval get_mg_comp(astg_graph *stg, astg_bool verbose) {
    long stg_count = astg_change_count(stg);

    if (stg_count != stg->mgc_count) {
        find_reductions(stg, ASTG_PLACE);
        stg->mgc_count = stg_count;
    }
    return check_reductions(stg, ASTG_PLACE, verbose);
}

void free_components(array_t *component_array)
{
    int i;

    if (component_array != NULL) {
        for (i = 0; i < array_n(component_array); i++) {
            array_free(array_fetch(array_t *, component_array, i));
        }
        array_free(component_array);
    }
}
#endif /* SIS */
