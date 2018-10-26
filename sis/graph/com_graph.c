
#ifdef SIS

#include "sis.h"

static char *month[] = {"Jan", "Feb", "March", "april", "may", "June",
                        "july", "Aug", "Sept", "Oct", "nov", "Dec"};

#define voidNULL (void (*)()) NULL
#define gGenericNULL (gGeneric(*)()) NULL

static void strfree(gGeneric thing) { FREE(thing); }

static int graph_test() {
    graph_t  *g1, *g2;
    int      i, j;
    vertex_t *v[10];
    lsGen    gen, gen2;
    vertex_t *vert;
    edge_t   *edge;

    g1 = g_alloc();

    for (i = 0; i < 10; i++) {
        v[i] = g_add_vertex(g1);
        v[i]->user_data = (gGeneric) i;
    }
    for (i = 0; i < 9; i++) {
        for (j = i + 1; j < 10; j++) {
            (void) g_add_edge(v[i], v[j]);
        }
    }
    (void) g_add_edge(v[5], v[5]); /* self loop */

    (void) lsFirstItem(g_get_out_edges(v[4]), (lsGeneric *) &edge, LS_NH);
    g_delete_edge(edge, voidNULL);

    g_delete_vertex(v[8], voidNULL, voidNULL);

    g_add_vertex(g1)->user_data = (gGeneric) 10; /* unconnected vertex */

    g2 = g_dup(g1, gGenericNULL, gGenericNULL, gGenericNULL);
    foreach_vertex(g2, gen, vert) {
        (void) fprintf(misout, "\nCopy of %d\ngoes to:    ", vert->user_data);
        foreach_out_edge(vert, gen2, edge) {
            (void) fprintf(misout, "%d ", g_e_dest(edge)->user_data);
        }
        (void) fprintf(misout, "\ncomes from: ");
        foreach_in_edge(vert, gen2, edge) {
            (void) fprintf(misout, "%d ", g_e_source(edge)->user_data);
        }
    }
    (void) fputc('\n', misout);
    g_check(g1);
    g_check(g2);
    g_free(g1, voidNULL, voidNULL, voidNULL);
    g_free(g2, voidNULL, voidNULL, voidNULL);

    g1     = g_alloc();
    for (i = 0; i < 12; i++) {
        g_add_vertex(g1)->user_data = (gGeneric) month[i];
    }
    g2 = g_dup(g1, gGenericNULL, (gGeneric(*)()) util_strsav, gGenericNULL);
    foreach_vertex(g1, gen, vert) { ((char *) vert->user_data)[0] = '\0'; }
    foreach_vertex(g2, gen, vert) { /* strings copied by strsav */
        (void) fprintf(misout, "%s\n", (char *) vert->user_data);
    }
    g_free(g1, voidNULL, voidNULL, voidNULL); /* don't free static strings */
    g_free(g2, voidNULL, strfree, voidNULL);  /* free copies */
    return (0);
}

static void edge_free(gGeneric thing) { FREE(((gGeneric *) thing)[2]); }

static gGeneric edge_copy(gGeneric thing) {
    gGeneric *new = ALLOC(gGeneric, 4);
    gGeneric *old = (gGeneric *) thing;

    new[0] = old[0];
    new[1] = old[1];
    new[2] = (gGeneric) util_strsav((char *) old[2]);
    new[3] = old[3];
    return ((gGeneric) new);
}

static int graph_static_test() {
    graph_t  *g1, *g2;
    int      i, j, x;
    vertex_t *v[10], *v1, *v2;
    edge_t   *e, *edge;
    lsGen    gen;

    g1 = g_alloc_static(3, 2, 4);

    for (i = 0; i < 10; i++) {
        v[i] = g_add_vertex_static(g1);
        g_set_v_slot_static(v[i], 0, (gGeneric) i);
        g_set_v_slot_static(v[i], 1, (gGeneric) (2 * i));
    }
    x      = 0;
    for (i = 0; i < 9; i++) {
        for (j = i + 1; j < 10; j++) {
            e = g_add_edge_static(v[i], v[j]);
            g_set_e_slot_static(e, 2, (gGeneric) util_strsav(month[i]));
            g_set_e_slot_static(e, 1, (gGeneric) x++);
        }
    }
    g_delete_vertex_static(v[3], voidNULL, edge_free); /* kill v[3] */
    (void) lsLastItem(g_get_out_edges(v[6]), (lsGeneric *) &edge, LS_NH);
    g_delete_edge_static(edge, edge_free); /* kill last edge of v[6] */

    g_set_g_slot_static(g1, 1, (gGeneric) 'f');
    g2 = g_dup_static(g1, gGenericNULL, gGenericNULL, edge_copy);

    v1 = g_add_vertex_static(g2);
    g_copy_v_slots_static(v[2], v1, gGenericNULL);

    foreach_edge(g2, gen, edge) {
        v1 = g_e_source(edge);
        v2 = g_e_dest(edge);
        (void) fprintf(misout, "%d (%s) connects %d & %d\n",
                       g_get_e_slot_static(edge, 1), g_get_e_slot_static(edge, 2),
                       g_get_v_slot_static(v1, 0), g_get_v_slot_static(v2, 0));
    }
    g_free_static(g1, voidNULL, voidNULL, edge_free);
    g_free_static(g2, voidNULL, voidNULL, edge_free);
    return (0);
}

static int reverso(char *a, char *b) { return ((int) ((vertex_t *) b)->user_data - (int) ((vertex_t *) a)->user_data); }

static int graph_dfs_test() {
    int      i;
    vertex_t *v[10];
    array_t  *arr;
    graph_t  *g;
    vertex_t *x;

    g      = g_alloc();
    for (i = 0; i < 10; i++) {
        v[i] = g_add_vertex(g);
        v[i]->user_data = (gGeneric) i;
    }
    (void) g_add_edge(v[3], v[4]);
    (void) g_add_edge(v[0], v[3]);
    (void) g_add_edge(v[0], v[6]);
    (void) g_add_edge(v[0], v[2]);
    (void) g_add_edge(v[1], v[3]);
    (void) g_add_edge(v[6], v[3]);
    (void) g_add_edge(v[2], v[5]);
    (void) g_add_edge(v[2], v[3]);
    (void) g_add_edge(v[3], v[5]);
    (void) g_add_edge(v[6], v[2]);

    (void) g_add_edge(v[7], v[8]);
    (void) g_add_edge(v[9], v[7]);
    (void) g_add_edge(v[9], v[8]);
    arr = g_dfs(g);
    (void) fprintf(misout, "Depth first sort\n");
    for (i = 0; i < 10; i++) {
        x = array_fetch(vertex_t *, arr, i);
        (void) fprintf(misout, "%d\n", x->user_data);
    }
    array_free(arr);
    (void) fprintf(misout, "\nReverse sort\n");
    arr    = g_graph_sort(g, reverso);
    for (i = 0; i < 10; i++) {
        x = array_fetch(vertex_t *, arr, i);
        (void) fprintf(misout, "%d\n", x->user_data);
    }
    array_free(arr);
    g_free(g, voidNULL, voidNULL, voidNULL);
    return (0);
}

init_graph() {
    int g_unique_id;

    g_unique_id = 0;
    com_add_command("_graph_test", graph_test, 0);
    com_add_command("_graph_static_test", graph_static_test, 0);
    com_add_command("_graph_dfs_test", graph_dfs_test, 0);
}

end_graph() {}

/*

       ______     1
      /      \	 /
     /	      v v
     |	0 ---> 3 ----> 4	9 --> 8
     |	| \    ^\		|    ^
     |	|  \   | \		|   /
     |	|   \  |  \		|  /
      \	|    \ |   \		| /
       \v     v|    v		v/
          6 ---> 2 --> 5		7

  This is the graph represented in test_graph_dfs
*/
#endif /* SIS */
