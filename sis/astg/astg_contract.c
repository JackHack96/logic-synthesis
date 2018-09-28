
/* -------------------------------------------------------------------------- *\
   contract.c -- net contraction for LSFCN STGs.

   Contraction on LSFC nets is described by Chu [87] in his thesis.  In
   section 6, a detailed analysis is given which defines several restrictions
   for how and when contraction can be done.  This is implemented here.

   Questions:
   Is the order of processing transitions important?
   Chu mentions on p. 99 that "we trust that the results developed here
   also apply to these nets which are non-FC (but behaviorally FC".  Where
   might this lead?
   Does contraction really lead to a better implementation?
   The check 6.8b can be simplified by finding a covering set of cycles instead
   of all simple cycles.  But not to worry for now.
\* ---------------------------------------------------------------------------*/

#ifdef SIS

#include "astg_core.h"
#include "astg_int.h"

static void print_place_info(astg_place *p) {
    astg_generator gen;
    astg_edge      *e;

    astg_foreach_in_edge(p, gen, e) printf(" %s", astg_v_name(astg_tail(e)));
    printf(" > %s:%d >", astg_v_name(p), p->type.place.initial_token);
    astg_foreach_out_edge(p, gen, e) printf(" %s", astg_v_name(astg_head(e)));
    printf("\n");
}

/* ----------------------------- Check Contract ----------------------------- *\

   This implements the checks from Chu's dissertation to see if it is legal
   to remove a transition as part of contraction.
\* -------------------------------------------------------------------------- */

static astg_bool check_62(astg_graph *stg, astg_vertex *te) {
    astg_edge      *e1, *e2, *e3;
    astg_vertex    *v, *p1, *p2;
    astg_vertex    *ti, *to;
    astg_generator gen1, gen2, gen3;

    astg_foreach_vertex(stg, gen1, v) v->flag0 = ASTG_FALSE;

    /* Mark input places and transitions of te */
    astg_foreach_in_edge(te, gen1, e1) {
        p1 = astg_tail(e1);
        p1->flag0 = ASTG_TRUE;
        astg_foreach_in_edge(p1, gen2, e2) {
            ti = astg_tail(e2);
            ti->flag0 = ASTG_TRUE;
        }
    }

    /* Now check Restriction 6.2, p. 100 Chu '87 */
    astg_foreach_out_edge(te, gen1, e1) {
        p1 = astg_head(e1);
        astg_foreach_out_edge(p1, gen2, e2) {
            to = astg_head(e2);
            if (to->flag0) {
                if (astg_debug_flag >= 2) {
                    msg("%s fails 6.2a:", astg_v_name(p1));
                    msg(" it's an input place for %s.\n", astg_v_name(to));
                    msg("cycle includes: %s -> %s -> %s -> ...\n", astg_v_name(te),
                        astg_v_name(p1), astg_v_name(to));
                }
                return ASTG_FALSE;
            }
            astg_foreach_out_edge(to, gen3, e3) {
                p2 = astg_head(e3);
                if (p2->flag0) {
                    if (astg_debug_flag >= 2) {
                        msg("%s fails 6.2b:", astg_v_name(p2));
                        msg(" it's an output place for %s.\n", astg_v_name(to));
                        msg("cycle is: %s -> %s -> %s -> %s ->...\n", astg_v_name(te),
                            astg_v_name(p1), astg_v_name(to), astg_v_name(p2));
                    }
                    return ASTG_FALSE;
                }
            }
        }
    }

    return ASTG_TRUE;
}

static astg_bool check_68a1(astg_vertex *te) {
    astg_edge      *e;
    astg_vertex    *place;
    astg_generator gen;

    astg_foreach_in_edge(te, gen, e) {
        place = astg_tail(e);
        if (astg_out_degree(place) > 1) {
            dbg(2, msg("6.8a1 violated by %s\n", astg_v_name(place)));
            return ASTG_FALSE;
        }
    }
    return ASTG_TRUE;
}

static astg_bool check_68a2(astg_vertex *te) {
    astg_edge      *e;
    astg_vertex    *place;
    astg_generator gen;

    astg_foreach_out_edge(te, gen, e) {
        place = astg_head(e);
        if (astg_in_degree(place) > 1) {
            dbg(2, msg("6.8a2 violated by %s\n", astg_v_name(place)));
            return ASTG_FALSE;
        }
    }
    return ASTG_TRUE;
}

static astg_bool check_68a(astg_vertex *te) {
    astg_bool pass_68a;
    /* This check can be satisfied in two ways. */
    pass_68a = check_68a1(te) || check_68a2(te);
    dbg(2, if (!pass_68a) msg("%s fails 6.8a\n"));
    return pass_68a;
}

static int mark_flag1(astg_graph *stg, void *data) {
    astg_vertex    *v;
    astg_generator gen;

    astg_foreach_path_vertex(stg, gen, v) { v->flag1 = ASTG_TRUE; }
    return 0;
}

static int check_flag1(astg_graph *stg, void *data) {
    astg_generator gen;
    astg_vertex    *v;
    int            rc = 0;

    astg_foreach_path_vertex(stg, gen, v) {
        if (v->flag1)
            rc = 1;
    }
    return rc;
}

static astg_bool check_68b(astg_graph *stg, astg_vertex *te) {
    astg_generator gen1, gen2, gen3, gen4;
    astg_vertex    *v;
    astg_vertex    *ti, *to;
    astg_edge      *e1, *e2, *e3, *e4;
    int            overlap;

    if (astg_in_degree(te) <= 1)
        return ASTG_TRUE;
    if (astg_out_degree(te) <= 1)
        return ASTG_TRUE;

    astg_foreach_in_edge(te, gen1, e1) {
        astg_foreach_in_edge(astg_tail(e1), gen2, e2) {
            ti = astg_tail(e2);
            /* Mark all cycles that ti is in. */
            astg_foreach_vertex(stg, gen3, v) v->flag1 = ASTG_FALSE;
            astg_simple_cycles(stg, ti, mark_flag1, ASTG_NO_DATA, ASTG_ALL);
            astg_foreach_out_edge(te, gen3, e3) {
                astg_foreach_out_edge(astg_head(e3), gen4, e4) {
                    to = astg_head(e4);
                    /* Check for overlap with cycles of ti */
                    overlap =
                            astg_simple_cycles(stg, to, check_flag1, ASTG_NO_DATA, ASTG_ALL);
                    if (overlap == 0) {
                        dbg(2, msg("6.8b violated for %s:", astg_v_name(te)));
                        dbg(2, msg(" %s and", astg_v_name(ti)));
                        dbg(2, msg(" %s would become sequential.\n", astg_v_name(to)));
                        return ASTG_FALSE;
                    }
                }
            }
        }
    }

    /* Whew.  Passed the check */
    return ASTG_TRUE;
}

static astg_bool check_68(astg_graph *stg, astg_vertex *te) { return check_68a(te) && check_68b(stg, te); }

static astg_bool can_contract_trans(astg_graph *stg, astg_vertex *te, astg_bool keep_fc) {
    /* The STG can be contracted by eliminating transition te iff
       te is not in the input set of the significant output signal,
       and it meets the two restrictions given in Chu's thesis. */

    astg_bool can_do_it;

    if (keep_fc)
        can_do_it = check_62(stg, te) && check_68(stg, te);
    else
        can_do_it = check_68(stg, te);

    return can_do_it;
}

static astg_bool can_contract(astg_graph *stg, astg_signal *sig_p, astg_bool keep_fc) {
    astg_bool      can_do_it = sig_p->can_elim;
    astg_generator gen;
    astg_trans     *sv;

    if (can_do_it) {
        astg_foreach_sig_trans(sig_p, gen, sv) {
            can_do_it &= can_contract_trans(stg, sv, keep_fc);
        }
    }

    dbg(2, if (!can_do_it) msg("...cannot contract signal %s.\n", sig_p->name));
    return can_do_it;
}

/* ------------------------------ Eliminate Signal -------------------------- */

static astg_bool pure_place_simple(astg_vertex *p) {
    astg_vertex    *in_trans, *out_trans;
    astg_generator gen;
    astg_vertex    *alt_p;
    astg_edge      *e;

    /* Only can test this for single fanin/fanout places. */
    if (astg_in_degree(p) == 1 && astg_out_degree(p) == 1) {
        in_trans  = p->in_edges->tail;
        out_trans = p->out_edges->head;

        /* First check the pure part. */
        if (in_trans == out_trans) {
            dbg(3, printf("not pure: "));
            dbg(3, print_place_info(p));
            return ASTG_FALSE;
        }

        /* Now check place simple. */
        astg_foreach_out_edge(in_trans, gen, e) {
            alt_p = astg_head(e);
            if (alt_p == p)
                continue;
            if (astg_in_degree(alt_p) != 1 || astg_out_degree(alt_p) != 1)
                continue;
            if (alt_p->out_edges->head == out_trans) {
                dbg(3, printf("not place simple by %s: ", astg_v_name(alt_p)));
                dbg(3, print_place_info(p));
                return ASTG_FALSE;
            }
        }
    }

    /* Seems to be pure and place simple. */
    dbg(2, print_place_info(p));
    return ASTG_TRUE;
}

static void elim_trans(astg_graph *stg, astg_vertex *te) {
    astg_edge      *e1, *e2, *ex;
    astg_vertex    *pin, *pout;
    astg_vertex    *p_prime;
    astg_generator gen1, gen2, gen3;
    char           pp_name[30];
    static int     n = 0;

    dbg(2, msg("elim_trans %s\n", astg_v_name(te)));

    astg_foreach_in_edge(te, gen1, e1) {
        pin = astg_tail(e1);
        astg_foreach_out_edge(te, gen2, e2) {
            pout = astg_head(e2);
            (void) sprintf(pp_name, "pc_%d", n++);
            /* Core leak on the name of this vertex... */
            /* Need a create_unique_vertex call. */
            p_prime = astg_new_place(stg, pp_name, NULL);
            /* It is possible that both have a token. */
            if (pin->type.place.initial_token || pout->type.place.initial_token) {
                p_prime->type.place.initial_token = ASTG_TRUE;
            }
            astg_foreach_in_edge(pin, gen3, ex) {
                astg_new_edge(astg_tail(ex), p_prime);
            }
            astg_foreach_out_edge(pout, gen3, ex) {
                astg_new_edge(p_prime, astg_head(ex));
            }
            /* check for pure and place simple. */
            if (!pure_place_simple(p_prime))
                astg_delete_place(p_prime);
        }
    }

    /* Now remove the old transition. */
    astg_foreach_in_edge(te, gen1, e1) {
        pin = astg_tail(e1);
        if (astg_out_degree(pin) == 1)
            astg_delete_place(pin);
    }
    astg_foreach_out_edge(te, gen2, e2) {
        pout = astg_head(e2);
        if (astg_in_degree(pout) == 1)
            astg_delete_place(pout);
    }
    astg_delete_trans(te);
}

static void keep_in_sig(astg_graph *stg, astg_trans *sv, astg_signal *outsig) {
    astg_vertex    *en_trans, *place;
    astg_signal    *trigger, *context;
    astg_generator gen1, gen2;
    astg_vertex    *v = sv;

    astg_foreach_input_place(v, gen1, place) {

        astg_foreach_input_trans(place, gen2, en_trans) {

            trigger = en_trans->type.trans.sig_p;
            dbg(2, msg("%s is a trigger signal\n", trigger->name));
            trigger->can_elim = ASTG_FALSE;

            for (context = trigger; context != NULL; context = context->lg_from) {
                dbg(2, msg("%s is a context signal\n", context->name));
                context->can_elim = ASTG_FALSE;
            }
        }
    }
}

static void elim_sig(astg_graph *g, astg_signal *sig_p) {
    astg_graph     *stg = g;
    astg_generator gen;
    astg_trans     *sv;

    /* eliminate signal completely. */
    astg_foreach_pos_trans(sig_p, gen, sv) { elim_trans(g, sv); }

    astg_foreach_neg_trans(sig_p, gen, sv) { elim_trans(g, sv); }

    stg->n_sig--;
    assert(sig_p->sig_type == ASTG_INPUT_SIG);
    DLL_REMOVE(sig_p, stg->sig_list, next, prev);
    /* can't free it now, since iterator is in use */
}

/* ---------------------- Net Contraction Interface ------------------------- */

astg_graph *astg_contract(astg_graph *stg, astg_signal *outsig, astg_bool keep_fc) {
    astg_graph     *cstg;
    astg_signal    *sig_p, *one_out;
    astg_trans     *sv;
    astg_generator gen;
    char           name[80];

    dbg(2, msg("\ncontract net for %s...\n", outsig->name));

    astg_foreach_signal(stg, gen, sig_p) { sig_p->can_elim = ASTG_TRUE; }
    outsig->can_elim = ASTG_FALSE;
    astg_lock_graph_shortest_path(stg, outsig);
    astg_foreach_sig_trans(outsig, gen, sv) { keep_in_sig(stg, sv, outsig); }

    /* Make duplicate graph with single output signal. */
    sprintf(name, "net_%s", astg_signal_name(outsig));
    cstg = astg_duplicate(stg, name);

    astg_foreach_signal(cstg, gen, sig_p) { sig_p->sig_type = ASTG_INPUT_SIG; }

    one_out = astg_find_named_signal(cstg, astg_signal_name(outsig));
    one_out->sig_type = ASTG_OUTPUT_SIG;
    cstg->n_out       = 1;

    /* Is this order-sensitive? */
    astg_foreach_signal(cstg, gen, sig_p) {
        if (can_contract(cstg, sig_p, keep_fc))
            elim_sig(cstg, sig_p);
    }

    return cstg;
}

#endif /* SIS */
