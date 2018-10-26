
#ifdef SIS
/* Routines to transform a Signal Transition Graph ("astg", for asynchronous
 * STG) into a State Transition Graph ("stg"), and various auxiliary things.
 */

#include "astg_core.h"
#include "astg_int.h"
#include "bwd_int.h"
#include "sis.h"

#define TNAME(t) ((t) == NIL(astg_trans) ? "" : astg_trans_name(t))
#define TRANS_TYPE(c) (((c) == '0') ? ASTG_POS_X : ASTG_NEG_X)

struct _scr_state {
    char     *name, *in, *out;
    vertex_t *state;
};

typedef struct _scr_state scr_state_t;

/* needed here because we must delete states in stg_patch */
static st_table *state_cache;
static graph_t  *state_cached_stg;

astg_scode bwd_astg_state_bit(astg_signal *sig_p) { return astg_state_bit(sig_p); }

/* un-minimized STG */
graph_t  *global_orig_stg;
st_table *global_orig_stg_names;

/* associates a state with a marking and vice-versa */
st_table *global_state_marking;
st_table *global_marking_state;
st_table *global_edge_trans;

/* same as stg_create_transition, but does not check for duplicate edges
 */
edge_t *stg_create_nfa_transition(vertex_t *from, vertex_t *to, char *in, char *out) {
    edge_t  *edge;
    graph_t *stg;

    stg = g_vertex_graph(from);
    if (stg != g_vertex_graph(to)) {
        fail("Vertices to and from belong to different stgs");
    }
    if (strlen(in) != (int) g_get_g_slot_static(stg, NUM_INPUTS)) {
        fail("In stg_create_nfa_transition: Invalid number of inputs specified");
    }
    if (strlen(out) != (int) g_get_g_slot_static(stg, NUM_OUTPUTS)) {
        fail("In stg_create_nfa_transition: Invalid number of outputs specified");
    }
    edge = g_add_edge_static(from, to);
    g_set_e_slot_static(edge, INPUT_STRING, (gGeneric) util_strsav(in));
    g_set_e_slot_static(edge, OUTPUT_STRING, (gGeneric) util_strsav(out));
    g_set_g_slot_static(
            stg, NUM_PRODUCTS,
            (gGeneric) ((int) g_get_g_slot_static(stg, NUM_PRODUCTS) + 1));
    return (edge);
}

int bwd_log2(astg_scode arg) {
    astg_scode i;
    int        value;

    for (i = 1, value = 0; i < arg; i = i << 1, value++);
    return value;
}

/* auxiliary debugging function */
void bwd_pedge(edge_t *edge) {
    fprintf(sisout, "%s %s %s %s\n", stg_edge_input_string(edge),
            stg_get_state_name(stg_edge_from_state(edge)),
            stg_get_state_name(stg_edge_to_state(edge)),
            stg_edge_output_string(edge));
}

/* auxiliary debugging function */
void bwd_pstate(vertex_t *state) {
    lsGen  gen;
    edge_t *edge;

    foreach_state_inedge(state, gen, edge) { bwd_pedge(edge); }
    foreach_state_outedge(state, gen, edge) { bwd_pedge(edge); }
}

vertex_t *bwd_get_state_by_name(graph_t *stg, char *name) {
    vertex_t *state;
    lsGen    gen;

    if (state_cached_stg != stg) {
        if (state_cache != NIL(st_table)) {
            st_free_table(state_cache);
            state_cache = NIL(st_table);
        }
    }
    if (state_cache == NIL(st_table)) {
        state_cache      = st_init_table(strcmp, st_strhash);
        state_cached_stg = stg;

        stg_foreach_state(stg, gen, state) {
            st_insert(state_cache, stg_get_state_name(state), (char *) state);
        }
    }

    if (!st_lookup(state_cache, name, (char **) &state)) {
/* better slow than dead... */
        state = stg_get_state_by_name(stg, name);
        if (state != NIL(vertex_t)) {
            st_insert(state_cache, stg_get_state_name(state), (char *) state);
        }
    }

    return state;
}

static void print_sg(astg_graph *astg, graph_t *stg, st_table *edge_trans) {
    vertex_t   *state, *to;
    edge_t     *edge;
    astg_trans *trans;
    array_t    *trans_arr;
    int        i;
    lsGen      gen, gen1;

    stg_foreach_state(stg, gen, state) {
        foreach_state_outedge(state, gen1, edge) {
            to = stg_edge_to_state(edge);
            if (to == state) {
                continue;
            }
            assert(st_lookup(edge_trans, (char *) edge, (char **) &trans_arr));
            fprintf(sisout, "%s", stg_get_state_name(state));
            for (i = 0; i < array_n(trans_arr); i++) {
                trans = array_fetch(astg_trans *, trans_arr, i);
                fprintf(sisout, " %s", astg_trans_name(trans));
            }
            fprintf(sisout, " %s\n", stg_get_state_name(to));
        }
    }
}

/* return the set of signals enabled in "to_state" not enabled in "from_state"
 */
astg_scode bwd_new_enabled_signals(edge_t *edge, st_table *edge_trans, st_table *state_marking) {
    astg_scode      from_enabled, new_enabled, firing;
    astg_marking    *marking;
    static st_table *new_enabled_cache;
    static graph_t  *new_enabled_cached_stg;
    astg_trans      *trans;
    int             i;
    array_t         *marking_arr, *trans_arr;

    if (new_enabled_cached_stg != g_edge_graph(edge)) {
        if (new_enabled_cache != NIL(st_table)) {
            st_free_table(new_enabled_cache);
            new_enabled_cache = NIL(st_table);
        }
    }
    if (new_enabled_cache == NIL(st_table)) {
        new_enabled_cache      = st_init_table_with_params(
                st_ptrcmp, st_ptrhash, ST_DEFAULT_INIT_TABLE_SIZE, /* density */ 1,
                ST_DEFAULT_GROW_FACTOR, ST_DEFAULT_REORDER_FLAG);
        new_enabled_cached_stg = g_edge_graph(edge);
    }

    if (st_lookup(new_enabled_cache, (char *) edge, (char **) &new_enabled)) {
        return new_enabled;
    }

    assert(st_lookup(state_marking, (char *) stg_edge_from_state(edge),
                     (char **) &marking_arr));
    from_enabled = 0;
    for (i       = 0; i < array_n(marking_arr); i++) {
        marking = array_fetch(astg_marking *, marking_arr, i);
        from_enabled |= astg_marking_enabled(marking);
    }
    assert(st_lookup(state_marking, (char *) stg_edge_to_state(edge),
                     (char **) &marking_arr));
    new_enabled = 0;
    for (i      = 0; i < array_n(marking_arr); i++) {
        marking = array_fetch(astg_marking *, marking_arr, i);
        new_enabled |= astg_marking_enabled(marking);
    }
    assert(st_lookup(edge_trans, (char *) edge, (char **) &trans_arr));
    firing = 0;
    for (i = 0; i < array_n(trans_arr); i++) {
        trans = array_fetch(astg_trans *, trans_arr, i);
        firing |= bwd_astg_state_bit(astg_trans_sig(trans));
    }

    /* check if the firing signal is enabled again, if so do not rm it */
    if (firing & new_enabled) {
        from_enabled &= ~firing;
    }
    new_enabled &= ~from_enabled;

    st_insert(new_enabled_cache, (char *) edge, (char *) new_enabled);
    return new_enabled;
}

/* transform a marking into a (unique) STG state name */
char *bwd_marking_string(astg_ba_rec *marked_places) {
    int         i;
    static char buf1[9], buf2[513];

    buf2[0] = '\0';
    assert(marked_places->n_word);
    for (i = 0; i < marked_places->n_word && i < (sizeof(buf2) / sizeof(buf1));
         i++) {
        sprintf(buf1, "_%x", marked_places->bit_array[i]);
        strcat(buf2, buf1);
    }
    return buf2;
}

/* check if an edge in,from -> to,out exists already in the STG */
int duplicate(graph_t *stg, vertex_t *from, vertex_t *to, char *in, char *out) {
    lsGen  gen;
    edge_t *edge, *dup_edge;
    int    result, base, len, i;
    char   *old_out;

    result   = 0;
    dup_edge = NIL(edge_t);
    foreach_state_outedge(from, gen, edge) {
        if (!strcmp(in, stg_edge_input_string(edge))) {
            result = 1;
            if (to != stg_edge_to_state(edge) ||
                strcmp(out, stg_edge_output_string(edge))) {
                dup_edge = edge;
            }
        }
    }

    if (dup_edge == NIL(edge_t)) {
        /* no duplication or same next state and output */
        return result;
    }

    /* check if we are just moving along a dummy edge to a new self loop */
    if (to == from &&
        stg_edge_to_state(dup_edge) == stg_edge_from_state(dup_edge)) {
        /* change the output to a "maximal" set of enabled transitions */
        len     = strlen(out);
        base    = strlen(in) - len;
        old_out = stg_edge_output_string(dup_edge);
        for (i  = 0; i < len; i++) {
            if (old_out[i] != out[i] && in[base + i] != out[i]) {
                old_out[i] = out[i];
            }
        }
        if (astg_debug_flag > 2) {
            fprintf(sisout, "updating %s %s %s %s\n", in, stg_get_state_name(from),
                    stg_get_state_name(to), old_out);
        }
        return 1;
    }
    return 1;
}

/* patch the stg replacing the state called "old" with "new"
 */
static void
stg_patch(graph_t *stg, st_table *state_out, st_table *edge_trans, st_table *marking_state, st_table *state_marking,
          vertex_t *old, vertex_t *new, array_t *from_stack, int from_sp) {
    astg_marking *marking;
    array_t      *marking_arr_old, *marking_arr_new;
    char         *out, *in, *name;
    lsGen        gen;
    edge_t       *edge, *new_edge;
    vertex_t     *state, **from_p;
    int          i;
    array_t      *trans_arr;

    /* patch the "from" of the callers, if appropriate */
    for (i = 0; i < from_sp; i++) {
        from_p = array_fetch(vertex_t **, from_stack, i);
        if (*from_p == old) {
            *from_p = new;
            if (astg_debug_flag > 2) {
                fprintf(sisout, "stack patch %s -> %s at level %d\n",
                        stg_get_state_name(*from_p), stg_get_state_name(new), i);
            }
        }
    }

    /* delete from state_marking */
    assert(st_delete(state_marking, (char **) &old, (char **) &marking_arr_old));
    assert(st_lookup(state_marking, (char *) new, (char **) &marking_arr_new));
    for (i = 0; i < array_n(marking_arr_old); i++) {
        marking = array_fetch(astg_marking *, marking_arr_old, i);
        array_insert_last(astg_marking *, marking_arr_new, marking);
        st_insert(marking_state, (char *) marking, (char *) new);
    }
    array_free(marking_arr_old);
    /* delete from state_out */
    assert(st_delete(state_out, (char **) &old, &out));

    /* now patch the stg (ugh...) */
    foreach_state_inedge(old, gen, edge) {
        state = stg_edge_from_state(edge);
        in    = stg_edge_input_string(edge);
        out   = stg_edge_output_string(edge);
        if (state == old) {
            if (duplicate(stg, new, new, in, out)) {
                if (astg_debug_flag > 0) {
                    fprintf(sisout,
                            "skipping replacement %s (%s -> %s) -> (%s -> %s) %s\n", in,
                            stg_get_state_name(old), stg_get_state_name(new),
                            stg_get_state_name(state), stg_get_state_name(new), out);
                }
                st_delete(edge_trans, (char **) &edge, (char **) &trans_arr);
                continue;
            }
            if (astg_debug_flag > 2) {
                fprintf(sisout, "replacing %s (%s -> %s) -> (%s -> %s) %s\n", in,
                        stg_get_state_name(state), stg_get_state_name(new),
                        stg_get_state_name(old), stg_get_state_name(new), out);
            }

            new_edge = stg_create_transition(new, new, in, out);
        } else {
            /* no duplication checking, because we are CERTAINLY duplicated */
            if (astg_debug_flag > 2) {
                fprintf(sisout, "replacing %s %s -> (%s -> %s) %s\n", in,
                        stg_get_state_name(state), stg_get_state_name(old),
                        stg_get_state_name(new), out);
            }

            new_edge = stg_create_nfa_transition(state, new, in, out);
        }
        if (st_delete(edge_trans, (char **) &edge, (char **) &trans_arr)) {
            st_insert(edge_trans, (char *) new_edge, (char *) trans_arr);
        }
    }
    foreach_state_outedge(old, gen, edge) {
        state = stg_edge_to_state(edge);
        if (state == old) {
            continue;
        }
        in  = stg_edge_input_string(edge);
        out = stg_edge_output_string(edge);
        if (duplicate(stg, new, state, in, out)) {
            if (astg_debug_flag > 0) {
                fprintf(sisout, "skipping replacement %s (%s -> %s) -> %s %s\n", in,
                        stg_get_state_name(old), stg_get_state_name(new),
                        stg_get_state_name(state), out);
            }
            st_delete(edge_trans, (char **) &edge, (char **) &trans_arr);
            continue;
        }
        if (astg_debug_flag > 2) {
            fprintf(sisout, "replacing %s (%s -> %s) -> %s %s\n", in,
                    stg_get_state_name(old), stg_get_state_name(new),
                    stg_get_state_name(state), out);
        }

        new_edge = stg_create_transition(new, state, in, out);
        if (st_delete(edge_trans, (char **) &edge, (char **) &trans_arr)) {
            st_insert(edge_trans, (char *) new_edge, (char *) trans_arr);
        }
    }
    /* remove from cache (sigh...) */
    if (state_cache != NIL(st_table) && state_cached_stg == stg) {
        name = stg_get_state_name(old);
        st_delete(state_cache, &name, (char **) &state);
    }
    /* better not free than corrupt... */
    g_delete_vertex(old, (void (*)()) 0, (void (*)()) 0);
}

/* Explore the reachability graph of the STG by firing each enabled transition
 * in turn. Maybe stg_to_f_recur should inherit this structure in the future.
 * The code structure is borrowed from flow_state in astg_flow.c.
 * It returns the newly created state transition graph state.
 */
static vertex_t *
astg_to_stg_recur(astg_flow_t *flow_info, astg_graph *astg, graph_t *stg, astg_scode state, astg_marking *curr_marking,
                  char *in, char *out, char *buf, int use_output, st_table *edge_trans, st_table *state_marking,
                  st_table *marking_state, st_table *state_out, array_t *curr_trans, int curr_i, array_t *from_stack,
                  int from_sp) {
    astg_scode     enabled;
    astg_signal    *sig_p;
    astg_generator sgen;
    astg_scode     new_state, new_out, bit;
    int            in_cnt, out_cnt, i, dummy_cnt, k, new_curr_i;
    vertex_t       *from, *to;
    edge_t         *edge;
    array_t        *enabled_array;
    astg_trans     *trans, *trans1;
    astg_marking   *marking, *new_curr_marking;
    char           *str;
    array_t        *marking_arr, *trans_arr, *new_curr_trans;

    /* get the array of enabled transitions in the current state */
    if ((enabled_array = astg_check_new_state(flow_info, state)) ==
        NIL(array_t)) {
        assert(
                st_lookup(marking_state, (char *) flow_info->marking, (char **) &from));
    } else {
        /* create the state, if not reached yet */
        sprintf(buf, "s%s", bwd_marking_string(curr_marking->marked_places));
        from = bwd_get_state_by_name(stg, buf);
        if (from == NIL(vertex_t)) {
            from = stg_create_state(stg, buf, NIL(char));
        }

        /* all enabled transitions must fire in the proper output */
        enabled = 0;
        for (i  = array_n(enabled_array); flow_info->status == 0 && i--;) {
            trans = array_fetch(astg_trans *, enabled_array, i);
            enabled |= bwd_astg_state_bit(astg_trans_sig(trans));
        }
        marking = astg_dup_marking(flow_info->marking);
        marking->enabled = enabled;

        if (!st_lookup(state_marking, (char *) from, (char **) &marking_arr)) {
            marking_arr = array_alloc(astg_marking *, 0);
            st_insert(state_marking, (char *) from, (char *) marking_arr);
        }
        array_insert_last(astg_marking *, marking_arr, marking);
        st_insert(marking_state, (char *) marking, (char *) from);

        /* if nothing changes, we have a self loop on the current state */
        in_cnt  = out_cnt = 0;
        new_out = state ^ enabled;
        astg_foreach_signal(astg, sgen, sig_p) {
            if (astg_signal_type(sig_p) == ASTG_DUMMY_SIG) {
                continue;
            }
            bit = bwd_astg_state_bit(sig_p);
            if (use_output || astg_signal_type(sig_p) == ASTG_INPUT_SIG) {
                in[in_cnt++] = (state & bit) ? '1' : '0';
            }
            if (astg_signal_type(sig_p) == ASTG_OUTPUT_SIG ||
                astg_signal_type(sig_p) == ASTG_INTERNAL_SIG) {
                out[out_cnt++] = (new_out & bit) ? '1' : '0';
            }
        }

        /* add the new (self-loop) edge */
        edge = stg_create_nfa_transition(from, from, in, out);
        st_insert(state_out, (char *) from, stg_edge_output_string(edge));

        if (astg_debug_flag > 2) {
            fprintf(sisout, "%s %s -(self)> %s %s\n", in, stg_get_state_name(from),
                    stg_get_state_name(from), out);
        }
        new_state = state;

        dummy_cnt = 0;
        for (i    = array_n(enabled_array); flow_info->status == 0 && i--;) {
            trans = array_fetch(astg_trans *, enabled_array, i);
            if (astg_signal_type(astg_trans_sig(trans)) == ASTG_DUMMY_SIG) {
                dummy_cnt++;
            }
        }
        for (i    = array_n(enabled_array); flow_info->status == 0 && i--;) {
            /* now fire each enabled transition in turn */
            trans     = array_fetch(astg_trans *, enabled_array, i);
            new_state = astg_fire(flow_info->marking, trans);

            /* check if we fired a dummy transition which is NOT unique (choice) */
            new_curr_trans = curr_trans;
            if (dummy_cnt > 1 &&
                astg_signal_type(astg_trans_sig(trans)) == ASTG_DUMMY_SIG) {
                new_curr_marking = curr_marking;
                array_insert(astg_trans *, curr_trans, curr_i, trans);
                curr_i++;
                new_curr_i = curr_i;
            } else {
                new_curr_marking = astg_dup_marking(flow_info->marking);
                if (curr_i) {
                    new_curr_trans = array_alloc(astg_trans *, 0);
                }
                new_curr_i       = 0;
            }

            array_insert(vertex_t **, from_stack, from_sp, &from);

            /* recur */
            to = astg_to_stg_recur(
                    flow_info, astg, stg, new_state, new_curr_marking, in, out, buf,
                    use_output, edge_trans, state_marking, marking_state, state_out,
                    new_curr_trans, new_curr_i, from_stack, from_sp + 1);

            if (astg_signal_type(astg_trans_sig(trans)) == ASTG_DUMMY_SIG) {
                if (from != to) {
                    stg_patch(stg, state_out, edge_trans, marking_state, state_marking,
                              from, to, from_stack, from_sp);
                    from = to;
                }
                if (dummy_cnt > 1) {
                    curr_i--;
                }
            } else {
                astg_delete_marking(new_curr_marking);
                if (curr_i) {
                    array_free(new_curr_trans);
                }
            }

            /* "In" is the input label of the new edge: it has the same
             * signal values as the current state except for the FIRED
             * transition.
             * "Out" is the output label: it has the same signal values as
             * the next state, except for the ENABLED transitions. It is
             * produced by the PREVIOUS call.
             * ALL signals go into the "in" label, only OUTPUT signals go
             * into the "out" label.
             */

            in_cnt = 0;
            astg_foreach_signal(astg, sgen, sig_p) {
                if (astg_signal_type(sig_p) == ASTG_DUMMY_SIG) {
                    continue;
                }
                bit = bwd_astg_state_bit(sig_p);
                if (use_output || astg_signal_type(sig_p) == ASTG_INPUT_SIG) {
                    in[in_cnt++] = (new_state & bit) ? '1' : '0';
                }
            }
            /* retrieve the self-loop from "to" */
            assert(st_lookup(state_out, (char *) to, (char **) &str));
            strcpy(out, str);

            /* add the new edge */
            edge = stg_create_nfa_transition(from, to, in, out);
            if (!st_lookup(edge_trans, (char *) edge, (char **) &trans_arr)) {
                trans_arr = array_alloc(astg_trans *, 0);
                st_insert(edge_trans, (char *) edge, (char *) trans_arr);
            }
            if (astg_debug_flag > 2) {
                fprintf(sisout, "%s %s - %s", in, stg_get_state_name(from),
                        TNAME(trans));
            }
            array_insert_last(astg_trans *, trans_arr, trans);
            for (k = 0; k < curr_i; k++) {
                trans1 = array_fetch(astg_trans *, curr_trans, k);
                array_insert_last(astg_trans *, trans_arr, trans1);
                if (astg_debug_flag > 2) {
                    fprintf(sisout, " %s", TNAME(trans1));
                }
            }
            if (astg_debug_flag > 2) {
                fprintf(sisout, " > %s %s\n", stg_get_state_name(to), out);
            }
            /* backtrack and go to the next transition */
            new_state = astg_unfire(flow_info->marking, trans);
        }
        array_free(enabled_array);
    }

    return from;
}

static void stg_set_prod_states(graph_t *stg) {
    lsGen    gen;
    int      n;
    vertex_t *state;
    edge_t   *trans;

    n = 0;
    stg_foreach_state(stg, gen, state) { n++; }
    stg_set_num_states(stg, n);

    n = 0;
    stg_foreach_transition(stg, gen, trans) { n++; }
    stg_set_num_products(stg, n);
}

graph_t *bwd_reduced_stg(graph_t *stg, st_table *partitions, st_table *orig_stg_names, array_t *names, int nstates) {
    graph_t  *red_stg;
    char     buf[80], *name1, *name2, *name, *str;
    vertex_t **states, *state1, *state2, *state3, *state;
    edge_t   *edge;
    lsGen    gen, gen1, gen2;
    int      cl1, cl2, cl3, merged, i, j, collapse, len;
    char     *in, *out;
    pset     deleted;
    array_t  *names1, *names2, *components, *component;

    deleted       = set_new(nstates);
    for (collapse = 0; collapse < 2; collapse++) {
        do {
            merged  = 0;
            red_stg = stg_alloc();
            stg_set_num_inputs(red_stg, stg_get_num_inputs(stg));
            stg_set_num_outputs(red_stg, stg_get_num_outputs(stg));

            states   = ALLOC(vertex_t *, nstates);
            for (cl1 = 0; cl1 < nstates; cl1++) {
                if (is_in_set(deleted, cl1)) {
                    states[cl1] = NIL(vertex_t);
                } else {
                    if (names == NIL(array_t)) {
                        sprintf(buf, "s%d", cl1);
                        states[cl1] = stg_create_state(red_stg, buf, NIL(char));
                    } else {
                        states[cl1] = stg_create_state(
                                red_stg, array_fetch(char *, names, cl1), NIL(char));
                    }
                }
            }

            stg_foreach_state(stg, gen, state1) {
                assert(st_lookup_int(partitions, (char *) state1, &cl1));

                /* take care of transitions to other partitions first */
                foreach_state_outedge(state1, gen1, edge) {
                    state2 = stg_edge_to_state(edge);
                    assert(st_lookup_int(partitions, (char *) state2, &cl2));
                    if (cl1 == cl2) {
                        continue;
                    }

                    in  = stg_edge_input_string(edge);
                    out = stg_edge_output_string(edge);

                    if (duplicate(red_stg, states[cl1], states[cl2], in, out)) {
                        if (collapse) {
                            /* merge together cl1 and cl2 */
                            if (astg_debug_flag > 1) {
                                fprintf(sisout, "merging class %d into %d\n", cl2, cl1);
                            }
                            set_insert(deleted, cl2);

                            stg_foreach_state(stg, gen2, state3) {
                                assert(st_lookup_int(partitions, (char *) state3, &cl3));
                                if (cl3 == cl2) {
                                    st_insert(partitions, (char *) state3, (char *) cl1);
                                }
                            }

                            /* update orig_stg_names */
                            if (names != NIL(array_t)) {
                                name1 = array_fetch(char *, names, cl1);
                                name2 = array_fetch(char *, names, cl2);
                                assert(st_lookup(orig_stg_names, name1, (char **) &names1));
                                assert(st_lookup(orig_stg_names, name2, (char **) &names2));
                                for (i = 0; i < array_n(names2); i++) {
                                    name = array_fetch(char *, names2, i);
                                    array_insert_last(char *, names1, name);
                                }
                                array_free(names2);
                                st_delete(orig_stg_names, &name2, NIL(char *));
                            }
                            merged = 1;
                            lsFinish(gen1);
                            break;
                        } else {
                            /* just skip it... */
                            continue;
                        }
                    }

                    (void) stg_create_transition(states[cl1], states[cl2], in, out);
                }
                if (merged) {
                    lsFinish(gen);
                    break;
                }

                /* now deal with self-loops (which should be taken care of by
                 * duplicate)
                 */
                foreach_state_outedge(state1, gen1, edge) {
                    state2 = stg_edge_to_state(edge);
                    assert(st_lookup_int(partitions, (char *) state2, &cl2));
                    in  = stg_edge_input_string(edge);
                    out = stg_edge_output_string(edge);

                    if (duplicate(red_stg, states[cl1], states[cl2], in, out)) {
                        continue;
                    }

                    (void) stg_create_transition(states[cl1], states[cl2], in, out);
                }
            }
            if (merged) {
                FREE(states);
                stg_free(red_stg);
            }
        } while (merged);

        components = graph_scc(red_stg);
        if (array_n(components) <= 1) {
            break;
        } else {
            if (astg_debug_flag > 1) {
                for (i = 0; i < array_n(components); i++) {
                    component = array_fetch(array_t *, components, i);
                    fprintf(sisout, "%d:", i);
                    len    = 4;
                    for (j = 0; j < array_n(component); j++) {
                        state = array_fetch(vertex_t *, component, j);
                        str   = stg_get_state_name(state);
                        len += strlen(str) + 1;
                        if (len > 80) {
                            fprintf(sisout, "\n   ");
                            len = strlen(str) + 4;
                        }
                        fprintf(sisout, " %s", str);
                    }
                    fprintf(sisout, "\n");
                }
            }

            if (collapse > 1) {
                fail("internal error: cannot force a connected STG ?\n");
            }
            fprintf(siserr, "warning: forcing a connected state transition graph\n");
            fprintf(siserr, "  (may not work if the signal transition graph had "
                            "critical races)\n");
            FREE(states);
            stg_free(red_stg);
        }
        array_free(components);
    }

    state1 = stg_get_start(stg);
    assert(st_lookup_int(partitions, (char *) state1, &cl1));
    stg_set_start(red_stg, states[cl1]);
    stg_set_current(red_stg, states[cl1]);
    stg_set_prod_states(red_stg);

    FREE(states);
    set_free(deleted);

    return red_stg;
}

/* Collapse any successor of "state1" whose newly enabled signal set is empty
 * or who is reached through the same transition
 */
static void
remove_subset_states(vertex_t *state1, st_table *partitions, int class, array_t *saved_names, st_table *edge_trans,
                     st_table *state_marking, int pre_minimize) {
    edge_t   *edge, *edge2;
    lsGen    gen, gen1, gen2;
    vertex_t *state2, *from;

    if (!pre_minimize) {
        return;
    }
    foreach_state_outedge(state1, gen, edge) {
        state2 = stg_edge_to_state(edge);
        if (state1 == state2 || st_is_member(partitions, (char *) state2)) {
            continue;
        }

        /* remove if there are no newly enabled signals
         */
        if (bwd_new_enabled_signals(edge, edge_trans, state_marking)) {
            continue;
        }

        if (astg_debug_flag > 1) {
            fprintf(sisout, "removing subset state %s (%d)\n",
                    stg_get_state_name(state2), class);
        }
        st_insert(partitions, (char *) state2, (char *) class);
        array_insert_last(char *, saved_names, stg_get_state_name(state2));

        remove_subset_states(state2, partitions, class, saved_names, edge_trans,
                             state_marking, pre_minimize);
    }
}

/* If pre_minimize, merge states where no new signal is enabled.
 */
static graph_t *merge_equivalent(graph_t *stg, int pre_minimize, st_table *edge_trans, st_table *state_marking,
                                 st_table *orig_stg_names) {
    st_table   *partitions;
    graph_t    *red_stg;
    int        rm, class, after;
    vertex_t   *state, *from;
    edge_t     *edge, *edge2;
    astg_scode new_enabled;
    lsGen      gen, gen1, gen2;
    array_t    *names, *saved_names;

    partitions = st_init_table(st_ptrcmp, st_ptrhash);
    names      = array_alloc(char *, 0);

    stg_foreach_state(stg, gen, state) {
        rm = 0;
        foreach_state_inedge(state, gen1, edge) {
            from = stg_edge_from_state(edge);
            if (state == from) {
                continue;
            }

            /* check for no new enabled signals */
            if (pre_minimize) {
                new_enabled = bwd_new_enabled_signals(edge, edge_trans, state_marking);

                if (!new_enabled) {
                    rm = 1;
                    lsFinish(gen1);
                    break;
                }
            }

#if 0
            /* check for parallel identical transitions, ORDERING according to
             * the position in the fanout list of "from"
             */
            after = 0;
            foreach_state_outedge (from, gen2, edge2) {
                if (edge2 == edge) {
                    after = 1;
                }
                if (! after ||
                    stg_edge_to_state (edge2) == state ||
                    stg_edge_to_state (edge2) == from) {
                    continue;
                }

                if (! strcmp (stg_edge_input_string (edge),
                    stg_edge_input_string (edge2))) {
                    rm = 1;
                    lsFinish(gen2);
                    break;
                }
            }
#endif
        }

        if (rm) {
            continue;
        }

        class = array_n(names);
        array_insert_last(char *, names, stg_get_state_name(state));
        if (astg_debug_flag > 1) {
            fprintf(sisout, "keeping state %s (%d)\n", stg_get_state_name(state),
                    class);
        }
        saved_names = array_alloc(char *, 0);
        array_insert_last(char *, saved_names, stg_get_state_name(state));
        st_insert(orig_stg_names, (char *) stg_get_state_name(state),
                  (char *) saved_names);
        st_insert(partitions, (char *) state, (char *) class);
        remove_subset_states(state, partitions, class, saved_names, edge_trans,
                             state_marking, pre_minimize);
    }

    red_stg =
            bwd_reduced_stg(stg, partitions, orig_stg_names, names, array_n(names));

    st_free_table(partitions);
    array_free(names);

    return red_stg;
}

/* hash the marked places */
int bwd_marking_hash(char *key, int modulus) {
    astg_ba_rec *ba;
#ifdef UNSAFE
    ba_element_t result;
#else
    unsigned result;
#endif
    int i;

    ba          = ((astg_marking *) key)->marked_places;
    for (result = i = 0; i < ba->n_word; i++) {
        result ^= ba->bit_array[i];
    }

    return result % modulus;
}

int bwd_marking_ptr_cmp(char *c1, char *c2) {
    char *p1, *p2;

    p1 = *(char **) c1;
    p2 = *(char **) c2;
    return bwd_marking_cmp(p1, p2);
}

/* compare two markings (astg_cmp_marking returns +1 or 0) */
int bwd_marking_cmp(char *c1, char *c2) {
    int          n;
    astg_marking *m1, *m2;
    astg_ba_rec  *ba1, *ba2;

    m1  = (astg_marking *) c1;
    m2  = (astg_marking *) c2;
    ba1 = m1->marked_places;
    ba2 = m2->marked_places;
    /* copied from ba_cmp in astg.c */
    if (ba1->n_elem != ba2->n_elem) {
        return ba1->n_elem - ba2->n_elem;
    }
#ifdef UNSAFE
    n = ba1->n_word * sizeof(ba_element_t);
#else
    n = ba1->n_word * sizeof(unsigned);
#endif
    return memcmp((char *) ba1->bit_array, (char *) ba2->bit_array, n);
}

/* Transform the ASTG into an STG from the initial state "state"
 * (ASTG = Signal Transition Graph, STG = State Transition Graph).
 * If "use_output" is TRUE, then output signals are also considered to be
 * input signals to the STG. The resulting STG is returned.
 */
graph_t *bwd_astg_to_stg(astg_graph *astg, astg_scode code, int use_output, int pre_minimize) {
    graph_t        *stg;
    char           *in, *out;
    static char    buf[512];
    int            nin, nout, change_count;
    astg_generator sgen, tgen;
    astg_signal    *sig_p;
    astg_trans     *t;
    vertex_t       *init_state, *state;
    astg_flow_t    flow_rec;
    edge_t         *edge;
    lsGen          gen;
    astg_marking   *curr_marking;
    st_table       *state_out;
    array_t        *curr_trans, *from_stack;

    /* set up variables for the recursion */
    stg = stg_alloc();
    nin = nout = 0;
    astg_foreach_signal(astg, sgen, sig_p) {
        if (astg_signal_type(sig_p) == ASTG_DUMMY_SIG) {
            continue;
        }
        /* all signals are inputs, only output signals are outputs */
        if (use_output || astg_signal_type(sig_p) == ASTG_INPUT_SIG) {
            nin++;
        }
        if (astg_signal_type(sig_p) == ASTG_OUTPUT_SIG ||
            astg_signal_type(sig_p) == ASTG_INTERNAL_SIG) {
            nout++;
        }
    }
    stg_set_num_inputs(stg, nin);
    stg_set_num_outputs(stg, nout);
    in = ALLOC(char, nin + 1);
    in[nin] = '\0';
    out = ALLOC(char, nout + 1);
    out[nout] = '\0';

    if (global_edge_trans != NIL(st_table)) {
        /* we should free the elements here */
        st_free_table(global_edge_trans);
    }
    global_edge_trans = st_init_table(st_ptrcmp, st_ptrhash);

    if (global_marking_state != NIL(st_table)) {
        st_free_table(global_marking_state);
    }
    global_marking_state = st_init_table(bwd_marking_cmp, bwd_marking_hash);

    if (global_state_marking != NIL(st_table)) {
        /* we should free the elements here */
        st_free_table(global_state_marking);
    }
    global_state_marking = st_init_table(st_ptrcmp, st_ptrhash);

    flow_rec.force_safe = ASTG_FALSE;
    flow_rec.status     = 0;
    flow_rec.stg        = astg;
    astg_foreach_trans(astg, tgen, t) { astg_set_useful(t, ASTG_FALSE); }
    flow_rec.marking = astg_initial_marking(astg);

    change_count = astg->change_count;
    astg->change_count = -1;

    astg_sel_clear(astg);
    astg_reset_state_graph(astg);
    astg->change_count           = change_count;
    flow_rec.marking->state_code = code;

    state_out = st_init_table(st_ptrcmp, st_ptrhash);

    /* recursively traverse the state transition graph */
    curr_marking = astg_dup_marking(flow_rec.marking);
    curr_trans   = array_alloc(astg_trans *, 0);
    from_stack   = array_alloc(vertex_t **, 0);
    init_state   = astg_to_stg_recur(&flow_rec, astg, stg, code, curr_marking, in,
                                     out, buf, use_output, global_edge_trans,
                                     global_state_marking, global_marking_state,
                                     state_out, curr_trans, 0, from_stack, 0);
    astg_delete_marking(curr_marking);
    array_free(curr_trans);
    array_free(from_stack);

    st_free_table(state_out);

    /* set up the global parameters of the transition graph */
    stg_set_start(stg, init_state);
    stg_set_current(stg, init_state);
    stg_set_prod_states(stg);

    if (astg_debug_flag > 1) {
        print_sg(astg, stg, global_edge_trans);
    }

    /* now look for states, where the next state differs only in the firing
     * of one transition (no new one is enabled), and merge them together.
     */
    if (global_orig_stg != NIL(graph_t)) {
        stg_free(global_orig_stg);
    }
    global_orig_stg       = stg;
    global_orig_stg_names = st_init_table(strcmp, st_strhash);
    stg                   = merge_equivalent(stg, pre_minimize, global_edge_trans,
                                             global_state_marking, global_orig_stg_names);

    return stg;
}

/* Given a string, return its PO form (the STG name): truncate any trailing
 * "_" or "_next", if any.
 * The result is a statically allocated area !!.
 */
char *bwd_po_name(char *name) {
    int         len;
    static char buf[256];

    strcpy(buf, name);
    len = strlen(buf);
    switch (buf[len - 1]) {
        case '_':assert(len > 1);
            buf[len - 1] = '\0';
            break;
        case 't':
            if (len > 5) {
                if (buf[len - 5] == '_' && buf[len - 4] == 'n' && buf[len - 3] == 'e' &&
                    buf[len - 2] == 'x' && buf[len - 1] == 't') {
                    buf[len - 5] = '\0';
                }
            }
            break;
        default:break;
    }
    return buf;
}

/* Given a string, return its fake PO form (the PO form with "_next" appended).
 * The result is a statically allocated area !!.
 */
char *bwd_fake_po_name(char *name) {
    static char buf[256];

    strcpy(buf, bwd_po_name(name));
    strcat(buf, "_next");
    return buf;
}

/* Given a string, return its fake PI form (the PO form with "_" appended).
 * The result is a statically allocated area !!.
 */
char *bwd_fake_pi_name(char *name) {
    static char buf[256];

    strcpy(buf, bwd_po_name(name));
    strcat(buf, "_");
    return buf;
}

void bwd_astg_latch(network_t *network, latch_synch_t latch_type, int keep_red) {
    node_t      *po, *fake_po, *pi, *node, *old_po;
    char        *new_name;
    lsGen       gen;
    latch_t     *latch;
    int         i;
    astg_retval status;
    astg_graph  *astg;
    astg_signal *sig_p;
    astg_scode  state, bit;
    array_t     *po_arr;

    /* find the initial marking, if missing.sh, for the initial latch value */
    astg  = astg_current(network);
    state = (astg_scode) 0;
    if (astg != NIL(astg_graph)) {
        status = astg_initial_state(astg, &state);
        if (status != ASTG_OK && status != ASTG_NOT_USC) {
            fprintf(siserr, "inconsistency during token flow (use astg_flow)\n");
            return;
        }
    }

    /* we must use this array because we are messing up with the PO list */
    po_arr = array_alloc(node_t *, 0);
    foreach_primary_output(network, gen, po) {
        array_insert_last(node_t *, po_arr, po);
    }
    for (i = 0; i < array_n(po_arr); i++) {
        po = array_fetch(node_t *, po_arr, i);
        if (!network_is_real_po(network, po)) {
            /* in the synchronous case... */
            continue;
        }
        /* find the corresponding PI (every PO must have one...) */
        pi = network_find_node(network, bwd_fake_pi_name(node_long_name(po)));
        if (pi == NIL(node_t) || pi == po) {
            /* a strange PO */
            continue;
        }

        /* remove PI's without fanout */
        if (!keep_red && node_type(pi) == PRIMARY_INPUT &&
            node_num_fanout(pi) == 0) {
            network_delete_node(network, pi);
            continue;
        }

        /* create the latch and find its initial value */
        node    = node_get_fanin(po, 0);
        fake_po = network_add_fake_primary_output(network, node);
        network_create_latch(network, &latch, fake_po, pi);
        node_patch_fanin(po, node, pi);

        latch_set_type(latch, latch_type);
        latch_set_initial_value(latch, 0);
        if (astg != NIL(astg_graph)) {
            sig_p = astg_find_named_signal(astg, bwd_po_name(node_long_name(pi)));
            if (sig_p == NIL(astg_signal)) {
                fprintf(siserr, "cannot find signal for %s\n", node_long_name(pi));
            } else {
                bit = astg_state_bit(sig_p);
                if (bit & state) {
                    latch_set_initial_value(latch, 1);
                }
            }
        }

        new_name = util_strsav(bwd_po_name(node_long_name(po)));
        network_change_node_name(network, po, new_name);

        new_name = util_strsav(bwd_fake_po_name(node_long_name(po)));
        network_change_node_name(network, fake_po, new_name);
#if 0
        /* hope the printing routines are fixed eventually... */
        node = node_get_fanin (fake_po, 0);
        if (node_num_fanout (node) == 1 && node_type (node) == INTERNAL) {
            network_change_node_name (network, node, new_name);
        }
        else {
            network_change_node_name (network, fake_po, new_name);
        }
#endif
    }
    array_free(po_arr);
}

static int sp, count;

/* Terminology and algorithm from Aho Hopcroft Ullman page 192
 * It returns LOWLINK[v]; the array of strongly connected components
 * is sc.
 */
static int scc_search(vertex_t *v, st_table *df, st_table *stack_st, array_t *components, array_t *stack_arr, int lev) {
    edge_t   *e;
    vertex_t *w;
    array_t  *component;
    lsGen    gen;
    int      i, df_v, df_w, low_v, low_w;

    if (astg_debug_flag > 2) {
        for (i = 0; i < lev; i++) {
            fprintf(sisout, " ");
        }
        fprintf(sisout, "v %s\n", stg_get_state_name(v));
    }

    df_v = count++;
    (void) st_insert(df, (char *) v, (char *) df_v);
    (void) st_insert(stack_st, (char *) v, NIL(char));
    array_insert(vertex_t *, stack_arr, sp++, v);
    low_v = df_v;

    foreach_out_edge(v, gen, e) {
        w = g_e_dest(e);
        if (astg_debug_flag > 2) {
            for (i = 0; i < lev; i++) {
                fprintf(sisout, " ");
            }
            fprintf(sisout, "w %s\n", stg_get_state_name(w));
        }

        if (st_lookup_int(df, (char *) w, &df_w)) {
            if (df_w < df_v && st_is_member(stack_st, (char *) w)) {
                low_v = MIN(low_v, df_w);
            }
        } else {
            low_w = scc_search(w, df, stack_st, components, stack_arr, lev + 1);
            low_v = MIN(low_v, low_w);
        }
        if (astg_debug_flag > 2) {
            for (i = 0; i < lev; i++) {
                fprintf(sisout, " ");
            }
            fprintf(sisout, "low_v %d\n", low_v);
        }
    }

    if (low_v == df_v) {
        component = array_alloc(vertex_t *, 0);
        array_insert_last(array_t *, components, component);
        do {
            w = array_fetch(vertex_t *, stack_arr, --sp);
            if (astg_debug_flag > 2) {
                for (i = 0; i < lev; i++) {
                    fprintf(sisout, " ");
                }
                fprintf(sisout, "c %s\n", stg_get_state_name(w));
            }
            array_insert_last(vertex_t *, component, w);
            (void) st_delete(stack_st, (char **) &w, NIL(char *));
        } while (w != v);
    }

    return low_v;
}

array_t *graph_scc(graph_t *g) {
    vertex_t *v;
    lsGen    gen;
    st_table *df, *stack_st;
    array_t  *components, *stack_arr;

    df         = st_init_table(st_ptrcmp, st_ptrhash);
    stack_st   = st_init_table(st_ptrcmp, st_ptrhash);
    components = array_alloc(array_t *, 0);
    stack_arr  = array_alloc(array_t *, 0);
    sp         = 0;
    count      = 0;

    foreach_vertex(g, gen, v) {
        if (!st_is_member(df, (char *) v)) {
            (void) scc_search(v, df, stack_st, components, stack_arr, 0);
        }
    }

    st_free_table(df);
    st_free_table(stack_st);
    array_free(stack_arr);

    return components;
}

static int covers(char *in1, char *in2) {
    int i;

    for (i = 0; in1[i]; i++) {
        if (in1[i] == '-') {
            continue;
        }
        if (in1[i] != in2[i]) {
            return 0;
        }
    }
    return 1;
}

static int compatible(char *in1, char *in2) {
    int i;

    for (i = 0; in1[i]; i++) {
        if (in1[i] == '-' || in2[i] == '-') {
            continue;
        }
        if (in1[i] != in2[i]) {
            return 0;
        }
    }
    return 1;
}

static vertex_t *
search_scr_state(graph_t *new_stg, array_t *states, char *name, char *in, char *out, char *buf, char *code, int *new) {
    int         i;
    scr_state_t *state;
    vertex_t    *new_state;

    *new = 0;
    for (i = 0; i < array_n(states); i++) {
        state = array_fetch(scr_state_t *, states, i);
        if (strcmp(state->name, name) || strcmp(state->in, in) ||
            strcmp(state->out, out)) {
            continue;
        }
#if 0
        if (strcmp (state->name, name) ||
            ! compatible (state->in, in) || strcmp (state->out, out)) {
            continue;
        }
        /* found, now check who covers whom */
        if (covers (state->in, in)) {
            return state->state;
        }
        /* we must replace the old state with the new one... */
        sprintf (buf, "%s_%s_%s", name, in, out);
        stg_set_state_name (state->state, buf);
        FREE (state->name);
        state->name = util_strsav (name);
        FREE (state->in);
        state->in = util_strsav (in);
        FREE (state->out);
        state->out = util_strsav (out);

#endif
        return state->state;
    }

    /* not found: create a new record and a new STG state */
    *new = 1;
    sprintf(buf, "%s_%s_%s", name, in, out);
    new_state = stg_create_state(new_stg, buf, code);
    state     = ALLOC(scr_state_t, 1);
    state->state = new_state;
    state->name  = util_strsav(name);
    state->in    = util_strsav(in);
    state->out   = util_strsav(out);
    array_insert_last(scr_state_t *, states, state);

    return state->state;
}

/* transform an FSM into an FSM satisfying the single cube restriction */
static void stg_scr_recur(char *buf, array_t *states, vertex_t *old_from, graph_t *old_stg, graph_t *new_stg) {
    lsGen    gen1, gen2;
    edge_t   *inedge, *outedge;
    vertex_t *old_to, *new_from, *new_to;
    char     *in, *out, *name;
    int      new;

    foreach_state_inedge(old_from, gen1, inedge) {
        /* self-loops don't matter */
        if (stg_edge_from_state(inedge) == old_from) {
            continue;
        }
        in       = stg_edge_input_string(inedge);
        out      = stg_edge_output_string(inedge);
        name     = stg_get_state_name(old_from);
        new_from = search_scr_state(new_stg, states, name, in, out, buf,
                                    stg_get_state_encoding(old_from), &new);
        foreach_state_outedge(old_from, gen2, outedge) {
            old_to = stg_edge_to_state(outedge);
            /* self-loops are a special case */
            if (old_to == old_from) {
                (void) stg_create_transition(new_from, new_from,
                                             stg_edge_input_string(outedge),
                                             stg_edge_output_string(outedge));
            } else {
                in     = stg_edge_input_string(outedge);
                out    = stg_edge_output_string(outedge);
                name   = stg_get_state_name(old_to);
                new_to = search_scr_state(new_stg, states, name, in, out, buf,
                                          stg_get_state_encoding(old_to), &new);
                if (new) {
                    stg_scr_recur(buf, states, old_to, old_stg, new_stg);
                }

                if (!duplicate(new_stg, new_from, new_to, in, out)) {
                    (void) stg_create_transition(new_from, new_to, in, out);
                }
            }
        }
    }
}

/* check if some signal changes from loop to in1 but not from loop to in2 */
static int is_burst_subset(char *loop, char *in1, char *in2) {
    int i;

    for (i = 0; loop[i]; i++) {
        if (loop[i] == '-' || in1[i] == '-' || in2[i] == '-') {
            continue;
        }
        if (loop[i] != in1[i] && loop[i] == in2[i]) {
            return 0;
        }
    }
    return 1;
}

graph_t *bwd_stg_scr(graph_t *old_stg) {
    graph_t     *new_stg;
    lsGen       gen, gen1, gen2;
    vertex_t    *state, *old_start, *new_start;
    edge_t      *edge, *edge1;
    int         len, i;
    char        *buf, *in, *out, *name;
    array_t     *states;
    scr_state_t *scr_state;

    /* check if the STG has output don't cares */
    stg_foreach_transition(old_stg, gen, edge) {
        if (strchr(stg_edge_output_string(edge), '-') != NULL) {
            fprintf(siserr, "invalid output don't care:\n");
            fprintf(siserr, "%s %s %s %s\n", stg_edge_input_string(edge),
                    stg_get_state_name(stg_edge_from_state(edge)),
                    stg_get_state_name(stg_edge_to_state(edge)),
                    stg_edge_output_string(edge));
            return NIL(graph_t);
        }
    }

    /* initialize the new STG */
    new_stg = stg_alloc();
    stg_set_num_inputs(new_stg, stg_get_num_inputs(old_stg));
    stg_set_num_outputs(new_stg, stg_get_num_outputs(old_stg));

    /* get the max name length */
    len = 0;
    stg_foreach_state(old_stg, gen, state) {
        len = MAX(len, strlen(stg_get_state_name(state)));
    }
    len += 3 + stg_get_num_inputs(old_stg) + stg_get_num_outputs(old_stg);
    buf = ALLOC(char, len);

    /* do the actual state splitting */
    states    = array_alloc(scr_state_t *, 0);
    old_start = stg_get_start(old_stg);
    stg_scr_recur(buf, states, old_start, old_stg, new_stg);

    for (i = 0; i < array_n(states); i++) {
        scr_state = array_fetch(scr_state_t *, states, i);
        FREE(scr_state);
    }
    array_free(states);

    /* check if some burst is a subset of another burst */
    stg_foreach_state(new_stg, gen, state) {
        in = NIL(char);
        foreach_state_inedge(state, gen1, edge) {
            if (stg_edge_from_state(edge) == state) {
                continue;
            }
            in = stg_edge_input_string(edge);
            lsFinish(gen1);
            break;
        }
        if (in == NIL(char)) {
            fprintf(siserr, "state %s has only self loops ?\n",
                    stg_get_state_name(state));
            continue;
        }

        foreach_state_outedge(state, gen1, edge) {
            if (stg_edge_to_state(edge) == state) {
                continue;
            }
            foreach_state_outedge(state, gen2, edge1) {
                if (edge1 == edge || stg_edge_to_state(edge1) == state) {
                    continue;
                }
                if (is_burst_subset(in, stg_edge_input_string(edge),
                                    stg_edge_input_string(edge1))) {
                    fprintf(siserr, "warning: changing input signals of transition\n");
                    fprintf(siserr, "%s %s %s %s\n", stg_edge_input_string(edge),
                            stg_get_state_name(stg_edge_from_state(edge)),
                            stg_get_state_name(stg_edge_to_state(edge)),
                            stg_edge_output_string(edge));
                    fprintf(siserr, "are a subset of transition\n");
                    fprintf(siserr, "%s %s %s %s\n", stg_edge_input_string(edge1),
                            stg_get_state_name(stg_edge_from_state(edge1)),
                            stg_get_state_name(stg_edge_to_state(edge1)),
                            stg_edge_output_string(edge1));
                    fprintf(siserr, "(old input signal values %s)\n", in);
                }
            }
        }
    }

    /* find a reset state, the first generated by the old one */
    name = stg_get_state_name(old_start);
    len  = strlen(name);
    stg_foreach_state(new_stg, gen, new_start) {
        if (!strncmp(stg_get_state_name(new_start), name, len)) {
            stg_set_start(new_stg, new_start);
            stg_set_current(new_stg, new_start);
            lsFinish(gen);
            break;
        }
    }

    FREE(buf);

    stg_set_prod_states(new_stg);
    return new_stg;
}

/* create the ASTG fragment corresponding to "from" */
astg_place *
stg_to_astg_recur(vertex_t *from, graph_t *stg, astg_graph *astg, edge_t *finedge, edge_t *foutedge, char *finbuf,
                  char *foutbuf, char *statebuf, char **innames, char **outnames,
                  char **stnames, char *dummyname) {
    astg_place *from_p, *to_p;
    astg_trans *from_t, *to_t, *mid_t1, *mid_t2, *trans;
    vertex_t   *to;
    lsGen      gen;
    char       *fin, *fout, *from_code, *to_code, *str, *new_name;
    int        i, len;
    static int idx = 0;

    from_p = NIL(astg_place);
    if (foutedge != NIL(edge_t)) {
        /* we are recurring on the fanout cube */
        str = strchr(foutbuf, '-');
        if (str == NULL) {
            /* we are at the bottom of the double recursion: finedge and foutedge
             * are set, and finbuf and foutbuf contain the resolved values
             */
            fout = stg_edge_input_string(foutedge);
            len  = strlen(fout);
            fin  = stg_edge_input_string(finedge);
            to   = stg_edge_to_state(foutedge);

            /* find or create the new places */
            strcpy(statebuf, stg_get_state_name(from));
            str = strchr(statebuf, '_') + 1;
            strncpy(str, finbuf, len);
            from_p = astg_find_place(astg, statebuf, /*create*/ ASTG_TRUE);

            strcpy(statebuf, stg_get_state_name(to));
            str = strchr(statebuf, '_') + 1;
            strncpy(str, foutbuf, len);
            if ((to_p = astg_find_place(astg, statebuf,
                    /*create*/ ASTG_FALSE)) == NIL(astg_place)) {
                new_name = util_strsav(statebuf);
                /* recur to create the new place */
                to_p     = stg_to_astg_recur(to, stg, astg, NIL(edge_t), NIL(edge_t),
                                             NIL(char), NIL(char), statebuf, innames,
                                             outnames, stnames, dummyname);
                to_p     = astg_find_place(astg, new_name, /*create*/ ASTG_FALSE);
                FREE(new_name);
            }
            assert(from_p != NIL(astg_place));
            assert(to_p != NIL(astg_place));

            if (astg_debug_flag > 1) {
                fprintf(sisout, "(%s %s) %s -> (%s %s) %s\n", fin, finbuf,
                        stg_get_state_name(from), fout, foutbuf,
                        stg_get_state_name(to));
            }

            /* create the dummy transitions */
            from_t = astg_find_trans(astg, dummyname, ASTG_DUMMY_X, idx++, ASTG_TRUE);
            astg_find_edge(from_p, from_t, ASTG_TRUE);
            to_t = astg_find_trans(astg, dummyname, ASTG_DUMMY_X, idx++, ASTG_TRUE);
            astg_find_edge(to_t, to_p, ASTG_TRUE);
            if (stnames != NIL(char *)) {
                mid_t1 =
                        astg_find_trans(astg, dummyname, ASTG_DUMMY_X, idx++, ASTG_TRUE);
                mid_t2 =
                        astg_find_trans(astg, dummyname, ASTG_DUMMY_X, idx++, ASTG_TRUE);
            } else {
                mid_t1 = mid_t2 =
                        astg_find_trans(astg, dummyname, ASTG_DUMMY_X, idx++, ASTG_TRUE);
            }
            astg_add_constraint(from_t, mid_t1, ASTG_TRUE);
            astg_add_constraint(mid_t2, to_t, ASTG_TRUE);
            if (astg_debug_flag > 0) {
                fprintf(sisout, "%s -> %s -> %s -> %s -> %s -> %s\n",
                        astg_place_name(from_p), astg_trans_name(from_t),
                        astg_trans_name(mid_t1), astg_trans_name(mid_t2),
                        astg_trans_name(to_t), astg_place_name(to_p));
            }

            /* create the input burst */
            fout   = stg_edge_input_string(foutedge);
            len    = strlen(fout);
            fin    = stg_edge_input_string(finedge);
            for (i = 0; i < len; i++) {
                if (fout[i] == '-') {
                    /* create the long arc */
                    if (finbuf[i] != foutbuf[i]) {
                        trans = astg_find_trans(astg, innames[i], TRANS_TYPE(finbuf[i]),
                                                idx++, ASTG_TRUE);
                        astg_add_constraint(from_t, trans, ASTG_TRUE);
                        astg_add_constraint(trans, to_t, ASTG_TRUE);
                        if (astg_debug_flag > 0) {
                            fprintf(sisout, "%s -> %s -> %s\n", astg_trans_name(from_t),
                                    astg_trans_name(trans), astg_trans_name(to_t));
                        }
                    }
                } else {
                    /* create the input transition */
                    if (finbuf[i] != foutbuf[i]) {
                        trans = astg_find_trans(astg, innames[i], TRANS_TYPE(finbuf[i]),
                                                idx++, ASTG_TRUE);
                        astg_add_constraint(from_t, trans, ASTG_TRUE);
                        astg_add_constraint(trans, mid_t1, ASTG_TRUE);
                        if (astg_debug_flag > 0) {
                            fprintf(sisout, "%s -> %s -> %s\n", astg_trans_name(from_t),
                                    astg_trans_name(trans), astg_trans_name(mid_t1));
                        }
                    }
                }
            }

            if (stnames != NIL(char *)) {
                /* create the state burst */
                from_code = stg_get_state_encoding(from);
                to_code   = stg_get_state_encoding(to);
                if (!strcmp(from_code, to_code)) {
                    /* same state code, split due to SCR */
                    astg_add_constraint(mid_t1, mid_t2, ASTG_TRUE);
                } else {
                    for (i = 0; from_code[i]; i++) {
                        if (from_code[i] != to_code[i]) {
                            trans = astg_find_trans(
                                    astg, stnames[i], TRANS_TYPE(from_code[i]), idx++, ASTG_TRUE);
                            astg_add_constraint(mid_t1, trans, ASTG_TRUE);
                            astg_add_constraint(trans, mid_t2, ASTG_TRUE);
                            if (astg_debug_flag > 0) {
                                fprintf(sisout, "%s -> %s -> %s\n", astg_trans_name(mid_t1),
                                        astg_trans_name(trans), astg_trans_name(mid_t2));
                            }
                        }
                    }
                }
            }

            /* create the output burst */
            fout = stg_edge_output_string(foutedge);
            len  = strlen(fout);
            fin  = stg_edge_output_string(finedge);

            for (i = 0; i < len; i++) {
                /* sanity check... */
                if (fout[i] == '-') {
                    fprintf(siserr, "illegal output don't care value\n");
                    fprintf(siserr, "%s %s %s %s\n", stg_edge_input_string(foutedge),
                            stg_get_state_name(from), stg_get_state_name(to),
                            stg_edge_output_string(foutedge));
                }
                /* create the output transition */
                if (fin[i] != fout[i]) {
                    trans = astg_find_trans(astg, outnames[i], TRANS_TYPE(fin[i]), idx++,
                                            ASTG_TRUE);
                    astg_add_constraint(mid_t2, trans, ASTG_TRUE);
                    astg_add_constraint(trans, to_t, ASTG_TRUE);
                    if (astg_debug_flag > 0) {
                        fprintf(sisout, "%s -> %s -> %s\n", astg_trans_name(mid_t2),
                                astg_trans_name(trans), astg_trans_name(to_t));
                    }
                }
            }
        } else {
            /* split on the two cases of the fanout cube */
            *str = '0';
            from_p =
                    stg_to_astg_recur(from, stg, astg, finedge, foutedge, finbuf, foutbuf,
                                      statebuf, innames, outnames, stnames, dummyname);
            assert(from_p != NIL(astg_place));
            *str = '1';
            from_p =
                    stg_to_astg_recur(from, stg, astg, finedge, foutedge, finbuf, foutbuf,
                                      statebuf, innames, outnames, stnames, dummyname);
            assert(from_p != NIL(astg_place));
            *str = '-';
        }
    } else if (finedge != NIL(edge_t)) {
        /* we are recurring on the fanin cube */
        str = strchr(finbuf, '-');
        if (str == NULL) {
            /* recur on the fanout cube */
            foreach_state_outedge(from, gen, foutedge) {
                if (stg_edge_to_state(foutedge) == from) {
                    continue;
                }
                foutbuf = util_strsav(stg_edge_input_string(foutedge));
                from_p  = stg_to_astg_recur(from, stg, astg, finedge, foutedge, finbuf,
                                            foutbuf, statebuf, innames, outnames,
                                            stnames, dummyname);
                FREE(foutbuf);
                assert(from_p != NIL(astg_place));
            }
        } else {
            /* split on the two cases of the fanin cube */
            *str = '0';
            from_p = stg_to_astg_recur(from, stg, astg, finedge, NIL(edge_t), finbuf,
                                       NIL(char), statebuf, innames, outnames,
                                       stnames, dummyname);
            assert(from_p != NIL(astg_place));
            *str = '1';
            from_p = stg_to_astg_recur(from, stg, astg, finedge, NIL(edge_t), finbuf,
                                       NIL(char), statebuf, innames, outnames,
                                       stnames, dummyname);
            assert(from_p != NIL(astg_place));
            *str = '-';
        }
    } else {
        /* at the top: recur on the fanin cube (the first one is enough) */
        foreach_state_inedge(from, gen, finedge) {
            if (stg_edge_from_state(finedge) == from) {
                continue;
            }
            finbuf = util_strsav(stg_edge_input_string(finedge));
            from_p = stg_to_astg_recur(from, stg, astg, finedge, NIL(edge_t), finbuf,
                                       NIL(char), statebuf, innames, outnames,
                                       stnames, dummyname);
            FREE(finbuf);
            assert(from_p != NIL(astg_place));
            break;
        }
    }
    assert(from_p != NIL(astg_place));

    return from_p;
}

/* transform an AFSM satisfying the single cube restriction into an STG */
astg_graph *bwd_stg_to_astg(graph_t *stg, network_t *network) {
    astg_graph *astg;
    lsGen      gen;
    node_t     *pi, *po;
    vertex_t   *start, *state;
    astg_place *place;
    char       *statebuf, **innames, **outnames, **stnames, *dummyname, buf[80];
    int        i, len, ni, no, ns;

    start = stg_get_start(stg);
    if (start == NIL(vertex_t)) {
        fprintf(siserr, "the state transition graph needs an initial state\n");
        return NIL(astg_graph);
    }

    astg = astg_new(network_name(network));
    astg->filename = util_strsav(network_name(network));
    astg->name     = util_strsav(network_name(network));

    ni      = stg_get_num_inputs(stg);
    innames = ALLOC(char *, ni);
    if (ni == network_num_pi(network)) {
        i = 0;
        foreach_primary_input(network, gen, pi) {
            innames[i] = util_strsav(node_long_name(pi));
            astg_find_signal(astg, innames[i], ASTG_INPUT_SIG, ASTG_TRUE);
            i++;
        }
    } else {
        for (i = 0; i < ni; i++) {
            sprintf(buf, "I%d", i);
            innames[i] = util_strsav(buf);
            astg_find_signal(astg, innames[i], ASTG_INPUT_SIG, ASTG_TRUE);
        }
    }

    no       = stg_get_num_outputs(stg);
    outnames = ALLOC(char *, no);
    if (no == network_num_po(network)) {
        i = 0;
        foreach_primary_output(network, gen, po) {
            outnames[i] = util_strsav(node_long_name(po));
            astg_find_signal(astg, outnames[i], ASTG_OUTPUT_SIG, ASTG_TRUE);
            i++;
        }
    } else {
        for (i = 0; i < no; i++) {
            sprintf(buf, "O%d", i);
            outnames[i] = util_strsav(buf);
            astg_find_signal(astg, outnames[i], ASTG_OUTPUT_SIG, ASTG_TRUE);
        }
    }

    if (stg_get_state_encoding(start) != NIL(char)) {
        ns      = strlen(stg_get_state_encoding(start));
        stnames = ALLOC(char *, ns);
        for (i  = 0; i < ns; i++) {
            sprintf(buf, "S%d", i);
            stnames[i] = util_strsav(buf);
            astg_find_signal(astg, stnames[i], ASTG_INTERNAL_SIG, ASTG_TRUE);
        }
    } else {
        stnames = NIL(char *);
    }

    dummyname = "dummy";
    astg_find_signal(astg, dummyname, ASTG_DUMMY_SIG, ASTG_TRUE);

    len      = 0;
    stg_foreach_state(stg, gen, state) {
        len = MAX(len, strlen(stg_get_state_name(state)));
    }
    statebuf = ALLOC(char, (len + 1));

    place = stg_to_astg_recur(start, stg, astg, NIL(edge_t), NIL(edge_t),
                              NIL(char), NIL(char), statebuf, innames, outnames,
                              stnames, dummyname);

    astg->has_marking               = ASTG_TRUE;
    place->type.place.initial_token = ASTG_TRUE;
    astg->file_count                = astg_change_count(astg);

    for (i = 0; i < ni; i++) {
        FREE(innames[i]);
    }
    FREE(innames);
    for (i = 0; i < no; i++) {
        FREE(outnames[i]);
    }
    FREE(outnames);
    if (stnames != NIL(char *)) {
        for (i = 0; i < ns; i++) {
            FREE(stnames[i]);
        }
        FREE(stnames);
    }
    FREE(statebuf);

    return astg;
}

static void string_to_set(pset set, char *str) {
    int i;

    for (i = 0; str[i]; i++) {
        switch (str[i]) {
            case '0':PUTINPUT(set, i, ZERO);
                break;
            case '1':PUTINPUT(set, i, ONE);
                break;
            default:PUTINPUT(set, i, TWO);
                break;
        }
    }
}

static void set_to_string(char *str, pset set, int n) {
    int i;

    for (i = 0; i < n; i++) {
        switch (GETINPUT(set, i)) {
            case ZERO:str[i] = '0';
                break;
            case ONE:str[i] = '1';
                break;
            default:str[i] = '-';
                break;
        }
    }
    str[n] = '\0';
}

/* replace each unspecified edge with a self-loop, with the same output
 * value as the entering output label.
 * The STG must satisfy the single-cube restriction
 */
void bwd_dc_to_self_loop(graph_t *stg) {
    set_family_t *F, *R;
    pset         p, p1, last;
    int          nin, nout;
    lsGen        gen, gen1;
    vertex_t     *state;
    edge_t       *edge;
    char         *in, *out;

    nin  = stg_get_num_inputs(stg);
    nout = stg_get_num_outputs(stg);
    define_cube_size(nin);
    in  = ALLOC(char, nin + 1);
    out = ALLOC(char, nout + 1);
    p   = set_new(2 * nin);
    stg_foreach_state(stg, gen, state) {
        /* collect input strings */
        F = sf_new(0, 2 * nin);
        foreach_state_outedge(state, gen1, edge) {
            string_to_set(p, stg_edge_input_string(edge));
            sf_addset(F, p);
        }
        R = complement(cube1list(F));

        /* collect the output string (SCR !) */
        foreach_state_inedge(state, gen1, edge) {
            if (stg_edge_from_state(edge) == state) {
                continue;
            }
            strcpy(out, stg_edge_output_string(edge));
            lsFinish(gen1);
            break;
        }

        /* create the self-loops */
        foreach_set(R, last, p1) {
            set_to_string(in, p1, nin);
            stg_create_transition(state, state, in, out);
        }
        sf_free(R);
        sf_free(F);
    }
    set_free(p);
    FREE(in);
    FREE(out);
}

void bwd_write_sg(FILE *fp, graph_t *stg, astg_graph *astg) {
    lsGen    gen, gen2;
    vertex_t *v;
    edge_t   *e;
    char     *state_encoding, *next_state_encoding;
    array_t  *ionames;
    int      i, first, not_open;
    FILE     *output_file;
    char     name[180];

    (void) fprintf(fp, "SG:\nSTATEVECTOR:");

    ionames = stg_get_names(stg, 1);
    if (ionames == NIL(array_t)) {
        (void) fprintf(fp, "input names cannot be found.\n");
    } else {
        for (i = 0; i < stg_get_num_inputs(stg); i++) {
            (void) sprintf(name, "%s", array_fetch(char *, ionames, i));
            if (name[strlen(name) - 1] == '_')
                name[strlen(name) - 1] = '\0';
            else
                (void) fprintf(fp, "INP ");
            (void) fprintf(fp, "%s ", name);
        }
    }
    (void) fprintf(fp, "\n");
    (void) fprintf(fp, "STATES:\n");

    stg_foreach_state(stg, gen, v) {
        first = 1;
        foreach_state_outedge(v, gen2, e) {
            if (first == 1) {
                /* catch state encoding via self-loop transition */
                state_encoding = stg_edge_input_string(e);
                /* (void) fprintf(fp,"state encoding: %s\n", state_encoding);*/
                first          = 0;
            } else {
                next_state_encoding = stg_edge_input_string(e);
                for (i              = 0; i < stg_get_num_inputs(stg); i++) {
                    if (next_state_encoding[i] != state_encoding[i]) {
                        if (state_encoding[i] == '1')
                            state_encoding[i] = 'F';
                        else if (state_encoding[i] == '0')
                            state_encoding[i] = 'R';
                    }
                }
            }
        }
        (void) fprintf(fp, "%s\n", state_encoding);
    }
}

#endif /* SIS */
