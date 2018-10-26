
#ifdef SIS

#include "sis.h"

int write_kiss(FILE *fp, graph_t *stg) {
    lsGen    gen, gen2;
    vertex_t *v;
    edge_t   *e;
    char     *vert_name;

    if (stg == (graph_t *) NULL) {
        (void) fprintf(miserr, "write_kiss: no stg specified\n");
        return (0);
    }

    (void) fprintf(fp, ".i %d\n.o %d\n.p %d\n.s %d\n", stg_get_num_inputs(stg),
                   stg_get_num_outputs(stg), stg_get_num_products(stg),
                   stg_get_num_states(stg));
    v = stg_get_start(stg);
    (void) fprintf(fp, ".r %s\n", stg_get_state_name(v));
    foreach_vertex(stg, gen, v) {
        vert_name = stg_get_state_name(v);
        foreach_out_edge(v, gen2, e) {
            (void) fprintf(fp, "%s %s %s %s\n", stg_edge_input_string(e), vert_name,
                           stg_get_state_name(stg_edge_to_state(e)),
                           stg_edge_output_string(e));
        }
    }
    return (1);
}

#endif /* SIS */
