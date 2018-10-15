
#include "sis.h"

pset_family node_sf_adjust();

PLA_t *network_to_pla(network_t *network_1) {
    PLA_t         *PLA;
    register int  i;
    register pset last, px, pdest;
    pset_family   x;
    node_t        *p, *node, *fanin, **new_fanin;
    network_t     *network;
    int           nin, nout, out, hack;
    lsGen         gen;
    network_t     *dcnetwork;
    node_t        *dcnode;
    st_table      *node_exdc_table;
    st_table      *node_table;
    char          *dummy;

    if (network_1 == 0)
        return 0;

    nin  = network_num_pi(network_1);
    nout = network_num_po(network_1);
    if (nin == 0 || nout == 0) {
        return 0;
    }

    network = network_dup(network_1);
    (void) network_collapse(network);

    /* nin and nout should be reset.  The reason is that collapse may have */
    /* deleted latches that fanout nowhere, which changes the number of pi's */
    /* and po's.  It is not expect that the number of inputs and outputs will */
    /* go to zero!! */

    nin  = network_num_pi(network);
    nout = network_num_po(network);

    dcnetwork = network_dc_network(network);
    if (dcnetwork != NIL(network_t)) {
        network_collapse(dcnetwork);
        node_table      = st_init_table(st_ptrcmp, st_ptrhash);
        node_exdc_table = attach_dcnetwork_to_network(network);
        foreach_primary_output(network, gen, node) {
            dcnode = find_ex_dc(node, node_exdc_table);
            (void) st_insert(node_table, (char *) node, (char *) dcnode);
        }
        st_free_table(node_exdc_table);
    }

    /* Form a list of the primary inputs */
    new_fanin = ALLOC(node_t *, nin);
    i         = 0;
    foreach_primary_input(network, gen, p) { new_fanin[i++] = p; }

    undefine_cube_size();
    cube.num_binary_vars = nin;
    cube.num_vars        = nin + 1;
    cube.part_size       = ALLOC(int, cube.num_vars);
    cube.part_size[cube.num_vars - 1] = nout;
    cube_setup();

    PLA = new_PLA();
    PLA->label = ALLOC(char *, cube.size);
    for (i = 0; i < cube.num_binary_vars; i++) {
        node = new_fanin[i];
        PLA->label[cube.first_part[i] + 1] = util_strsav(node->name);
        /* map_dcset() requires names on all variables */
        PLA->label[cube.first_part[i]]     = ALLOC(char, strlen(node->name) + 6);
        (void) sprintf(PLA->label[cube.first_part[i]], "%s.bar", node->name);
    }

    out = cube.first_part[cube.output];
    foreach_primary_output(network, gen, p) {
        PLA->label[out++] = util_strsav(p->name);
    }

    PLA->F = new_cover(100);
    if (dcnetwork != NIL(network_t)) {
        PLA->D = new_cover(100);
    }
    out = cube.first_part[cube.output];
    foreach_primary_output(network, gen, node) {

        /* special hack for output == input */
        fanin = node->fanin[0];
        hack  = 0;
        if (fanin->type == PRIMARY_INPUT) {
            fanin = node_literal(fanin, 1);
            hack  = 1;
        }

        x = node_sf_adjust(fanin, new_fanin, nin);
        foreach_set(x, last, px) {
            pdest  = new_cube();
            for (i = 0; i < nin * 2; i++) {
                if (is_in_set(px, i)) {
                    set_insert(pdest, i);
                }
            }
            set_insert(pdest, out);
            PLA->F = sf_addset(PLA->F, pdest);
            set_free(pdest);
        }

        /* special hack for output == input */
        if (hack) {
            node_free(fanin);
        }
        sf_free(x);
        if (dcnetwork != NIL(network_t)) {
            (void) st_lookup(node_table, (char *) node, &dummy);
            dcnode = (node_t *) dummy;
            if (dcnode->type == PRIMARY_INPUT) {
                dcnode = node_literal(dcnode, 1);
            }
            x      = node_sf_adjust(dcnode, new_fanin, nin);
            foreach_set(x, last, px) {
                pdest  = new_cube();
                for (i = 0; i < nin * 2; i++) {
                    if (is_in_set(px, i)) {
                        set_insert(pdest, i);
                    }
                }
                set_insert(pdest, out);
                PLA->D = sf_addset(PLA->D, pdest);
                set_free(pdest);
            }

            /* special hack for output == input */
            node_free(dcnode);
            sf_free(x);
        }
        out++;
    }
    if (dcnetwork != NIL(network_t)) {
        st_free_table(node_table);
    }

    FREE(new_fanin);
    network_free(network);
    return PLA;
}

void discard_pla(pPLA PLA) {
    if (PLA != 0)
        free_PLA(PLA);

    setdown_cube();
    FREE(cube.part_size);
    define_cube_size(20);
}
