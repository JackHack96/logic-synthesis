
#ifdef SIS
/* Routines to transform a Signal Transition Graph ("astg", for asynchronous
 * STG) into a network implementing it.
 */

#include "sis.h"
#include "astg_int.h"
#include "astg_core.h"
#include "bwd_int.h"

/* print the set of enabled transitions in the present state */
static void
print_enabled(stg, state, enabled)
astg_graph *stg;
astg_scode state;
astg_scode enabled;
{  
  astg_signal *s;
  astg_generator agen;

  (void)fprintf(sisout, "\tenabled transitions : [");
  astg_foreach_signal(stg, agen, s) {
    if (s->state_bit & enabled) {
      (void)fprintf(sisout,"%s%s ", s->name, (s->state_bit & state)?"-":"+");
    }
  }
  (void)fprintf(sisout, "]\n");
}

/* return the transition enabled in marking_p itself AND the markings reached
 * by firing dummy transitions
 */
static astg_scode 
astg_marking_enabled_dummy (astg, marking)
astg_graph *astg;
astg_marking *marking;
{
    astg_generator tgen;
    astg_trans *trans;
    astg_marking *dup;
    astg_scode result;
    astg_signal *sig_p;

    result = 0;
    astg_foreach_trans (astg, tgen, trans) {
        if (astg_disabled_count (trans, marking)) {
            continue;
        }
        sig_p = astg_trans_sig (trans);
        result |= astg_state_bit (sig_p);
        if (! trans->active && astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
            dup = astg_dup_marking (marking);
            (void) astg_fire (dup, trans);
            trans->active = 1;
            result |= astg_marking_enabled_dummy (astg, dup);
            trans->active = 0;
            astg_delete_marking (dup);
        }
    }
    return result;
}

/* Find maximal subsets of transitions enabled in the current state ("state")
 * that do not enable a transition on the signal we are synthesizing logic for
 * ("sig_bit"): for each such subset generate a cube of the on-set or the
 * off-set covers of the next state function ("F" and "R"), according to
 * whether the next-state value is 1 or 0 (position "sig_bit" in "value").
 *
 * The algorithm is as follows:
 * for each marking enabled in the current state
 *     for each place marked
 *         if the place is free-choice and we did not recur on it yet then
 *            for each output transition of the place
 *                enable that transition and recur
 *         else (now all free choices are solved)
 *         if the current signal is enabled firing enabled transitions then
 *            for each enabled signal
 *                fire it and recur
 *                unfire it and recur
 *         else (now the current signal is not enabled)
 *            generate a cube and add it to the on-set or off-set cover
 *
 * Meaning of (too many) parameters:
 * cur_fc: index among the free-choice places enabled in all markings enabled
 *   in the current state ("state") of the place we are currently recurring on:
 *   we choose one marked free-choice place in sequence, and we recur as long
 *   as there is choice.
 * cur_sig: integer index of the signal we are recurring on, splitting on the
 * astg: our signal transition graph.
 * sig_bit: bit mask of the signal we are synthesizing (NOT "cur_sig").
 * state: current STG state (may not be unique... so we must transform it into
 *   a list of enabled markings).
 * value: same as state, except that "sig_bit" may be complemented, if it is
 *   enabled: then "value" must be used to decide if we generate on-set or
 *   off-set cubes.
 * enabled: signals enabled in the current marking for "state".
 * F, R: current on-set and off-set covers for "cur_sig".
 * dup: cubes in the intersection of "F" and "R": used for error messages.
 * dup_arr: array, indexed by dup cube number, containing states where the
 *   intersection of "F" and "R" is not empty: used for error messages.
 *   Even entries are on-set entries, odd entries are off-set entries.
 * dup_marking: current marking where we began looking for maximal subsets:
 *   used for error messages.
 * cb: the cube (statically allocated for speed...).
 */
static void
find_maximal_subset (cur_fc, cur_sig, astg, sig_bit, state, value, enabled, F, R, dup, dup_arr, dup_marking, cb)
int cur_fc, cur_sig;
astg_graph *astg;
astg_scode sig_bit, state, value, enabled;
set_family_t *F, *R, *dup;
array_t *dup_arr;
astg_marking *dup_marking;
pset cb;
{
  astg_scode new_state, new_enabled, bit, fc_enabled, mask_fc, tmp_enabled;
  astg_signal * sig_p;
  astg_generator mgen, sgen, pgen, tgen;
  astg_place *place;
  astg_marking *marking;
  astg_trans *trans;
  int j, i, found, fc;
  pset p;
  st_table *dup_st;

  /* get the new state, if we fire all enabled transitions */
  new_state = state ^ enabled;

  /* check if a free-choice place, with index greater than "cur_fc" is
   * marked in some marking corresponding to "state"
   */
  fc = 0;
  if (astg_find_state (astg, state, /* create */ 0) == NIL(astg_state)) {
      fprintf (siserr, "Fatal error in state %x\n", state);
      fail ("cannot retrieve the current state??\n");
  }
  astg_foreach_marking (astg_find_state (astg, state, 0), mgen, marking) {
    astg_foreach_place (astg, pgen, place) {
      if (! astg_get_marked (marking, place)) {
        continue;
      }

      /* many output transitions == free-choice... */
      if (astg_out_degree (place) > 1) {
        fc++;
      }
      if (fc > cur_fc) {
        break;
      }
    }
    if (fc > cur_fc) {
      break;
    }
  }
  /* now if fc > cur_fc, an unvisited free-choice place is marked, and
   * "place" points to it
   */
  if (astg_debug_flag > 2) {
    if (fc > cur_fc) {
      fprintf (sisout, "free-choice place marked in state %x\n", state);
    }
  }

  if (fc > cur_fc) {
    /* free-choice place is marked: recur on it, and increment "cur_fc" */

    /* mask_fc is all 1 except for fanout of place */
    mask_fc = (astg_scode) ~0;
    astg_foreach_output_trans(place, tgen, trans) {
      sig_p = astg_trans_sig(trans);
      bit = astg_state_bit (sig_p);
      mask_fc &= ~ (bit);
    }

    /* fc_enabled is all 0 except for enabled fanout of place */
    fc_enabled = (astg_scode) 0;
    astg_foreach_output_trans(place, tgen, trans) {
      sig_p = astg_trans_sig(trans);
      bit = astg_state_bit (sig_p);
      fc_enabled |= bit;
    }

    /* save fc enabled bits, and remove them from enabled */
    fc_enabled &= enabled;
    tmp_enabled = enabled & mask_fc;

    if (fc_enabled == (astg_scode) 0) {
      /* check if we are really free-choice */
      /* WHY ???
       * if (astg_is_free_choice_net (astg)) {
       */
          /* no fanout transition of place is enabled in the current marking */
          find_maximal_subset (cur_fc + 1, cur_sig, astg, sig_bit,
               state, value, enabled, F, R, dup, dup_arr, dup_marking, cb);
      /*
       * }
       */
    }
    else {
      /* recur by enabling in turn each fanout transition of "place" */
      astg_foreach_output_trans(place, tgen, trans) {
        sig_p = astg_trans_sig(trans);
        bit = astg_state_bit (sig_p);
        if (fc_enabled & bit) {
          new_enabled = tmp_enabled | bit;
          find_maximal_subset (cur_fc + 1, cur_sig, astg, sig_bit,
               state, value, new_enabled, F, R, dup, dup_arr,
               dup_marking, cb);
        }
      }
    }
  }
  else {
    /* we have resolved all choices */

    /* sanity check: we want the next state to be reachable */
    if (astg_find_state (astg, new_state, /* create */ 0) == NIL(astg_state)) {
      fprintf (siserr, "Fatal error in state %x\n", state);
      fail ("no next state from current marking ??\n");
    }

    /* loop over all markings enabled in the current state (it should
     * become the other way around: the marking is a parameter of the
     * procedure, and we get the state from it, but this does not hurt...)
     */
    astg_foreach_marking (astg_find_state (astg, new_state, 0), mgen,
              marking) {
      /* now check if a transition on "sig_bit" is enabled in this marking */
      new_enabled = astg_marking_enabled_dummy (astg, marking);
      if (! new_enabled) {
        fprintf (siserr, "the STG is not live: no transition enabled in\n");
        astg_print_marking (0, astg, marking);
        continue;
      }
      if (sig_bit & new_enabled) {
        /* look for the first enabled signal at or after cur_sig, and split */
        i = 0;
        found = 0;
        astg_foreach_signal (astg, sgen, sig_p) {
          bit = astg_state_bit (sig_p);
          if (i >= cur_sig && (enabled & bit)) {
            found = 1;
            break;
          }
          i++;
        }
        if (! found) {
          /* bottom of recursion: no subset of enabled can be found */
          continue;
        }

        /* first leave the current signal enabled, and try
         * disabling some signal AFTER it, recurring
         */
        find_maximal_subset (cur_fc, i + 1, astg, sig_bit, state,
                     value, enabled, F, R, dup, dup_arr, dup_marking, cb);

        /* now disable the current signal and recur again */
        new_enabled = enabled & ~ (bit);
        if (new_enabled) {
          find_maximal_subset (cur_fc, i + 1, astg, sig_bit,
                       state, value, new_enabled, F, R, dup, dup_arr,
                       dup_marking, cb);
        }
      }
      else {
        /* no transition for "sig_bit" enabled: generate a cube */
        i = 0;
        /* values as in "value" except for enabled signals, which
         * are don't care
         */
        astg_foreach_signal (astg, sgen, sig_p) {
          if (astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
            continue;
          }
          bit = astg_state_bit (sig_p);
          if (bit & enabled) {
            PUTINPUT (cb, i, TWO);
          }
          else if (bit & value) {
            PUTINPUT (cb, i, ONE);
          }
          else {
            PUTINPUT (cb, i, ZERO);
          }
          i++;
        }
        /* choose whether on-set or off-set, according to "value" */
        if (sig_bit & value) {
          sf_addset (F, cb);
          if (astg_debug_flag > 3) {
            fprintf(sisout, "if we fire ");
            print_enabled (astg, state, enabled);
            fprintf(sisout, "the reached marking would have ");
            print_enabled (astg, new_state, new_enabled);
            fprintf(sisout, "so we add %s to F\n", pc1(cb));
          }
        }
        else {
          sf_addset (R, cb);
          if (astg_debug_flag > 3) {
            fprintf(sisout, "if we fire ");
            print_enabled (astg, state, enabled);
            fprintf(sisout, "the reached marking would have ");
            print_enabled (astg, new_state, new_enabled);
            fprintf(sisout, "so we add %s to R\n", pc1(cb));
          }
        }

        /* save non-empty intersection information, to give error
         * messages later, if required
         */
        if (dup != NIL(set_family_t)) {
          foreachi_set (dup, j, p) {
            if (cdist0 (p, cb)) {
              if (sig_bit & value) {
                  dup_st = array_fetch (st_table *, dup_arr, 2 * j);
              }
              else {
                  dup_st = array_fetch (st_table *, dup_arr, 2 * j + 1);
              }
              if (dup_st == NIL(st_table)) {
                dup_st = st_init_table (st_numcmp, st_numhash);
                if (sig_bit & value) {
                    array_insert (st_table *, dup_arr, 2 * j, dup_st);
                }
                else {
                    array_insert (st_table *, dup_arr, 2 * j + 1, dup_st);
                }
              }
              st_insert (dup_st, (char*) enabled, (char*) dup_marking);
              break;
            }
          }
        }
      }
    }
  }
}

/* Recursive step, computing the new state and exploring it if not yet
 * reached. See find_maximal_subset for the meaning of the parameters,
 * except for "reached", the symbol table of reached states.
 * For each state, we call find_maximal_subset on it, and then we fire each
 * enabled transition in turn, calling astg_to_f_recur recursively.
 * Since the STG may not be strictly USC, we need to loop for all markings
 * corresponding to "state", and we do the same in find_maximal_subset: it
 * would be MUCH better to recur on markings, rather than on states.
 */

static void
astg_to_f_recur (astg, sig_bit, state, reached, F, R, dup, dup_arr, cb)
astg_graph *astg;
astg_scode sig_bit, state;
st_table * reached;
set_family_t *F, *R, *dup;
array_t *dup_arr;
pset cb;
{
  astg_scode enabled, to_fire;
  astg_signal *sig_p;
  astg_generator sgen, mgen;
  astg_marking *marking;
  astg_scode new_state, value, bit;
  int i, found;
  static int mcount;

  /* may not be USC: check all markings */
  if (astg_find_state (astg, state, /* create */ 0) == NIL(astg_state)) {
      fprintf (siserr, "Fatal error in state %x\n", state);
      fail ("cannot retrieve the current state??\n");
  }
  astg_foreach_marking (astg_find_state (astg, state, 0), mgen, marking) {
    enabled = astg_marking_enabled (marking);
    if (! enabled) {
        /* dummy transitions... */
        continue;
    }
    value = state;
    if (enabled & sig_bit) {
      value ^= sig_bit;
    }

    if (astg_debug_flag > 3) {
      fprintf(sisout, "reached state ");
      astg_print_state (astg, state);
      fprintf(sisout, "with value ");
      astg_print_state (astg, value);
      astg_print_marking (mcount++, astg, marking);
    }

    i = 0;
    found = 0;
    astg_foreach_signal (astg, sgen, sig_p) {
      bit = astg_state_bit (sig_p);
      if (enabled & bit) {
        found = 1;
        break;
      }
      i++;
    }
    if (! found) {
      if (astg_debug_flag > 1) {
        fprintf (sisout, "no enabled transitions in current marking\n");
        astg_print_state (astg, state);
      }
      continue;
    }

    /* first produce cubes, if any */
    find_maximal_subset (0, i, astg, sig_bit, state, value, enabled,
             F, R, dup, dup_arr, marking, cb);

    /* then fire each transition in turn */
    astg_foreach_signal (astg, sgen, sig_p) {
      to_fire = astg_state_bit (sig_p);
      if (to_fire & enabled) {
        if (astg_debug_flag > 3) {
          fprintf(sisout, "firing ");
          print_enabled (astg, state, to_fire);
          fprintf(sisout, "\n");
        }

        new_state = state ^ to_fire;

        if (st_lookup (reached, (char *) new_state, NIL(char *))) {
          if (astg_debug_flag > 3) {
            fprintf(sisout, "reaching again ");
            astg_print_state (astg, new_state);
            fprintf(sisout, "\n");
          }
          continue;
        }

        st_insert (reached, (char *) new_state, NIL(char));

        astg_to_f_recur (astg, sig_bit, new_state,
                 reached, F, R, dup, dup_arr, cb);
      }
    }
  }
}

/* Add to M a minimum number of cubes to cover S + M.
 * Hacked from espresso irred.c
 */
static set_family_t *
add_cubes(S, M, R)
set_family_t *S, *M, *R;
{
  set_family_t *SM, *D;
  pset p, p1, last;
  sm_matrix *table;
  sm_row *cover;
  sm_element *pe;
  int index, added_cubes;

  /* extract a minimum cover */
  index = 0;
  foreach_set(S, last, p) {
    PUTSIZE(p, index);
    index++;
  }
  foreach_set(M, last, p) {
    PUTSIZE(p, index);
    index++;
  }
  SM = sf_append (sf_save (S), sf_save (M));
  D = complement (cube2list (SM, R));
  table = irred_derive_table(D, M, SM);
  cover = sm_minimum_cover(table, NIL(int), /* heuristic */ 0, /* debug */ 0);

  /* mark the cubes for the result */
  foreach_set(SM, last, p) {
    RESET(p, ACTIVE);
  }
  foreach_set(M, last, p) {
    p1 = GETSET(SM, SIZE(p));
    assert(setp_equal(p1, p));
    SET(p1, ACTIVE);
  }
  sm_foreach_row_element(cover, pe) {
    p1 = GETSET(SM, pe->col_num);
    SET(p1, ACTIVE);
  }

  added_cubes = (-M->count);
  sf_free(D);
  sf_free(M);
  sm_free(table);
  sm_row_free(cover);

  M = sf_inactive (SM);
  added_cubes += M->count;
  if (added_cubes > 0) {
    fprintf (sisout,
         "warning: added %d cubes to make S and R disjoint\n",
         added_cubes);
  }
  return M;
}

/* Create and return the node function for the specified signal (position
 * "index" in the "fanin" array).
 * We set up variables for astg_to_f_recur and call it. Then we check if the
 * intersection of F and R is empty. If not, we call astg_to_f_recur again
 * in order to get detailed error information. In this way we are (maybe ??)
 * faster if the STG can be synthesized...
 * We set up a minimum covering problem between the set of unexpanded and
 * the set of expanded on-set and off-set cubes.
 * Then if omit_fake is 0, we decompose each next-state function into an S
 * and an M cover, and implement a node of the form
 * x = M x + S (x = (M + S) x + S if "disjoint"
 * is 1, to make the Set and Reset inputs of the FF disjoint) by adding primary
 * outputs for S and M, so that subsequent optimizations will preserve them.
 */

void
bwd_astg_to_f (network, astg, sig_p, fanin, state, hazard_list, index, use_old, sep_s_r, disjoint, force)
network_t * network;
astg_graph *astg;
astg_signal *sig_p;
array_t *fanin;
astg_scode state;
st_table *hazard_list;
int index, use_old, sep_s_r, disjoint, force;
{
  astg_scode sig_bit, dup_enabled;
  astg_signal *tsig;
  set_family_t *F, *R, *dup, *old_F, *old_R, *new_R;
  st_table *dup_on, *dup_off;
  st_table * reached;
  st_generator *rgen;
  astg_place *place;
  astg_trans *trans;
  astg_generator pgen, tgen, sgen;
  astg_marking *dup_marking;
  pset c, last, p, old_p;
  node_t *node, *po;
  int i, old_i, j;
  sm_matrix *matrix;
  sm_row *cover;
  sm_element *el;
  int mincov_debug;
  char *name;
  array_t *dup_arr;
  set_family_t *S, *M, *tmp;
  char *buf;
  int len;
  node_t *M_node, *S_node, *R_node, **new_fanin;
  pset mask;
  char *dummy;

  name = util_strsav (bwd_fake_po_name (sig_p->name));
  mincov_debug = 0;
  F = sf_new (0, 2 * array_n (fanin));
  R = sf_new (0, 2 * array_n (fanin));
  c = set_new (2 * array_n (fanin));
  reached = st_init_table (st_numcmp, st_numhash);
  st_insert (reached, (char *) state, NIL(char));
  sig_bit = astg_state_bit (sig_p);
  define_cube_size (array_n (fanin));
  astg_foreach_trans (astg, tgen, trans) {
    trans->active = 0;
  }

  /* first pass, hopefully without problems */
  astg_to_f_recur (astg, sig_bit, state, reached, F, R, NIL(set_family_t),
           NIL(array_t), c);

  F = sf_contain (F);
  R = sf_contain (R);

  if (astg_debug_flag > 1) {
    mincov_debug = 1;

    fprintf (sisout, "on-set cover\n");
    foreach_set (F, last, p) {
      fprintf(sisout, "%s\n", pc1(p));
    }
    fprintf (sisout, "off-set cover\n");
    foreach_set (R, last, p) {
      fprintf(sisout, "%s\n", pc1(p));
    }
  }

  dup = cv_intersect(F, R);
  if (dup->count) {
    /* oops... we have a problem... find it out, signal and wrap up */
    fprintf (siserr, "State assignment problem for %s:\n",
         name);
    F->count = R->count = 0;
    st_free_table (reached);
    reached = st_init_table (st_numcmp, st_numhash);
    dup_arr = array_alloc (st_table *, 2 * dup->count);
    for (i = 0; i < 2 * dup->count; i++) {
        array_insert (st_table *, dup_arr, i, NIL(st_table));
    }
    st_insert (reached, (char *) state, NIL(char));
    astg_to_f_recur (astg, sig_bit, state, reached, F, R, dup, dup_arr, c);

    foreachi_set (dup, i, p) {
      dup_on = array_fetch (st_table *, dup_arr, 2 * i);
      dup_off = array_fetch (st_table *, dup_arr, 2 * i + 1);
      if (dup_on != NIL(st_table) && dup_off != NIL(st_table)) {
          fprintf(siserr, "signal values");
          j = 0;
          astg_foreach_signal (astg, sgen, tsig) {
            switch (GETINPUT (p, j)) {
            case ONE:
                fprintf (siserr, " %s=1", tsig->name);
                break;
            case ZERO:
                fprintf (siserr, " %s=0", tsig->name);
                break;
            case TWO:
                fprintf (siserr, " %s=+/-", tsig->name);
                break;
            default:
                break;
            }
            j++;
          }
          fprintf(siserr, "\n");
          fprintf(siserr, " on-set markings\n", pc1(p));
          st_foreach_item (dup_on, rgen, &dummy,
                (char **) &dup_marking) {
            dup_enabled = (astg_scode) dummy;
            fprintf (siserr, "  ");
            astg_foreach_place (astg, pgen, place) {
                if (astg_get_marked (dup_marking, place)) {
#ifdef UNSAFE
                    fprintf(siserr, " %s (%d)", astg_place_name(place),
                        astg_get_nmarked(dup_marking,place));
#else /* UNSAFE */
                    fprintf(siserr, " %s", astg_place_name(place));
#endif /* UNSAFE */
                }
            }
            fprintf (siserr, "\n");
          }
          fprintf(siserr, " off-set markings\n", pc1(p));
          st_foreach_item (dup_off, rgen, &dummy,
                (char **) &dup_marking) {
            dup_enabled = (astg_scode) dummy;
            fprintf (siserr, "  ");
            astg_foreach_place (astg, pgen, place) {
                if (astg_get_marked (dup_marking, place)) {
#ifdef UNSAFE
                    fprintf(siserr, " %s (%d)", astg_place_name(place),
                        astg_get_nmarked(dup_marking,place));
#else /* UNSAFE */
                    fprintf(siserr, " %s", astg_place_name(place));
#endif /* UNSAFE */
                }
            }
            fprintf (siserr, "\n");
          }
      }
      if (dup_on != NIL(st_table)) {
          st_free_table (dup_on);
      }
      if (dup_off != NIL(st_table)) {
          st_free_table (dup_off);
      }
    }
    array_free (dup_arr);
    if (force) {
        new_R = cv_dsharp (R, F);
        sf_free (R);
        R = new_R;
    }
    else {
        st_free_table (reached);
        sf_free (dup);
        sf_free (F);
        sf_free (R);
        set_free (c);

        /* create a dummy node, otherwise we will die later... */
        node = node_constant (0);
        network_add_node (network, node);
        network_change_node_name (network, node, name);
        (void) network_add_primary_output(network, node);
        return;
    }
  }

  sf_free (dup);

  /* no USC problems: generate the covers by expanding and covering */
  old_F = sf_save (F);
  old_R = sf_save (R);
  F = expand (F, R, /* nonsparse */ 0);
  R = expand (R, F, /* nonsparse */ 0);

  /* each cube in old_F must be covered by at least one cube in F */
  matrix = sm_alloc ();
  foreachi_set (old_F, old_i, old_p) {
    foreachi_set (F, i, p) {
      if (setp_implies (old_p, p)) {
        sm_insert (matrix, old_i, i);
      }
    }
  }
  cover = sm_minimum_cover (matrix, NIL(int), /*heuristic*/ 0, mincov_debug);

  /* now copy back in F only cubes in the minimum cover */
  sf_free (old_F);
  old_F = F;
  F = sf_new (cover->length, 2 * array_n (fanin));
  sm_foreach_row_element (cover, el) {
    p = GETSET (old_F, el->col_num);
    sf_addset (F, p);
  }

  sf_free (old_F);
  sm_row_free (cover);
  sm_free (matrix);

  /* each cube in old_R must be covered by at least one cube in R */
  matrix = sm_alloc ();
  foreachi_set (old_R, old_i, old_p) {
    foreachi_set (R, i, p) {
      if (setp_implies (old_p, p)) {
        sm_insert (matrix, old_i, i);
      }
    }
  }
  cover = sm_minimum_cover (matrix, NIL(int), /*heuristic*/ 0, mincov_debug);

  /* now copy back in R only cubes in the minimum cover */
  sf_free (old_R);
  old_R = R;
  R = sf_new (cover->length, 2 * array_n (fanin));
  sm_foreach_row_element (cover, el) {
    p = GETSET (old_R, el->col_num);
    sf_addset (R, p);
  }

  sf_free (old_R);
  sm_row_free (cover);
  sm_free (matrix);

  if (astg_debug_flag > 1) {
    fprintf (sisout, "final on-set cover\n");
    foreach_set (F, last, p) {
      fprintf(sisout, "%s\n", pc1(p));
    }
    fprintf (sisout, "final off-set cover\n");
    foreach_set (R, last, p) {
      fprintf(sisout, "%s\n", pc1(p));
    }
  }

  if (sep_s_r) {
      /* now partition F into S (cubes not depending on input "index") and M
       * (cubes depending on input "index")
       */
      mask = set_clear (set_new (2 * array_n (fanin)), 2 * array_n (fanin));
      PUTINPUT (mask, index, TWO);
      S = sf_new (0, 2 * array_n (fanin));
      M = sf_new (0, 2 * array_n (fanin));
      foreach_set (F, last, p) {
        switch (GETINPUT (p, index)) {
        case ONE:
          set_or (c, p, mask);
          sf_addset (M, c);
          break;
        case TWO:
          sf_addset (S, p);
          break;
        default:
          fail ("illegal cube value in bwd_astg_to_f\n");
          break;
        }
      }
      set_free (c);

      if (disjoint) {
        /* add to M all cubes of S which are not covered by it: this will make
         * S and M' disjoint
         */
        define_cube_size (array_n (fanin));
        M = add_cubes (S, M, R);
      }

      if (M->count) {
        /* create a new PO holding the function for S */
        S_node = node_create (S, array_data (node_t *, fanin), array_n (fanin));
        node_minimum_base (S_node);
        network_add_node (network, S_node);
        buf = util_strsav (name);
        len = strlen(buf);
        buf[len - 3] = 'S';
        buf[len - 2] = '_';
        buf[len - 1] = '\0';
        network_change_node_name (network, S_node, buf);
        (void) network_add_primary_output(network, S_node);

        /* create a new PO holding the function for M */
        M_node = node_create (M, array_data (node_t *, fanin), array_n (fanin));
        node_minimum_base (M_node);
        network_add_node (network, M_node);
        buf = util_strsav (name);
        len = strlen(buf);
        buf[len - 3] = 'M';
        buf[len - 2] = '_';
        buf[len - 1] = '\0';
        network_change_node_name (network, M_node, buf);
        (void) network_add_primary_output(network, M_node);

        /* now complement M to obtain M' */
        R_node = node_literal (M_node, 0);
        network_add_node (network, R_node);
        buf = util_strsav (name);
        len = strlen(buf);
        buf[len - 3] = 'R';
        buf[len - 2] = '_';
        buf[len - 1] = '\0';
        network_change_node_name (network, R_node, buf);
        (void) network_add_primary_output(network, R_node);

        /* finally build the new flip flop node with function X * (! M') + S */
        tmp = sf_new (2, 6);
        c = set_new (6);
        new_fanin = ALLOC (node_t *, 3);
        new_fanin[0] = array_fetch (node_t *, fanin, index);
        new_fanin[1] = R_node;
        new_fanin[2] = S_node;
        /* X * (! M') */
        PUTINPUT (c, 0, ONE);
        PUTINPUT (c, 1, ZERO);
        PUTINPUT (c, 2, TWO);
        sf_addset (tmp, c);
        /* S */
        PUTINPUT (c, 0, TWO);
        PUTINPUT (c, 1, TWO);
        PUTINPUT (c, 2, ONE);
        sf_addset (tmp, c);
        node = node_create (tmp, new_fanin, 3);
      }
      else {
          sf_free (S);
          sf_free (M);
          node = node_create (sf_save (F), array_data (node_t *, fanin),
            array_n(fanin));
      }
  }
  else {
      node = node_create (sf_save (F), array_data (node_t *, fanin),
        array_n(fanin));
  }
  node_minimum_base (node);
  network_add_node (network, node);
  network_change_node_name (network, node, name);
  (void) network_add_primary_output(network, node);

  if (hazard_list != NIL(st_table)) {
    /* now find potential hazards in the next-state function */
    define_cube_size (array_n (fanin));

    bwd_find_hazards (astg, sig_p, fanin, F, R, hazard_list, name, use_old);
  }

  /* wrap up and return */
  sf_free (F);
  sf_free (R);
  st_free_table (reached);
}
#endif /* SIS */
