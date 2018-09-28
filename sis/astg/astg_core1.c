
/* -------------------------------------------------------------------------- *\
   astg.c -- Lowest level graph functions and algorithms for ASTG.
\* ---------------------------------------------------------------------------*/

#ifdef SIS

#include <astg_int.h>

#include "astg_core.h"
#include "astg_int.h"

int astg_debug_flag;

/* ------------------------------ Bit arrays -------------------------------- */

static unsigned BA_WORD_SIZE = 0; /* Number of bits in an unsigned.	*/
static unsigned BA_WORD_BITS;     /* Bits for 0..WORDSIZE-1.		*/
static unsigned BA_WORD_MASK;     /* Mask of BA_WORD_BITS 1's.		*/

void astg_ba_init_param() {
    unsigned int one_bit = 1;
    int          n_bit;

    if (BA_WORD_SIZE != 0)
        return;

    for (n_bit = 0, one_bit = 1; one_bit != 0; one_bit <<= 1) {
        n_bit++;
    }
    BA_WORD_SIZE = n_bit;

    BA_WORD_MASK = BA_WORD_BITS = 0;
    while (n_bit != 1) {
        BA_WORD_BITS++;
        BA_WORD_MASK = (BA_WORD_MASK << 1) | 1;
        n_bit /= 2;
    }
}

void astg_ba_set(astg_ba_rec *ba, int n) {
    if (n < 0 || n > ba->n_elem) fail("bad index");
    ba->bit_array[n >> BA_WORD_BITS] |= (1 << (n & BA_WORD_MASK));
}

void astg_ba_clr(astg_ba_rec *ba, int n) {
    if (n < 0 || n > ba->n_elem) fail("bad index");
    ba->bit_array[n >> BA_WORD_BITS] &= ~(1 << (n & BA_WORD_MASK));
}

astg_bool astg_ba_get(astg_ba_rec *ba, int n) {
    unsigned which_int, bit_mask;

    if (n < 0 || n > ba->n_elem) fail("bad index");
    which_int = ba->bit_array[n >> BA_WORD_BITS];
    bit_mask  = 1 << (n & BA_WORD_MASK);
    return ((which_int & bit_mask) != 0);
}

int astg_ba_cmp(astg_ba_rec *ba1, astg_ba_rec *ba2) {
    int result;
    int n_byte;

    if (ba1->n_elem != ba2->n_elem) {
        result = (ba1->n_elem - ba2->n_elem);
    } else {
        n_byte = ba1->n_word * sizeof(unsigned);
        result = memcmp((char *) ba1->bit_array, (char *) ba2->bit_array, n_byte);
    }

    return result;
}

astg_ba_rec *astg_ba_init(astg_ba_rec *b, int n) {
    /* Initialize a bitarray record. */
    int n_byte;

    b->n_elem    = n;
    b->n_word    = (n + BA_WORD_SIZE) / BA_WORD_SIZE;
    b->bit_array = ALLOC(unsigned, b->n_word);
    n_byte = b->n_word * sizeof(unsigned);
    memset((char *) b->bit_array, 0, n_byte);
    return b;
}

astg_ba_rec *astg_ba_new(int n) { return astg_ba_init(ALLOC(astg_ba_rec, 1), n); }

astg_ba_rec *astg_ba_dup(astg_ba_rec *b) {
    astg_ba_rec *bcopy;
    int         n_byte;

    bcopy  = astg_ba_new(b->n_elem);
    n_byte = b->n_word * sizeof(unsigned);
    (void) memcpy((char *) bcopy->bit_array, (char *) b->bit_array, n_byte);
    return bcopy;
}

void astg_ba_dispose(astg_ba_rec *ba) {
    FREE(ba->bit_array);
    FREE(ba);
}

/* --------------------------- Generic Vertex ------------------------------- */

static astg_vertex *astg_new_vertex(astg_graph *stg, astg_vertex_enum vtype, char *name, void *userdata) {
    astg_vertex *v = ALLOC(astg_vertex, 1);

    v->vtype = vtype;
    v->x     = v->y            = 0.0;
    stg->change_count++;
    v->stg       = stg;
    v->subset    = ASTG_TRUE;
    v->selected  = ASTG_FALSE;
    v->userdata  = userdata;
    v->out_edges = v->in_edges = NULL;
    stg->n_vertex++;
    v->name = (name == NULL) ? NULL : util_strsav(name);

    DLL_INSERT(v, stg->vertices, next, prev, stg->vtail);
    stg->vtail = v;
    return v;
}

extern astg_vertex_enum astg_v_type(astg_vertex *v) {
    /* Return whether a vertex is a place or transition. */
    return v->vtype;
}

char *astg_v_name(astg_vertex *v) { return (v->name != NULL) ? v->name : "*unnamed*"; }

void astg_set_useful(astg_vertex *v, astg_bool value) { v->useful = value; }

astg_bool astg_get_useful(astg_vertex *v) { return v->useful; }

void astg_delete_vertex(astg_vertex *v) {
    astg_graph     *stg = v->stg;
    astg_generator gen;
    astg_edge      *e;

    astg_foreach_out_edge(v, gen, e) astg_delete_edge(e);
    astg_foreach_in_edge(v, gen, e) astg_delete_edge(e);

    FREE(v->name);
    DLL_REMOVE(v, stg->vertices, next, prev);
    if (stg->vtail == v)
        stg->vtail = v->prev;
    stg->n_vertex--;
    stg->change_count++;
    FREE(v);
}

extern int astg_out_degree(astg_vertex *v) {
    /*	Return the out-degree of a vertex. */
    int            n_out_edge = 0;
    astg_generator gen;
    astg_edge      *e;

    astg_foreach_out_edge(v, gen, e) n_out_edge++;
    return n_out_edge;
}

extern int astg_in_degree(astg_vertex *v) {
    /*	Return the in-degree of a vertex. */
    int            n_in_edge = 0;
    astg_generator gen;
    astg_edge      *e;

    astg_foreach_in_edge(v, gen, e) n_in_edge++;
    return n_in_edge;
}

extern int astg_degree(astg_vertex *v) {
    /*	Return the degree (in-degree+out-degree) of a vertex. */
    return astg_in_degree(v) + astg_out_degree(v);
}

void astg_set_v_locn(astg_vertex *v, float *x, float *y) {
    v->x = *x;
    v->y = *y;
}

void astg_get_v_locn(astg_vertex *v, float *xp, float *yp) {
    *xp = v->x;
    *yp = v->y;
}

void astg_set_hilite(astg_vertex *v, astg_bool value) { v->hilited = value; }

astg_bool astg_get_hilite(astg_vertex *v) { return v->hilited; }

astg_bool astg_get_selected(astg_vertex *v) { return v->selected; }

/* ------------------------------- Places ----------------------------------- */

astg_place *astg_new_place(astg_graph *g, char *name, void *userdata) {
    astg_place *p = astg_new_vertex(g, ASTG_PLACE, name, userdata);
    p->type.place.initial_token = ASTG_FALSE;
    p->type.place.user_named    = ASTG_FALSE;
    p->type.place.flow_id       = -1;
    return p;
}

static astg_place *astg_dup_place(astg_graph *g, astg_place *old_p) {
    astg_place *p = astg_new_place(g, old_p->name, NULL);
    p->x                        = old_p->x;
    p->y                        = old_p->y;
    p->parent                   = old_p;
    old_p->alg.dup.eq           = p;
    p->type.place.initial_token = old_p->type.place.initial_token;
    return p;
}

void astg_delete_place(astg_place *p) { astg_delete_vertex(p); }

astg_bool astg_boring_place(astg_place *p) {
    return (p->vtype == ASTG_PLACE && !p->type.place.user_named &&
            astg_in_degree(p) == 1 && astg_out_degree(p) == 1);
}

extern char *astg_place_name(astg_place *p) {
    /*	Return name of a place.  Do not modify the string. */

    static char pname[80], *rc;
    char        *in, *out;

    if (astg_boring_place(p)) {
        in  = astg_trans_name(astg_tail(p->in_edges));
        out = astg_trans_name(astg_head(p->out_edges));
        sprintf(pname, "<%s,%s>", in, out);
        rc = pname;
    } else {
        rc = astg_v_name(p);
    }
    return rc;
}

extern astg_place *astg_find_place(astg_graph *stg, char *name, astg_bool create) {
    /*	Return the specified place, or NULL if it does not exist.  If create
    is true, the place is created if it does not already exist.  If name
    is NULL, it does not match any existing place, and a new, unnamed
    place is created if necessary. */

    astg_generator pgen;
    astg_place     *p, *new_p = NULL;
    astg_bool      found      = ASTG_FALSE;

    if (name != NULL) {
        astg_foreach_place(stg, pgen, p) {
            if (!found && p->name != NULL && !strcmp(p->name, name)) {
                found = ASTG_TRUE;
                new_p = p;
            }
        }
    }

    if (!found && create) {
        new_p = astg_new_place(stg, name, NULL);
    }

    return new_p;
}

void astg_make_place_name(astg_graph *stg, astg_place *p) {
    /*	If the place does not have a name (because it was created implicitly)
    then generate and assign one now. */

    char place_name[20];

    if (p->name == NULL) {
        do {
            sprintf(place_name, "pin%d", stg->next_place++);
        } while (astg_find_place(stg, place_name, ASTG_FALSE));
        p->name = util_strsav(place_name);
    }
}

/* -------------------------------------------------------------------------- *\
   The flow search history is maintained in a hash table, key=state code,
   data=array of markings and enabled transitions for that
   state.  This allows state coding problems to be detected: for a new
   state, look up all previous markings, if any are incompatible then there
   is a state coding problem.  If the marking was used directly as the key,
   this could not be done.

   The state code that is hashed is the uncorrected state code, which must
   be interpreted using the phase_adj mask after the flow is completed.
\* ---------------------------------------------------------------------------*/

static enum st_retval free_marking_info(char *key, char *value, char *arg) {
    /*  Callback for st_foreach for flow states. */
    astg_delete_state((astg_state *) value);
    return ST_DELETE;
}

astg_bool astg_reset_state_graph(astg_graph *stg) {
    /*	If the existing state graph is out of date, clear it and return
    ASTG_TRUE, else return ASTG_FALSE. */

    astg_scode     state_bit;
    astg_generator gen;
    astg_trans     *t;
    astg_place     *p;
    astg_signal    *sig_p;
    int            n_place = 0;

    if (stg->change_count == stg->flow_info.change_count)
        return ASTG_FALSE;

    stg->flow_info.status        = ASTG_OK;
    stg->flow_info.change_count  = stg->change_count;
    stg->flow_info.phase_adj     = 0;
    stg->flow_info.initial_state = 0;

    if (stg->flow_info.state_list != NULL) {
        st_foreach(stg->flow_info.state_list, free_marking_info, NIL(char));
        st_free_table(stg->flow_info.state_list);
        stg->flow_info.state_list = NULL;
    }

    state_bit = 1;
    stg->flow_info.in_width  = 0;
    stg->flow_info.out_width = 0;

    astg_foreach_signal(stg, gen, sig_p) {
        if (astg_is_noninput(sig_p)) {
            stg->flow_info.in_width++;
            stg->flow_info.out_width++;
            sig_p->state_bit = state_bit;
            state_bit <<= 1;
        }
    }

    astg_foreach_signal(stg, gen, sig_p) {
        if (astg_is_input(sig_p)) {
            stg->flow_info.in_width++;
            sig_p->state_bit = state_bit;
            state_bit <<= 1;
        }
        dbg(2, msg("%s: 0x%X\n", sig_p->name, (unsigned int)sig_p->state_bit));
    }

    astg_foreach_trans(stg, gen, t) { astg_set_useful(t, ASTG_FALSE); }

    astg_foreach_place(stg, gen, p) {
        astg_set_useful(p, ASTG_FALSE);
        astg_plx(p)->flow_id = n_place++;
    }

    stg->flow_info.state_list = st_init_table(st_numcmp, st_numhash);
    return ASTG_TRUE;
}

extern astg_state *astg_find_state(astg_graph *stg, astg_scode real_code, astg_bool create) {
    /*	Return the state information for a state code, or NULL if it could
    not be returned (flow not performed, invalid state code, etc.).  If
    not found and create is ASTG_TRUE, then a new state with no markings
    is created. */

    astg_state *state = NULL;
    astg_scode scode  = real_code ^stg->flow_info.phase_adj;

    if (!st_lookup(stg->flow_info.state_list, (char *) scode, (char **) &state)) {
        if (create) {
            /* Create a new state record with zero markings. */
            state = astg_new_state(stg);
            st_insert(stg->flow_info.state_list, (char *) scode, (char *) state);
        }
    }
    return state;
}

/* ------------------------ More Token Firing Stuff ------------------------- */

astg_bool astg_sim_node(node_t *node, astg_pi_fcn f, astg_graph *stg, void *data) {
    int         cube_n, in_n;
    node_t      *fanin;
    node_cube_t cube;
    astg_bool   result;

    switch (node_function(node)) {

        case NODE_PI:result = (*f)(node_name(node), stg, data);
            break;

        case NODE_PO:assert(node_num_fanin(node) == 1);
            result = astg_sim_node(node_get_fanin(node, 0), f, stg, data);
            break;

        case NODE_0:result = ASTG_FALSE;
            break;

        case NODE_1:result = ASTG_TRUE;
            break;

        default:result  = ASTG_FALSE;
            for (cube_n = node_num_cube(node); cube_n-- && !result;) {
                result = ASTG_TRUE;
                cube   = node_get_cube(node, cube_n);
                foreach_fanin(node, in_n, fanin) {
                    if (!result)
                        continue;
                    switch (node_get_literal(cube, in_n)) {
                        case ONE:result = astg_sim_node(fanin, f, stg, data);
                            break;
                        case ZERO:result = !astg_sim_node(fanin, f, stg, data);
                            break;
                    }
                }
            }
            break;

    } /* end switch */

    dbg(2, msg("sim %s = %d\n", node_name(node), result));
    return result;
}

astg_bool astg_signal_level(char *sig_name, astg_graph *stg, void *data) {
    astg_marking *marking = (astg_marking *) data;
    astg_signal  *sig_p;
    astg_bool    value;

    sig_p = astg_find_named_signal(stg, sig_name);
    assert(sig_p != NULL);
    value = astg_get_level(sig_p, marking->state_code);
    dbg(2, msg("  sig %s level %d\n", sig_name, value));
    return value;
}

astg_retval astg_mark(astg_vertex *place, astg_marking *marking) {
    astg_retval status = ASTG_OK;

    if (astg_get_marked(marking, place)) {
        status = ASTG_NOT_SAFE;
        astg_sel_new(place->stg, "unsafe place", ASTG_FALSE);
        astg_sel_vertex(place, ASTG_TRUE);
        dbg(1, msg("Unsafe marking at place %s\n", astg_v_name(place)));
    } else {
        astg_set_marked(marking, place, ASTG_TRUE);
    }
    return status;
}

void astg_unmark(astg_vertex *place, astg_marking *marking) {
    assert(place);
    astg_ba_clr(marking->marked_places, place->type.place.flow_id);
}

int astg_disabled_count(astg_trans *trans, astg_marking *marking) {
    /*	Returns number of input places which do not have a token. */

    int            in_degree = astg_in_degree(trans);
    int            n_marked  = 0;
    astg_edge      *e;
    astg_generator gen;
    astg_place     *p;

    astg_foreach_in_edge(trans, gen, e) {
        p = astg_tail(e);
        if (astg_get_marked(marking, p) &&
            (e->guard_eqn == NULL || astg_sim_node(e->guard, astg_signal_level,
                                                   trans->stg, (void *) marking))) {
            n_marked++;
        }
    }

    return (in_degree - n_marked);
}

void astg_print_state(astg_graph *stg, astg_scode state) {
    astg_signal    *sig_p;
    astg_generator gen;

    astg_foreach_signal(stg, gen, sig_p) {
        printf(" %s=%d", astg_signal_name(sig_p), astg_get_level(sig_p, state));
    }
    fputs("\n", stdout);
}

void astg_print_marking(
        marking_n, stg,
        marking)int marking_n; /*i title to print for this marking	*/
                astg_graph *stg;            /*i the STG doing the flow for		*/
                astg_marking *marking;      /*i the marking to print		*/
{
    astg_generator gen;
    astg_place     *p;

    printf("marking %d\n  state 0x%lX, enabled 0x%lX\n  marked:", marking_n,
           marking->state_code, marking->enabled);

    astg_foreach_place(stg, gen, p) {
        if (astg_get_marked(marking, p)) {
            printf(" %s", astg_place_name(p));
        }
    }
    printf("\n");
}

extern void astg_set_marking(astg_graph *stg, astg_marking *marking_p) {
    /*	Set the STG marking to the specified one.  This new marking will be
    returned by astg_initial_marking until it is changed again.  It is
    also used for astg_token_flow.  The stg change count is incremented
    if the new marking is different than the old one. */

    astg_generator pgen;
    astg_place     *p;
    astg_bool      changed       = !stg->has_marking;

    dbg(1, msg("changing marking to %x\n", marking_p->state_code));
    astg_foreach_place(stg, pgen, p) {
        if (astg_plx(p)->initial_token != astg_get_marked(marking_p, p)) {
            changed = ASTG_TRUE;
            astg_plx(p)->initial_token = astg_get_marked(marking_p, p);
        }
    }
    stg->flow_info.initial_state = marking_p->state_code;
    stg->has_marking             = ASTG_TRUE;
    if (changed)
        stg->change_count++;
}

astg_scode astg_adjusted_code(astg_graph *stg, astg_scode scode) { return (scode ^ stg->flow_info.phase_adj); }

astg_scode astg_fire(astg_marking *marking, astg_trans *t) {
    astg_generator gen;
    astg_edge      *e;
    astg_scode     tmask;
    astg_scode     new_state, real_state;
    int            should_be, is;
    astg_graph     *stg = t->stg;

    tmask     = astg_trx(t)->sig_p->state_bit;
    new_state = marking->state_code ^ tmask;

    if (astg_trx(t)->trans_type == ASTG_POS_X ||
        astg_trx(t)->trans_type == ASTG_NEG_X) {
        real_state = new_state ^ stg->flow_info.phase_adj;
        should_be  = (astg_trx(t)->trans_type == ASTG_POS_X);
        is         = get_bit(real_state, tmask);
        if (should_be && !is || is && !should_be) {
            /* Seem to have the phase wrong. */
            if (get_bit(stg->flow_info.phase_adj, tmask)) {
                /* But we've tried correction already. */
                stg->flow_info.status = ASTG_NOT_CSA;
                astg_sel_new(stg, "Inconsistent state assignment", ASTG_FALSE);
                dbg(1, msg("Phase error for %s.\n", astg_trans_name(t)));
                astg_sel_vertex(t, ASTG_TRUE);
            }
            set_bit(stg->flow_info.phase_adj, tmask);
        }
    }

    astg_foreach_in_edge(t, gen, e) { astg_unmark(astg_tail(e), marking); }

    marking->state_code ^= t->type.trans.sig_p->state_bit;
    dbg(2, msg("fire %s = 0x%lX\n", astg_trans_name(t), marking->state_code));

    astg_foreach_out_edge(t, gen, e) { astg_mark(astg_head(e), marking); }
    return new_state;
}

astg_scode astg_unfire(astg_marking *marking, astg_trans *t) {
    astg_edge      *e;
    astg_scode     prev_state;
    astg_generator gen;

    prev_state = marking->state_code ^ astg_trx(t)->sig_p->state_bit;

    astg_foreach_out_edge(t, gen, e) { astg_unmark(astg_head(e), marking); }

    marking->state_code ^= t->type.trans.sig_p->state_bit;
    dbg(2, msg("unfire %s = 0x%lX\n", astg_v_name(t), marking->state_code));

    astg_foreach_in_edge(t, gen, e) { astg_mark(astg_tail(e), marking); }
    return prev_state;
}

void astg_set_flow_status(astg_graph *stg, astg_retval status) { stg->flow_info.status = status; }

astg_scode astg_get_enabled(astg_marking *marking) { return marking->enabled; }

astg_retval astg_get_flow_status(astg_graph *stg) { return stg->flow_info.status; }

void astg_delete_marking(astg_marking *marking) {
    FREE(marking->marked_places);
    FREE(marking);
}

astg_marking *astg_dup_marking(astg_marking *marking) {
    astg_marking *dup = ALLOC(astg_marking, 1);
    dup->marked_places = astg_ba_dup(marking->marked_places);
    dup->enabled       = marking->enabled;
    dup->state_code    = marking->state_code;
    dup->is_dummy      = marking->is_dummy;
    return dup;
}

extern astg_marking *astg_initial_marking(astg_graph *stg) {
    /*	Return initial marking of the STG if there is one, or NULL. */

    astg_marking   *marking = ALLOC(astg_marking, 1);
    astg_generator pgen;
    astg_place     *p;
    int            n_place  = 0;

    astg_foreach_place(stg, pgen, p) { n_place++; }

    marking->marked_places = astg_ba_new(n_place);
    marking->enabled       = 0;
    marking->state_code    = 0;
    marking->is_dummy      = ASTG_FALSE;

    astg_foreach_place(stg, pgen, p) {
        if (astg_plx(p)->initial_token) {
            astg_mark(p, marking);
        }
    }
    return marking;
}

void astg_set_marked(astg_marking *marking, astg_place *p, astg_bool value) {
    if (value) {
        astg_ba_set(marking->marked_places, p->type.place.flow_id);
    } else {
        astg_ba_clr(marking->marked_places, p->type.place.flow_id);
    }
}

extern astg_bool astg_get_marked(astg_marking *marking, astg_place *p) {
    /*	Return true if this place has a token in this marking or
     * if the place has no flow-id, i.e. it has just been added (e.g. by the
     * state encoding routines)
     */

    if (p->type.place.flow_id < 0) {
        return 1;
    }
    return astg_ba_get(marking->marked_places, p->type.place.flow_id);
}

/* ---------------------------- Transitions --------------------------------- */

static char *astg_sanitize_name(char *name) {
    char c, *p, *s, buffer[100], tmp[2];

    buffer[0] = '\0';
    for (p = name; *p != 0; p++) {
        c = *p;
        if (isalpha(c)) {
            tmp[0] = c;
            tmp[1] = '\0';
            s = tmp;
        } else if (c == '+') {
            s = "_plus";
        } else if (c == '-') {
            s = "_minus";
        } else if (c == '~') {
            s = "_toggle";
        } else {
            s = "x";
        }
        strcat(buffer, s);
    }

    return util_strsav(buffer);
}

astg_trans *astg_new_trans(astg_graph *g, char *name, void *userdata) {
    astg_trans *t = astg_new_vertex(g, ASTG_TRANS, name, userdata);

    t->type.trans.sig_p      = NULL;
    t->type.trans.delay      = 0.0;
    t->type.trans.alpha_name = astg_sanitize_name(t->name);
    return t;
}

static astg_trans *astg_dup_trans(astg_graph *g, astg_trans *old_t) {
    astg_trans *t = astg_find_trans(g, old_t->type.trans.sig_p->name,
                                    old_t->type.trans.trans_type,
                                    old_t->type.trans.copy_n, TRUE);
    t->x              = old_t->x;
    t->y              = old_t->y;
    t->parent         = old_t;
    old_t->alg.dup.eq = t;
    /* Could use signal dup to copy signal info here. */
    return t;
}

void astg_delete_trans(astg_place *t) {
    astg_generator gen;
    astg_trans     *tx;

    astg_foreach_sig_trans(t->type.trans.sig_p, gen, tx) {
        if (tx->type.trans.next_t == t) {
            tx->type.trans.next_t = t->type.trans.next_t;
        }
    }

    FREE(t->type.trans.alpha_name);
    astg_delete_vertex(t);
}

static void astg_link_trans(astg_trans *new_t) {
    new_t->type.trans.next_t        = new_t->type.trans.sig_p->t_list;
    new_t->type.trans.sig_p->t_list = new_t;
}

char *astg_trans_alpha_name(astg_trans *t) { return t->type.trans.alpha_name; }

astg_trans_enum astg_trans_type(astg_trans *t) { return t->type.trans.trans_type; }

extern char *astg_trans_name(astg_place *t) {
    /*	Return name of a transition.  Do not modify the string. */

    return astg_v_name(t);
}

extern astg_trans *astg_find_trans(astg_graph *stg, char *sig_name, astg_trans_enum type, int copy_n,
                                   astg_bool create) {
    /*	Return the corresponding transition, or NULL if it does not exist.
    If create is true, the transition is created if it does not already
    exist (and thus will never return NULL). */

    astg_generator tgen;
    astg_trans     *t, *new_t = NULL;
    astg_bool      found      = ASTG_FALSE;
    astg_signal    *sig_p;
    char           *saved_name;

    astg_foreach_trans(stg, tgen, t) {
        if (!found && t->type.trans.copy_n == copy_n &&
            t->type.trans.trans_type == type &&
            !strcmp(t->type.trans.sig_p->name, sig_name)) {
            found = ASTG_TRUE;
            new_t = t;
        }
    }

    if (!found && create) {
        sig_p = astg_find_named_signal(stg, sig_name);
        if (sig_p == NULL ||
            (type == ASTG_DUMMY_X && sig_p->sig_type != ASTG_DUMMY_SIG) ||
            (type != ASTG_DUMMY_X && sig_p->sig_type == ASTG_DUMMY_SIG)) {
            new_t = NULL;
        } else {
            saved_name = astg_make_name(sig_name, type, copy_n);
            new_t      = astg_new_trans(stg, saved_name, NULL);
            new_t->type.trans.trans_type = type;
            new_t->type.trans.copy_n     = copy_n;
            new_t->type.trans.sig_p      = sig_p;
            astg_link_trans(new_t);
        }
    }

    return new_t;
}

extern astg_trans *astg_noninput_trigger(astg_trans *one_trans) {
    /*	Return a noninput signal which enables t.  If there is more than
    one such signal, an arbitrary one is selected. */

    astg_trans *t = one_trans;
    astg_place *p;

    while (astg_is_input(t->type.trans.sig_p)) {
        p = astg_tail(t->in_edges);
        t = astg_tail(p->in_edges);
        if (t == one_trans)
            return NULL;
    }
    return t;
}

astg_bool astg_has_constraint(astg_trans *t1, astg_trans *t2) {
    astg_bool      found = ASTG_FALSE;
    astg_generator pgen, tgen;
    astg_place     *p;
    astg_trans     *tx;

    astg_foreach_output_place(t1, pgen, p) {
        if (found)
            continue;
        astg_foreach_output_trans(p, tgen, tx) {
            if (tx == t2)
                found = ASTG_TRUE;
        }
    }
    return found;
}

astg_signal *astg_trans_sig(astg_trans *t) { return t->type.trans.sig_p; }

/* -------------------------------- Edges ----------------------------------- */

astg_edge *astg_new_edge(astg_vertex *tail, astg_vertex *head) {
    astg_edge  *e   = ALLOC(astg_edge, 1);
    astg_graph *stg = head->stg;

    if (head->vtype == tail->vtype) fail("");
    if (head->stg != tail->stg) fail("");

    e->userdata      = NULL;
    e->tail          = tail;
    e->guard_eqn     = NULL;
    e->guard         = NULL;
    e->spline_points = NULL;

    stg->n_edge++;
    stg->change_count++;
    DLL_PREPEND(e, tail->out_edges, next_out, prev_out);
    e->head = head;
    DLL_PREPEND(e, head->in_edges, next_in, prev_in);
    return e;
}

void astg_delete_edge(astg_edge *e) {
    astg_graph *stg = e->tail->stg;

    DLL_REMOVE(e, e->tail->out_edges, next_out, prev_out);
    DLL_REMOVE(e, e->head->in_edges, next_in, prev_in);
    stg->n_edge--;
    stg->change_count++;
    FREE(e->guard_eqn);
    if (e->spline_points != NULL)
        array_free(e->spline_points);
    FREE(e);
}

static astg_edge *astg_dup_edge(astg_edge *old_e) {
    astg_edge *e =
                      astg_new_edge(astg_tail(old_e)->alg.dup.eq, astg_head(old_e)->alg.dup.eq);
    if (old_e->guard_eqn != NULL) {
        e->guard_n   = old_e->guard_n;
        e->guard_eqn = util_strsav(old_e->guard_eqn);
    }
    if (old_e->spline_points != NULL) {
        e->spline_points = array_dup(old_e->spline_points);
    }
    return e;
}

char *astg_edge_name(astg_edge *e) {
    static char buffer[180];

    strcat(strcpy(buffer, "("), astg_v_name(astg_tail(e)));
    strcat(strcat(buffer, ","), astg_v_name(astg_head(e)));
    strcat(buffer, ")");
    return buffer;
}

astg_vertex *astg_head(astg_edge *e) { return e->head; }

astg_vertex *astg_tail(astg_edge *e) { return e->tail; }

static char *astg_make_guard_name(astg_edge *e, char *buffer, int n) {
    sprintf(buffer, "guard_%s_%s", astg_place_name(astg_tail(e)),
            astg_trans_alpha_name(astg_head(e)));
    if (strlen(buffer) >= n) fail("buffer overflow");
    return buffer;
}

node_t *astg_guard_node(astg_graph *stg, astg_edge *e) {
    char guard_name[100];

    astg_make_guard_name(e, guard_name, sizeof(guard_name));
    return network_find_node(stg->guards, guard_name);
}

astg_bool astg_set_guard(astg_edge *se, char *guard_str, io_t *source) {
    astg_bool  ok        = ASTG_TRUE; /* assume no problems. */
    int        n_missing = 0;
    char       *p;
    char       guard_name[80], buf1[80];
    network_t  *eqn_network;
    lsGen      pi_gen;
    node_t     *pi_node;
    astg_graph *stg      = astg_head(se)->stg;

    if (guard_str == NULL) { /* degenerate case */
        se->guard_eqn = NULL;
        se->guard     = NULL;
        return ok;
    }

    se->guard_eqn = util_strsav(source->buffer);
    se->guard_n   = ++stg->guard_n;
    astg_make_guard_name(se, guard_name, sizeof(guard_name));
    strcat(strcat(strcpy(buf1, guard_name), " = "), se->guard_eqn);
    while ((p = strchr(buf1, '"')) != NULL)
        *p = ' ';
    strcat(buf1, ";");
    dbg(2, msg("guard = \"%s\"\n", buf1));
    eqn_network = read_eqn_string(buf1);

    if (eqn_network == NULL) {
        ok = ASTG_FALSE;
    } else {
        foreach_primary_input(eqn_network, pi_gen, pi_node) {
            if (astg_find_named_signal(stg, node_name(pi_node)) == NULL) {
                if (source != NULL) {
                    io_error(source, "invalid signals used in guard");
                    printf("     Signal \"%s\"\n", node_name(pi_node));
                }
                n_missing++;
                ok = ASTG_FALSE;
            }
        }

        if (n_missing == 0) {
            network_append(stg->guards, eqn_network);
            se->guard = astg_guard_node(stg, se);
            assert(se->guard != NULL);
        }
        network_free(eqn_network);
    }

    return ok;
}

extern astg_edge *astg_find_edge(astg_vertex *v1, astg_vertex *v2, astg_bool create) {
    /*	Return the specified edge if it already exists.  If create is
    ASTG_TRUE, then create the edge and return it, otherwise return
    NULL. */

    astg_edge      *e;
    astg_generator gen;

    astg_foreach_out_edge(v1, gen, e) {
        if (astg_head(e) == v2) {
            return e;
        }
    }

    if (create)
        return astg_new_edge(v1, v2);
    return NULL;
}

/* -------------------------------- Graph ----------------------------------- */

astg_graph *astg_new(char *grName) {
    astg_graph *stg = ALLOC(astg_graph, 1);

    astg_ba_init_param();
    stg->name         = util_strsav(grName);
    stg->change_count = 1;
    stg->file_count   = 0;
    stg->parent       = NULL;
    stg->n_vertex     = 0;
    stg->n_edge       = 0;
    stg->vertices     = NULL;
    stg->vtail        = NULL;

    stg->has_marking = ASTG_FALSE;
    stg->comments    = lsCreate();
    stg->guards      = network_alloc();
    stg->guard_n     = 0;
    stg->sig_list    = NULL;
    stg->n_sig       = stg->n_out = 0;
    network_set_name(stg->guards, "guards");
    stg->flow_info.status       = ASTG_OK;
    stg->flow_info.state_list   = NULL;
    stg->flow_info.change_count = 0;
    stg->sm_comp                = NULL;
    stg->smc_count              = 0;
    stg->mg_comp                = NULL;
    stg->mgc_count              = 0;

    stg->next_place = 0;
    stg->next_sig   = 0;

    astg_do_daemons(stg, NIL(astg_graph), ASTG_DAEMON_ALLOC);
    return stg;
}

int astg_num_vertex(astg_graph *stg) { return stg->n_vertex; }

char *astg_name(astg_graph *stg) { return stg->name; }

void astg_free_components(array_t *component_array) {
    int i;

    if (component_array != NULL) {
        for (i = 0; i < array_n(component_array); i++) {
            array_free(array_fetch(array_t *, component_array, i));
        }
        array_free(component_array);
    }
}

void astg_delete_sig(astg_graph *stg, astg_signal *sig_p) {
    if (sig_p == NULL)
        return;
    DLL_REMOVE(sig_p, stg->sig_list, next, prev);
    array_free(sig_p->lg_edges);
    FREE(sig_p->name);
    FREE(sig_p);
}

void astg_delete(astg_graph *stg) {
    astg_generator gen;
    astg_vertex    *v;

    if (stg == NULL)
        return;

    astg_do_daemons(stg, NIL(astg_graph), ASTG_DAEMON_FREE);

    astg_foreach_place(stg, gen, v) {
        /* This dumps edges too. */
        astg_delete_place(v);
    }
    astg_foreach_trans(stg, gen, v) { astg_delete_trans(v); }

    while (stg->sig_list != NULL) {
        astg_delete_sig(stg, stg->sig_list);
    }

    if (stg->flow_info.state_list != NULL) {
        st_free_table(stg->flow_info.state_list);
    }
    lsDestroy(stg->comments, free);
    network_free(stg->guards);
    astg_free_components(stg->sm_comp);
    astg_free_components(stg->mg_comp);
    FREE(stg->name);
    FREE(stg);
}

long astg_change_count(astg_graph *stg) { return stg->change_count; }

static lsGeneric astg_copy_comments(lsGeneric data) {
    /* Callback for lsCopy(), used in astg_copy(). */
    char *s = (char *) data;
    return (lsGeneric) util_strsav(s);
}

astg_graph *astg_duplicate(astg_graph *old_g, char *name) {
    /*	Create a suitable copy of an STG.  It is not an exact duplicate
    since computed values may be reset instead of copied.  How does
    dup_signals fit into this? */

    astg_graph     *g;
    astg_generator gen;
    astg_vertex    *old_v;
    astg_edge      *old_e;
    astg_signal    *old_s, *sig_p;

    if (old_g == NULL)
        return NULL;
    g = astg_new(name ? name : old_g->name);

    g->parent = old_g;

    /* Must dup guard network before duplicating signals. */
    network_free(g->guards);
    g->guards      = network_dup(old_g->guards);
    g->next_place  = old_g->next_place;
    g->next_sig    = old_g->next_sig;
    g->comments    = lsCopy(old_g->comments, astg_copy_comments);
    g->filename    = util_strsav("<none>");
    g->has_marking = old_g->has_marking;

    astg_foreach_signal(old_g, gen, old_s) {
        sig_p = astg_find_signal(g, old_s->name, old_s->sig_type, ASTG_TRUE);
        old_s->dup      = sig_p;
        sig_p->can_elim = old_s->can_elim;
    }

    astg_foreach_place(old_g, gen, old_v) { astg_dup_place(g, old_v); }

    astg_foreach_trans(old_g, gen, old_v) { astg_dup_trans(g, old_v); }

    astg_foreach_edge(old_g, gen, old_e) { astg_dup_edge(old_e); }

    astg_do_daemons(g, old_g, ASTG_DAEMON_DUP);
    return g;
}

/* -------------------------------- Marking stuff --------------------------- */

extern astg_scode astg_marking_enabled(astg_marking *marking_p) {
    /*	Return set of enabled transitions for this marking. */

    return marking_p->enabled;
}

extern int astg_state_count(astg_graph *stg) {
    /*	Return the number of states in the state graph.  This is only valid
    after some call which does token flow such as astg_token_flow or
    astg_initial_state. */

    return st_count(stg->flow_info.state_list);
}

extern astg_scode astg_state_code(astg_state *state_p) {
    /*	Return the state code for a flow state. */

    astg_scode   real_code = 0;
    astg_marking *one_marking;

    one_marking = array_fetch(astg_marking *, state_p->states, 0);
    real_code   = one_marking->state_code ^ state_p->stg->flow_info.phase_adj;
    return real_code;
}

extern astg_scode astg_state_enabled(astg_state *state_p) {
    /*	Return the complete set of enabled transitions in this state. */

    astg_scode     enabled = 0;
    astg_generator gen;
    astg_marking   *one_marking;

    astg_foreach_marking(state_p, gen, one_marking) {
        enabled |= astg_marking_enabled(one_marking);
    }
    return enabled;
}

int astg_state_n_marking(astg_state *state_p) { return array_n(state_p->states); }

int astg_cmp_marking(astg_marking *m1, astg_marking *m2) {
    int result = 0;
    if (m1->state_code != m2->state_code) {
        result = 1;
    } else if (astg_ba_cmp(m1->marked_places, m2->marked_places)) {
        result = 1;
    }
    return result;
}

/* --------------------------------- States stuff --------------------------- */
astg_state *astg_new_state(astg_graph *stg) {
    astg_state *state_p = ALLOC(astg_state, 1);
    state_p->states = array_alloc(astg_marking *, 0);
    state_p->stg    = stg;
    return state_p;
}

astg_bool astg_is_dummy_marking(astg_marking *marking) { return marking->is_dummy; }

void astg_add_marking(astg_state *state_p, astg_marking *marking) {
    astg_generator gen;
    astg_scode     enabled = 0;
    astg_trans     *trans;
    astg_signal    *sig_p;
    int            n_disabled;

    marking->is_dummy = ASTG_FALSE;

    astg_foreach_trans(state_p->stg, gen, trans) {
        /* All input places are marked => transition enabled. */
        n_disabled = astg_disabled_count(trans, marking);
        if (n_disabled == 0) {
            sig_p = astg_trans_sig(trans);
            enabled |= astg_state_bit(sig_p);
            if (astg_signal_type(sig_p) == ASTG_DUMMY_SIG) {
                marking->is_dummy = ASTG_TRUE;
            }
        }
    }

    marking->enabled = enabled;
    array_insert_last(astg_marking *, state_p->states, marking);
}

void astg_delete_state(astg_state *state_p) {
    astg_generator mgen;
    astg_marking   *marking;

    astg_foreach_marking(state_p, mgen, marking) {
        astg_ba_dispose(marking->marked_places);
        FREE(marking);
    }
    array_free(state_p->states);
    FREE(state_p);
}

/* ----------------------------- Iterator Functions ------------------------- */

astg_bool astg_spline_iter(astg_edge *e, astg_generator *gen, float *xp, float *yp) {
    astg_vertex *v;
    astg_bool   ok = ASTG_FALSE;
    int         i  = (gen->first_time) ? 0 : gen->gen_index;
    gen->first_time = ASTG_FALSE;

    if (e->spline_points == NULL) {
        v = astg_head(e);
        if (v->vtype == ASTG_PLACE && astg_in_degree(v) == 1 &&
            astg_out_degree(v) == 1) {
            if (i == 0) {
                v = astg_tail(e);
                *xp = v->x;
                *yp = v->y;
                i += 2;
                ok  = ASTG_TRUE;
            } else if (i == 2) {
                v = astg_head(v->out_edges);
                *xp = v->x;
                *yp = v->y;
                i += 2;
                ok  = ASTG_TRUE;
            }
        }
    } else if ((i + 1) < array_n(e->spline_points)) {
        *xp = array_fetch(float, e->spline_points, i++);
        *yp = array_fetch(float, e->spline_points, i++);
        ok = ASTG_TRUE;
    }

    gen->gen_index = i;
    return ok;
}

astg_bool astg_sig_x_iter(astg_signal *sig, astg_generator *gen, astg_trans_enum type, astg_trans **trans_p) {
    astg_trans *t = (gen->first_time) ? sig->t_list : gen->gv;
    gen->first_time = ASTG_FALSE;

    if (type != ASTG_ALL_X) {
        /* Advance to transition type of interest.	*/
        while (t != NULL && t->type.trans.trans_type != type) {
            t = t->type.trans.next_t;
        }
    }

    if (t != NULL)
        gen->gv = t->type.trans.next_t;
    *trans_p = t;
    return (t != NULL);
}

astg_bool astg_vertex_iter(astg_graph *stg, astg_generator *gen, astg_vertex_enum vtype, astg_vertex **vertex_p) {
    astg_bool   ok = ASTG_FALSE;
    astg_vertex *v;

    v = (gen->first_time) ? stg->vertices : gen->gv;
    gen->first_time = ASTG_FALSE;

    while (v != NULL && v->vtype == vtype) {
        v = v->next;
    }

    if (v == NULL) {
        gen->gv = NULL;
    } else {
        *vertex_p = v;
        gen->gv = v->next;
        ok = ASTG_TRUE;
    }

    return ok;
}

astg_bool astg_path_iter(astg_graph *stg, astg_generator *gen, astg_vertex **vertex_p) {
    astg_vertex *v;

    if (gen->first_time) {
        v = stg->path_start;
        gen->first_time = ASTG_FALSE;
    } else {
        v     = gen->gv;
        if (v == stg->path_start)
            v = NULL;
    }

    gen->gv = (v == NULL) ? NULL : v->alg.sc.trail->head;
    *vertex_p = v;
    return (v != NULL);
}

astg_bool astg_edge_iter(astg_graph *stg, astg_generator *gen, astg_edge **edge_p) {
    astg_vertex *v;
    astg_edge   *e;

    if (gen->first_time) {
        v = stg->vertices;
        e = (v == NULL) ? NULL : v->out_edges;
        gen->first_time = ASTG_FALSE;
    } else {
        v = gen->gv;
        e = gen->ge;
    }

    if (v != NULL) {
        while (e == NULL) {
            v = v->next;
            if (v == NULL) {
                gen->ge = NULL;
                break;
            }
            e = v->out_edges;
        }
    }

    gen->gv     = v;
    if (e != NULL)
        gen->ge = e->next_out;
    *edge_p = e;
    return (e != NULL);
}

astg_bool astg_out_edge_iter(astg_vertex *v, astg_generator *gen, astg_edge **edge_p) {
    astg_edge *e = (gen->first_time) ? v->out_edges : gen->ge;
    gen->first_time = ASTG_FALSE;

    if (e != NULL) {
        gen->ge = e->next_out;
        *edge_p = e;
    }
    return (e != NULL);
}

astg_bool astg_in_edge_iter(astg_vertex *v, astg_generator *gen, astg_edge **edge_p) {
    astg_edge *e = (gen->first_time) ? v->in_edges : gen->ge;
    gen->first_time = ASTG_FALSE;

    if (e != NULL) {
        gen->ge = e->next_in;
        *edge_p = e;
    }
    return (e != NULL);
}

astg_bool astg_in_vertex_iter(astg_vertex *v, astg_generator *gen, astg_vertex **vertex_p) {
    astg_edge *e = (gen->first_time) ? v->in_edges : gen->ge;
    gen->first_time = ASTG_FALSE;

    if (e != NULL) {
        gen->ge = e->next_in;
        *vertex_p = e->tail;
    }
    return (e != NULL);
}

astg_bool astg_out_vertex_iter(astg_vertex *v, astg_generator *gen, astg_vertex **vertex_p) {
    astg_edge *e = (gen->first_time) ? v->out_edges : gen->ge;
    gen->first_time = ASTG_FALSE;

    if (e != NULL) {
        gen->ge = e->next_out;
        *vertex_p = e->head;
    }
    return (e != NULL);
}

astg_bool astg_sig_iter(astg_graph *stg, astg_generator *gen, astg_signal **sig_p) {
    astg_signal *sig = (gen->first_time) ? stg->sig_list : gen->sig_p;
    gen->first_time = ASTG_FALSE;

    if (sig != NULL)
        gen->sig_p = sig->next;
    *sig_p = sig;
    return (sig != NULL);
}

astg_bool astg_state_iter(astg_graph *stg, astg_generator *gen, astg_state **state_p) {
    astg_bool    ok;
    astg_scode   key;
    st_generator *sgen;

    if (gen->first_time) {
        gen->state_gen  = st_init_gen(stg->flow_info.state_list);
        gen->first_time = ASTG_FALSE;
    }

    sgen = (st_generator *) gen->state_gen;
    ok   = sgen != NULL && st_gen(sgen, (char **) &key, (char **) state_p);

    if (!ok && gen->state_gen != NULL) {
        st_free_gen(sgen);
        gen->state_gen = NULL;
    }
    return ok;
}

astg_bool astg_marking_iter(astg_state *state, astg_generator *gen, astg_marking **marking_p) {
    int marking_n = (gen->first_time) ? 0 : gen->marking_n;
    gen->first_time = ASTG_FALSE;

    if (marking_n >= array_n(state->states))
        return ASTG_FALSE;
    gen->marking_n = marking_n + 1;
    *marking_p = array_fetch(astg_marking *, state->states, marking_n);
    return ASTG_TRUE;
}

/* ----------------------------- Misc --------------------------------------- */

astg_bool astg_is_rel(astg_trans *t1, astg_trans *t2) {
    astg_bool      found = ASTG_FALSE;
    astg_generator pgen, tgen;
    astg_place     *p;
    astg_trans     *tx;

    if (t1 == NULL || t2 == NULL)
        return ASTG_FALSE;

    astg_foreach_output_place(t1, pgen, p) {
        if (found)
            continue;
        astg_foreach_output_trans(p, tgen, tx) {
            if (tx == t2)
                found = ASTG_TRUE;
        }
    }
    return found;
}

void astg_add_constraint(astg_trans *t1, astg_trans *t2, astg_bool check) {
    /* Add an ordering constraint between these two transitions. */

    char       place_name[ASTG_NAME_LEN];
    astg_place *p;
    astg_graph *stg = t1->stg;

    if (!check || !astg_is_rel(t1, t2)) {
        dbg(2, msg("    adding constraint %s -> %s\n", t1->name, t2->name));
        sprintf(place_name, "p_%d", stg->next_place++);
        p = astg_find_place(stg, place_name, ASTG_TRUE);
        astg_new_edge(t1, p);
        astg_new_edge(p, t2);
        /* This invalidates any current marking. */
        stg->has_marking = ASTG_FALSE;
    }
}

/* ------------------- Net Selection Functions ------------------ */

void astg_sel_vertex(astg_vertex *v, astg_bool val) { v->selected = val; }

void astg_sel_edge(astg_edge *e, astg_bool val) { e->selected = val; }

void astg_sel_clear(astg_graph *stg) { stg->sel_name = NULL; }

void astg_sel_new(astg_graph *stg, char *selname, astg_bool value) {
    astg_generator gen;
    astg_place     *p;
    astg_trans     *t;
    astg_edge      *e;

    stg->sel_name = selname;
    astg_foreach_place(stg, gen, p) astg_sel_vertex(p, value);
    astg_foreach_trans(stg, gen, t) astg_sel_vertex(t, value);
    astg_foreach_edge(stg, gen, e) e->selected = value;
}

void astg_sel_show(astg_graph *stg) {
    /*	Print list of selected transitions and places.  Also, if graphics
    is enabled, highlight these vertices. */

    astg_generator tgen, pgen;
    astg_place     *p;
    astg_trans     *t;
    int            n_trans = 0, n_place = 0;
    FILE           *gfp;

    astg_foreach_trans(stg, tgen, t) {
        if (t->selected)
            n_trans++;
    }
    astg_foreach_place(stg, pgen, p) {
        if (p->selected)
            n_place++;
    }
    if (stg->sel_name == NULL || (n_trans + n_place) == 0)
        return;

    if (com_graphics_enabled()) {
        gfp = com_graphics_open("astg", stg->name, "highlight");
        fprintf(gfp, ".clear\t%s\n.nodes", stg->sel_name);
    } else {
        gfp = NULL;
    }
    fprintf(stdout, "%s:\n", stg->sel_name);

    if (n_trans != 0) {
        astg_foreach_trans(stg, tgen, t) {
            if (t->selected) {
                fprintf(stdout, "%s ", astg_trans_name(t));
                if (gfp)
                    fprintf(gfp, "\t%s", astg_v_name(t));
            }
        }
        fputs("\n", stdout);
    }

    if (n_place != 0) {
        astg_foreach_place(stg, pgen, p) {
            if (p->selected) {
                fprintf(stdout, "%s ", astg_place_name(p));
                if (gfp)
                    fprintf(gfp, "\t%s", astg_v_name(p));
            }
        }
        fputs("\n", stdout);
    }

    if (gfp) {
        fprintf(gfp, "\n");
        com_graphics_close(gfp);
    }
}

/* ---------------------------- Signal Stuff ----------------------- */

astg_signal_enum astg_signal_type(astg_signal *sig) { return sig->sig_type; }

extern char *astg_signal_name(astg_signal *sig) {
    /*	Return name of a signal.  Do not modify the string. */

    return sig->name;
}

extern astg_scode astg_state_bit(astg_signal *sig) {
    /*	Returns a bit mask for the bit being used to represent this signal.
    This mask is used to interpret the state code and enabled transitions
    from the state list and astg_get_enabled. */

    return sig->state_bit;
}

void astg_set_level(astg_signal *sig, astg_scode *scode, astg_bool value) {
    if (value) {
        set_bit(*scode, sig->state_bit);
    } else {
        clr_bit(*scode, sig->state_bit);
    }
}

astg_bool astg_get_level(astg_signal *sig, astg_scode scode) { return get_bit(scode, sig->state_bit); }

astg_bool astg_is_noninput(astg_signal *sig) {
    return (sig->sig_type == ASTG_OUTPUT_SIG ||
            sig->sig_type == ASTG_INTERNAL_SIG);
}

astg_bool astg_is_dummy(astg_signal *sig) { return (sig->sig_type == ASTG_DUMMY_SIG); }

astg_bool astg_is_input(astg_signal *sig) { return (sig->sig_type == ASTG_INPUT_SIG); }

int astg_signal_bit(astg_signal *sig) { return sig->state_bit; }

static astg_signal *sig_new(astg_graph *stg, char *name) {
    astg_signal *sig_p = ALLOC(astg_signal, 1);
    astg_signal *after = stg->sig_list;

    while (after != NULL && after->next != NULL)
        after = after->next;
    DLL_INSERT(sig_p, stg->sig_list, next, prev, after);
    sig_p->name = util_strsav(name);
    return sig_p;
}

extern astg_signal *astg_find_named_signal(astg_graph *stg, char *sig_name) {
/*	Return the signal, or NULL if it does not exist. */

    astg_signal    *sig_p, *found_sig = NULL;
    astg_generator sgen;

    astg_foreach_signal(stg, sgen, sig_p) {
        if (!strcmp(astg_signal_name(sig_p), sig_name)) {
            found_sig = sig_p;
            break;
        }
    }
    return
            found_sig;
}

extern astg_signal *astg_find_signal(astg_graph *stg, char *name, astg_signal_enum type, astg_bool create) {
    /*	Return the signal, or NULL if it does not exist. */

    astg_signal *sig_p = astg_find_named_signal(stg, name);
    node_t      *new_pi;

    if (sig_p != NULL) {
        if (type != ASTG_ANY_SIG && type != sig_p->sig_type) {
            sig_p = NULL;
        }
    } else if (sig_p == NULL && create) {
        sig_p = sig_new(stg, name);
        stg->n_sig++;
        sig_p->sig_type = type;
        sig_p->t_list   = NULL;
        if (network_find_node(stg->guards, name) == NULL) {
            new_pi = node_alloc(); /* Make a PI in the guards network. */
            network_add_primary_input(stg->guards, new_pi);
            network_change_node_name(stg->guards, new_pi, util_strsav(name));
        }
        sig_p->lg_edges = array_alloc(astg_signal *, 0);
        if (astg_is_noninput(sig_p)) {
            stg->n_out++;
        }
        sig_p->state_bit = 0;
        sig_p->can_elim  = ASTG_FALSE;
    }
    return sig_p;
}

astg_signal *astg_new_sig(astg_graph *stg, astg_signal_enum type) {
    char sig_name[40];

    do {
        sprintf(sig_name, "is%d", stg->next_sig++);
    } while (astg_find_named_signal(stg, sig_name) != NULL);

    /* Create a new internal signal. */
    return astg_find_signal(stg, sig_name, type, ASTG_TRUE);
}

extern astg_bool astg_is_trigger(astg_signal *sig1, astg_signal *sig2) {
    /*	Returns true if sig1 is a trigger signal for sig2. */

    astg_trans     *trigger, *t;
    astg_place     *p;
    astg_generator pgen, tgen, sgen;
    astg_bool      is_trigger = ASTG_FALSE;

    astg_foreach_pos_trans(sig1, sgen, trigger) {
        astg_foreach_output_place(trigger, pgen, p) {
            astg_foreach_output_trans(p, tgen, t) {
                if (t->type.trans.sig_p == sig2) {
                    is_trigger = ASTG_TRUE;
                }
            }
        }
    }

    astg_foreach_neg_trans(sig1, sgen, trigger) {
        astg_foreach_output_place(trigger, pgen, p) {
            astg_foreach_output_trans(p, tgen, t) {
                if (t->type.trans.sig_p == sig2) {
                    is_trigger = ASTG_TRUE;
                }
            }
        }
    }

    return is_trigger;
}

/* ---------------------- Write PLA (espresso) format ----------------------- */

static int add_espresso(astg_graph *stg, FILE *fout, astg_scode in_mask, astg_scode out_mask, astg_scode new_state,
                        astg_scode delta_ns) {
    astg_scode s, v;
    int        i;

    s      = new_state;
    v      = in_mask;
    for (i = stg->flow_info.in_width; i--; s >>= 1, v >>= 1) {
        fputc((v & 1) ? ((s & 1) ? '1' : '0') : '-', fout);
    }
    fputc(' ', fout);

    s      = new_state ^ delta_ns;
    v      = out_mask;
    for (i = stg->flow_info.out_width; i--; s >>= 1, v >>= 1) {
        fputc((v & 1) ? ((s & 1) ? '1' : '0') : '-', fout);
    }
    fputc('\n', fout);
}

static int cmp_encoding(char *data1, char *data2) {
    /* callback for array_sort */
    astg_signal *en1 = *((astg_signal **) data1);
    astg_signal *en2 = *((astg_signal **) data2);
    int         order;

    if (en1->state_bit < en2->state_bit)
        order = -1;
    else if (en1->state_bit == en2->state_bit)
        order = 0;
    else
        order = 1;
    return order;
}

int astg_write_pla(astg_graph *stg, FILE *fout) {
    astg_signal    *sig_p;
    array_t        *signals; /* array of (astg_signal *)	*/
    astg_generator sgen;
    astg_state     *state_p;
    astg_scode     in_mask = 0, out_mask = 0;
    int            n;

    if (astg_token_flow(stg, ASTG_FALSE) != ASTG_OK)
        return 1;

    signals = array_alloc(astg_signal *, stg->flow_info.in_width);
    astg_foreach_signal(stg, sgen, sig_p) {
        if (sig_p->sig_type != ASTG_DUMMY_SIG) {
            array_insert_last(astg_signal *, signals, sig_p);
        }
    }
    array_sort(signals, cmp_encoding);

    fprintf(fout, "# %s\n", astg_name(stg));
    fprintf(fout, ".i %d\n.o %d", array_n(signals), stg->n_out);

    fputs("\n.ilb", fout);
    for (n = 0; n < array_n(signals); n++) {
        sig_p = array_fetch(astg_signal *, signals, n);
        fprintf(fout, " %s", sig_p->name);
        in_mask |= sig_p->state_bit;
        dbg(2, msg("%s: 0x%X\n", sig_p->name, sig_p->state_bit));
    }

    fputs("\n.ob", fout);
    for (n = 0; n < array_n(signals); n++) {
        sig_p = array_fetch(astg_signal *, signals, n);
        if (astg_is_noninput(sig_p)) {
            fprintf(fout, " %s_out", sig_p->name);
            out_mask |= sig_p->state_bit;
        }
    }

    fputs("\n.type fr\n", fout);

    astg_foreach_state(stg, sgen, state_p) {
        add_espresso(stg, fout, in_mask, out_mask, astg_state_code(state_p),
                     astg_state_enabled(state_p));
    }

    fputs(".e\n", fout);
    return 0;
}

#endif /* SIS */
