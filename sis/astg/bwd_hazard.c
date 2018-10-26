
/* Routines to find out potential hazards in the initial two-level
 * implementation of the next state function of each STG signal
 */

#ifdef SIS
#include "sis.h"
#include "astg_int.h"
#include "astg_core.h"
#include "bwd_int.h"

#define XOR(a,b)	(((a)&&!(b))||(!(a)&&(b)))

#define ONEFROMTOP 0x8000
#define ONEFROMBOT 0x4000

#if 0
/* compare two strings: used to sort hazards (obsolete) */
static int
array_strcmp (s1, s2)
char **s1, **s2;
{
 	return strcmp (*s1, *s2);
}

/* compare two set families: used to sort hazards (obsolete) */
static int
sf_compare (sf1, sf2)
set_family_t *sf1, *sf2;
{
  register pset p1, p2;
  register int i, j;

  if (sf1->count != sf2->count) {
    return sf1->count - sf2->count;
  }
  foreachi_set (sf1, j, p1) {
    p2 = GETSET (sf2, j);
    i = LOOP(p1);
    do {
      if (p1[i] > p2[i]) {
		return 1; 
      }
      else if (p1[i] < p2[i]) {
		return -1;
      }
    }
    while (--i > 0);
  }
  return 0;
}

#endif

/* cb_intersect -- form the intersection of a cover with a cube */
/* hacked from espresso */

#define MAGIC 100               /* save 100 cubes before containment */

static set_family_t *
cb_intersect(pi, B)
pset pi;
set_family_t *B;
{
  register pset pj, lastj, pt;
  set_family_t *T, *Tsave = NULL;

  /* How large should each temporary result cover be ? */
  T = sf_new (MAGIC, B->sf_size);
  pt = T->data;

  /* Form pairwise intersection of each cube of A with each cube of B */
  foreach_set(B, lastj, pj) {
    if (cdist0(pi, pj)) {
      (void) set_and(pt, pi, pj);
      if (++T->count >= T->capacity) {
	if (Tsave == NULL)
	  Tsave = sf_contain(T);
	else
	  Tsave = sf_union(Tsave, sf_contain(T));
	T = new_cover(MAGIC);
	pt = T->data;
      } else
	pt += T->wsize;
    }
  }


  if (Tsave == NULL)
    Tsave = sf_contain(T);
  else
    Tsave = sf_union(Tsave, sf_contain(T));
  return Tsave;
}

/* convert an integer representation into a cube representation */
static void
bit_to_cube (stg, bits, cb)
astg_graph *stg;
astg_scode bits;
pset cb;
{
  register int i = 0;
  astg_signal *sig_p;
  register astg_scode bit;
  astg_generator sgen;

  astg_foreach_signal (stg, sgen, sig_p) {
	if (astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
		continue;
	}
    bit = astg_state_bit (sig_p);
    PUTINPUT (cb, i, ((bit & bits) ? ONE : ZERO));
    i++;
  }
}

/* compute the value of cover "F" for input minterm "vector" */
static int
func_value (F, vector)
set_family_t *F;
pset vector;
{
  register pset last, p;

  foreach_set (F, last, p) {
    if (cdist0 (vector, p)) {
      return 1;
    }
  }
  return 0;
}

void
bwd_dup_hazard (new, old)
hazard_t *new, *old;
{
	new->dir1 = old->dir1;
	new->dir2 = old->dir2;
	new->s1 = util_strsav (old->s1);
	new->s2 = util_strsav (old->s2);
	new->on_cb1 = set_save (old->on_cb1);
	new->off_cb1 = set_save (old->off_cb1);
	new->on_cb2 = set_save (old->on_cb2);
	new->off_cb2 = set_save (old->off_cb2);
}

void
bwd_free_hazard (haz)
hazard_t *haz;
{
	FREE (haz->s1);
	FREE (haz->s2);
	set_free (haz->on_cb1);
	set_free (haz->off_cb1);
	set_free (haz->on_cb2);
	set_free (haz->off_cb2);
}


/* return 1 if t1 == t2, 0 otherwise */
static int
same_hazard (t1, t2)
hazard_t *t1, *t2;
{
  if (t1->dir1 != t2->dir1 || 
	t1->dir2 != t2->dir2) {
	return 0;
  }
  if (strcmp (t1->s1, t2->s1)) {
    return 0;
  }
  if (strcmp (t1->s2, t2->s2)) {
    return 0;
  }
  if (setp_equal (t1->on_cb1, t2->on_cb1)) {
    return 0;
  }
  if (setp_equal (t1->off_cb1, t2->off_cb1)) {
    return 0;
  }
  if (setp_equal (t1->on_cb2, t2->on_cb2)) {
    return 0;
  }
  if (setp_equal (t1->off_cb2, t2->on_cb2)) {
    return 0;
  }
  return 1;
}

/* Use the algorithm and terminology of the DAC paper.
 * if R intersected with cb is empty then return
 * for each off-cube in R
 *     for each on-cube1, on-cube2 in F
 *         if d(on-cube1, top) == d(off-cube, top) - 1 and
 *             d(on-cube2, bot) == d(off-cube, bot) - 1 then
 *             store a potential hazard with the variables changing 
 *                 from on-cube1 to off-cube and from on-cube2 to off-cube
 */
static void
store_hazards_old (fanin, hazards, F, R, cb, top, bot, onset, name)
array_t *fanin, *hazards;
set_family_t *F, *R;
pset cb, top, bot;
int onset;
char *name;
{
  pset tmp, off_cube, on_cube1, on_cube2, last, last1, last2;
  set_family_t *F_1, *R_1;
  int d_top, d_bot, var1, var2, nvar, i, cnt;
  char dir1, dir2;
  hazard_t hazard, hazard1;

  if (astg_debug_flag > 2) {
    fprintf (sisout, "transition cube %s\n", pc1(cb));
    fprintf (sisout, "top %s ", pc1(top));
    fprintf (sisout, "bot %s\n", pc1(bot));
  }

  /* let us restrict F and R to the transition cube "cb" */
  R_1 = cb_intersect (cb, R);

  F_1 = cb_intersect (cb, F);
  nvar = F->sf_size / 2;
  if (astg_debug_flag > 3) {
    fprintf (sisout, "R_1:\n");
    foreach_set (R_1, last, off_cube) {
      fprintf (sisout, "%s\n", pc1(off_cube));
    }
    fprintf (sisout, "F_1:\n");
    foreach_set (F_1, last, on_cube1) {
      fprintf (sisout, "%s\n", pc1(on_cube1));
    }
  }
  tmp = set_new (F->sf_size);

  /* now for each off-set cube find:
   * 1) all on-set cubes at 1 less distance from top;
   * 2) all on-set cubes at 1 less distance from bot.
   * These cubes must be distance-1 from the offset cube. Add to the hazards
   * the variable pair.
   */
  foreach_set (R_1, last, off_cube) {
    d_top = cdist (off_cube, top);
    d_bot = cdist (off_cube, bot);
    if (d_top < 0 || d_bot < 0) {
      /* something went wrong with the enabling conditions... */
      continue;
    }
	if (astg_debug_flag > 3) {
		fprintf (sisout, "off_cube %s (%d from top %d from bot)\n", 
			pc1(off_cube), d_top, d_bot);
	}
    foreach_set (F_1, last1, on_cube1) {
      if (cdist (on_cube1, top) == d_top - 1) {
		SET (on_cube1, ONEFROMTOP);
      }
      else {
		RESET (on_cube1, ONEFROMTOP);
      }
      if (cdist (on_cube1, bot) == d_bot - 1) {
		SET (on_cube1, ONEFROMBOT);
      }
      else {
		RESET (on_cube1, ONEFROMBOT);
      }
    }
    foreach_set (F_1, last1, on_cube1) {
      if (! TESTP (on_cube1, ONEFROMTOP) ||
		  cdist01 (on_cube1, off_cube) != 1) {
		continue;
      }
      INLINEset_and (tmp, on_cube1, off_cube);
      for (i = cnt = 0; i < nvar; i++) {
		if (GETINPUT (tmp, i) == 0) {
		  dir1 = (GETINPUT (on_cube1, i) == ZERO) ? '+' : '-';
		  var1 = i;
		  cnt++;
		}
      }
      assert (cnt == 1);
	  if (astg_debug_flag > 3) {
		fprintf (sisout, "on_cube1 %s\n", pc1(on_cube1));
	  }
      foreach_set (F_1, last2, on_cube2) {
		if (on_cube1 == on_cube2 ||
			! TESTP (on_cube2, ONEFROMBOT) ||
			cdist01 (on_cube2, off_cube) != 1) {
		  continue;
		}
		INLINEset_and (tmp, on_cube2, off_cube);
		for (i = cnt = 0; i < nvar; i++) {
		  if (GETINPUT (tmp, i) == 0) {
			dir2 = (GETINPUT (on_cube2, i) == ONE) ? '+' : '-';
			var2 = i;
			cnt++;
		  }
		}
		assert (cnt == 1);
		if (astg_debug_flag > 3) {
			fprintf (sisout, "on_cube2 %s\n", pc1(on_cube2));
		}

		/* check for duplicates */
		hazard.s1 = node_long_name (array_fetch (node_t *, fanin, var1));
		hazard.s2 = node_long_name (array_fetch (node_t *, fanin, var2));
		hazard.dir1 = dir1;
		hazard.dir2 = dir2;
		hazard.on = onset;
		for (i = 0; i < array_n (hazards); i++) {
		  hazard1 = array_fetch (hazard_t, hazards, i);
		  if (same_hazard (&hazard, &hazard1)) {
			break;
		  }
		}

		if (i >= array_n (hazards)) {
		  hazard.s1 = util_strsav (hazard.s1);
		  hazard.s2 = util_strsav (hazard.s2);
		  array_insert_last (hazard_t, hazards, hazard);
		  if (astg_debug_flag > 0) {
			fprintf (sisout, 
			"potential hazard %s%c -> %s%c -> %s (%d)\n", hazard.s2, 
				hazard.dir2, hazard.s1, hazard.dir1, name, hazard.on);
		  }
		}
      }
    }
  }

  /* wrap up */
  sf_free (F_1);
  sf_free (R_1);
  set_free (tmp);
}

/* Use the algorithm and terminology of the TCAD paper.
 * if F or R intersected with cb is empty then return
 * for each off_cube1, off_cube2 in R intersected with cb
 *     for each on_cube1, on_cube2 in F intersected with cb
 *         if d(off_cube1, on_cube1) == 1 and
 *            d(off_cube2, on_cube2) == 1 then
 *             store a potential hazard with the variables changing 
 *                 from on_cube1 to off_cube1 and from off_cube2 to on_cube2
 */
static void
store_hazards (fanin, hazards, F, R, cb, top, bot, onset, name)
array_t *fanin, *hazards;
set_family_t *F, *R;
pset cb, top, bot;
int onset;
char *name;
{
  pset tmp, off_cube1, off_cube2, on_cube1, on_cube2;
  set_family_t *F_1, *R_1;
  int var1, var2, nvar, i, off_i1, off_i2, on_i1, on_i2;
  int top_to_off1, top_to_off2, bot_to_off1, bot_to_off2;
  int top_to_on1, top_to_on2, bot_to_on1, bot_to_on2;
  char dir1, dir2;
  hazard_t hazard, hazard1;

  if (astg_debug_flag > 2) {
    fprintf (sisout, "transition cube %s\n", pc1(cb));
    fprintf (sisout, "top %s ", pc1(top));
    fprintf (sisout, "bot %s\n", pc1(bot));
  }

  /* let us restrict F and R to the transition cube "cb" */
  R_1 = cb_intersect (cb, R);
  if (! R_1->count) {
    if (astg_debug_flag > 2) {
      fprintf (sisout, "R_1 empty: no need to check for hazards\n");
    }
    sf_free (R_1);
    return;
  }

  F_1 = cb_intersect (cb, F);
  if (! F_1->count) {
    if (astg_debug_flag > 2) {
      fprintf (sisout, "F_1 empty: no need to check for hazards\n");
    }
    sf_free (F_1);
    sf_free (R_1);
    return;
  }

  nvar = F->sf_size / 2;
  if (astg_debug_flag > 3) {
    fprintf (sisout, "F_1:\n");
    foreachi_set (F_1, on_i1, on_cube1) {
      fprintf (sisout, "%s\n", pc1(on_cube1));
    }
    fprintf (sisout, "R_1:\n");
    foreachi_set (R_1, off_i1, off_cube1) {
      fprintf (sisout, "%s\n", pc1(off_cube1));
    }
  }
  tmp = set_new (F->sf_size);

  foreachi_set (R_1, off_i1, off_cube1) {
	top_to_off1 = cdist (top, off_cube1);
	bot_to_off1 = cdist (bot, off_cube1);
	if (astg_debug_flag > 3) {
	  fprintf (sisout, "off_cube1 %s (%d from top %d from bot)\n", 
		pc1(off_cube1), top_to_off1, bot_to_off1);
	}

	foreachi_set (R_1, off_i2, off_cube2) {
		top_to_off2 = cdist (top, off_cube2);
		bot_to_off2 = cdist (bot, off_cube2);
		/* make sure that off_cube1 is closer to top and off_cube2 is
		 * closer to bot
		 */
		if (top_to_off2 < top_to_off1 || bot_to_off1 < bot_to_off2) {
			continue;
		}

		if (astg_debug_flag > 3) {
		  fprintf (sisout, "off_cube2 %s (%d from top %d from bot)\n", 
			pc1(off_cube2), top_to_off2, bot_to_off2);
		}

		foreachi_set (F_1, on_i1, on_cube1) {
		  /* make sure that on_cube1 is at distance 1 from off_cube1 */
		  if (cdist01 (on_cube1, off_cube1) != 1) {
			continue;
		  }
		  top_to_on1 = cdist (top, on_cube1);
		  bot_to_on1 = cdist (bot, on_cube1);

		  INLINEset_and (tmp, on_cube1, off_cube1);
		  for (i = 0; i < nvar; i++) {
			if (GETINPUT (tmp, i) == 0) {
			  dir1 = (GETINPUT (on_cube1, i) == ZERO) ? '+' : '-';
			  var1 = i;
			  break;
			}
		  }
		  if (astg_debug_flag > 3) {
		    fprintf (sisout, "on_cube1 %s (%d from top %d from bot %s%c)\n", 
				pc1(on_cube1), top_to_on1, bot_to_on1,
				node_long_name (array_fetch (node_t *, fanin, var1)), dir1);
		  }

		  foreachi_set (F_1, on_i2, on_cube2) {
			if (on_i1 == on_i2 || cdist01 (on_cube2, off_cube2) != 1) {
			  continue;
			}
		    top_to_on2 = cdist (top, on_cube2);
		    bot_to_on2 = cdist (bot, on_cube2);
			/* make sure that on_cube1 is closer to top and on_cube2 is
			 * closer to bot
			 */
			if (top_to_on2 < top_to_on1 || bot_to_on1 < bot_to_on2) {
				continue;
			}

			INLINEset_and (tmp, on_cube2, off_cube2);
			for (i = 0; i < nvar; i++) {
			  if (GETINPUT (tmp, i) == 0) {
			    dir2 = (GETINPUT (on_cube2, i) == ONE) ? '+' : '-';
				var2 = i;
				break;
			  }
			}
			if (astg_debug_flag > 3) {
			  fprintf (sisout, "on_cube2 %s (%d from top %d from bot %s%c)\n", 
				pc1(on_cube2), top_to_on2, bot_to_on2,
				node_long_name (array_fetch (node_t *, fanin, var2)), dir2);
			}

			/* check for duplicates */
			hazard.s1 = node_long_name (array_fetch (node_t *, fanin, var1));
			hazard.s2 = node_long_name (array_fetch (node_t *, fanin, var2));
			hazard.dir1 = dir1;
			hazard.dir2 = dir2;
			hazard.on_cb1 = on_cube1;
			hazard.off_cb1 = off_cube1;
			hazard.on_cb2 = on_cube2;
			hazard.off_cb2 = off_cube2;
			for (i = 0; i < array_n (hazards); i++) {
			  hazard1 = array_fetch (hazard_t, hazards, i);
			  if (same_hazard (&hazard, &hazard1)) {
				break;
			  }
			}

			if (i >= array_n (hazards)) {
			  hazard.s1 = util_strsav (hazard.s1);
			  hazard.s2 = util_strsav (hazard.s2);
			  hazard.on_cb1 = set_save (hazard.on_cb1);
			  hazard.off_cb1 = set_save (hazard.off_cb1);
			  hazard.on_cb2 = set_save (hazard.on_cb2);
			  hazard.off_cb2 = set_save (hazard.off_cb2);
			  array_insert_last (hazard_t, hazards, hazard);
			  if (astg_debug_flag > 0) {
				fprintf (sisout, 
				"potential hazard %s%c -> %s%c -> %s (%d)\n", hazard.s2, 
					hazard.dir2, hazard.s1, hazard.dir1, name, onset);
			  }
			}
		  }
		}
	}
  }

  sf_free (F_1);
  sf_free (R_1);
  set_free (tmp);
}

/* Recur from current state "state" until you find a state where the next state
 * function (always F: checking for hazards due to R we just swap the
 * pointers...) is different from the current value. At this point we can
 * check for all potential hazards. "Top" is the top state of the chain,
 * "bot" is the bottom and "cb" is the transition cube between the two.
 */
static void
find_bot (fanin, hazards, F, R, stg, sig_bit, state_code, reached, cb, top, bot, onset, use_old, name)
array_t *fanin, *hazards;
set_family_t *F, *R;
astg_graph *stg;
astg_scode sig_bit, state_code;
st_table *reached;
pset cb, top, bot;
int onset, use_old;
char *name;
{
  astg_scode enabled, to_fire, new_state;
  astg_signal * sig_p;
  int i, savebit, recursions;
  astg_generator sgen;

  enabled = astg_state_enabled (astg_find_state (stg, state_code, 0));

  i = -1;
  recursions = 0;
  astg_foreach_signal (stg, sgen, sig_p) {
	if (astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
		continue;
	}
    i++;
    to_fire = astg_state_bit (sig_p);
    if (! (to_fire & enabled)) {
      continue;
    }
    new_state = state_code ^ to_fire;
    savebit = GETINPUT (cb, i);
    PUTINPUT (cb, i, TWO);
    bit_to_cube (stg, new_state, bot);
    /* if the function value is different, stop the recursion (remember that
	 * here F can be on-set or off-set...)
	 */
    if (func_value (F, bot) == 0) {
      PUTINPUT (cb, i, savebit);
      continue;
    }

    recursions++;
    /* count it, but only recur if not yet reached... */
    if (st_lookup (reached, (char *) new_state, NIL(char *))) {
      PUTINPUT (cb, i, savebit);
      continue;
    }
    st_insert (reached, (char *) new_state, NIL(char));

    find_bot (fanin, hazards, F, R, stg, sig_bit, new_state, 
		 reached, cb, top, bot, onset, use_old, name);

    PUTINPUT (cb, i, savebit);
  }

  /* if we did not recur, then we reached the bottom of a state chain */
  if (! recursions) {
    bit_to_cube (stg, state_code, bot);
	if (use_old) {
		store_hazards_old (fanin, hazards, F, R, cb, top, bot, onset, name);
	}
	else {
		store_hazards (fanin, hazards, F, R, cb, top, bot, onset, name);
	}
  }
}

/* Find all state graph states which "are not top", that is they are next
 * states for some state with the same next state function value.
 * This is done by marking each NEXT state of the current state.
 */
static void
find_not_top (F, stg, sig_bit, state_code, reached, not_top, bot, onset)
set_family_t *F;
astg_graph *stg;
astg_scode sig_bit, state_code;
st_table *reached, *not_top;
pset bot;
int onset;
{
  astg_scode enabled, to_fire, new_state;
  astg_signal * sig_p;
  astg_generator sgen;

  enabled = astg_state_enabled (astg_find_state (stg, state_code, 0));

  astg_foreach_signal (stg, sgen, sig_p) {
	if (astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
		continue;
	}
    to_fire = astg_state_bit (sig_p);
    if (! (to_fire & enabled)) {
      continue;
    }
    new_state = state_code ^ to_fire;
    if (st_lookup (reached, (char *) new_state, NIL(char *))) {
      continue;
    }
    st_insert (reached, (char *) new_state, NIL(char));

    bit_to_cube (stg, new_state, bot);
	/* Remember that here we always call F what can be on-set or off-set... */
    if (func_value (F, bot) == 0) {
      continue;
    }

    st_insert (not_top, (char *) new_state, NIL(char));
    find_not_top (F, stg, sig_bit, new_state, reached, 
		  not_top, bot, onset);
  }
}

/* Must be called with a PO immediately after function generation.
 * It explores all states in the SG where a transition for that signal is
 * enabled. It marks all those that can be reached from other similar states, 
 * and drops them (since they are not maximally distant, or "top").
 * From "top"s we can now traverse depth-first the SG until we meet a
 * transition for the same signal again ("bot"). 
 * Now if
 * 1) the signal must be at 0 and its transition cube intersects the on-set OR
 * 2) the signal must be at 1 and its transition cube is not covered by a
 *    single cube
 * then we must record a pair of signals, so that we can later measure the
 * delays in the bwd_slow_down procedure.
 * We use either F or R as the "current on-set and off-set", according to
 * whether the next state function value for the current "top" is 1 or 0.
 * We must generate ALL PRIMES in the "current off-set", because otherwise
 * we miss some potential hazards...
 */

void
bwd_find_hazards (stg, sig_p, fanin, node_F, node_R, hazard_list, name, use_old)
astg_graph *stg;
astg_signal *sig_p;
array_t *fanin;
set_family_t *node_F, *node_R;
st_table *hazard_list;
char *name;
int use_old;
{
  astg_generator gen;
  astg_scode sig_bit, state_code, new_state, enabled;
  set_family_t *F, *R, *R_D;
  array_t * hazards;
  int onset;
  pset cb, top, bot;
  st_table *reached, *not_top;
  astg_state *state;
  char *buf;

  /* set up the variables for the recursion */
  sig_bit = astg_state_bit (sig_p);

  cb = set_new (2 * array_n (fanin));
  top = set_new (2 * array_n (fanin));
  bot = set_new (2 * array_n (fanin));
  hazards = array_alloc (hazard_t, 0);

  /* find "not top" states and mark them */
  not_top = st_init_table (st_numcmp, st_numhash);
  astg_foreach_state (stg, gen, state) {
	state_code = astg_state_code (state);
    enabled = astg_state_enabled (state);
    if (! (enabled & sig_bit)) {
      continue;
    }

    bit_to_cube (stg, state_code, cb);
    bit_to_cube (stg, state_code, bot);
    reached = st_init_table (st_numcmp, st_numhash);
    st_insert (reached, (char *) state_code, NIL(char));

    new_state = state_code ^ sig_bit;
    if (new_state & sig_bit) {
	  onset = 1;
      F = node_F;
    }
    else {
	  onset = 0;
      F = node_R;
    }

    find_not_top (F, stg, sig_bit, state_code, reached, not_top, 
		  bot, onset);

    st_free_table (reached);
  }

  /* now explore all "top" states and detect potential hazards */
  astg_foreach_state (stg, gen, state) {
	state_code = astg_state_code (state);
    enabled = astg_state_enabled (state);
    if (st_lookup (not_top, (char *) state_code, NIL(char *))) {
      continue;
    }
		
    /* initialize local recursion variables */
    bit_to_cube (stg, state_code, cb);
    bit_to_cube (stg, state_code, top);
    bit_to_cube (stg, state_code, bot);
    reached = st_init_table (st_numcmp, st_numhash);
    st_insert (reached, (char *) state_code, NIL(char));

    new_state = state_code;
    if (enabled & sig_bit) {
      new_state ^= sig_bit;
    }

    /* check if F or R is the "current on-set" */
    if (new_state & sig_bit) {
	  onset = 1;
      F = node_F;
	}
    else {
	  onset = 0;
      F = node_R;
	}
    R_D = complement (cube1list (F));
	if (use_old) {
	    R = primes_consensus (cube1list (R_D));
	    sf_free (R_D);
	}
	else {
	   R = R_D;
	}

    /* do the actual computation, finding all maximally distant
     * "bot" states from currrent "top" state_code
     */
    find_bot (fanin, hazards, F, R, stg, sig_bit, state_code, 
		 reached, cb, top, bot, onset, use_old, sig_p->name);

    /* wrap up */
    sf_free (R);
    st_free_table (reached);
  }

  /* save the hazard list under the current output signal name */
  buf = util_strsav (bwd_po_name (name));
  st_insert (hazard_list, buf, (char *) hazards);

  /* wrap up */
  set_free (cb);
  set_free (top);
  set_free (bot);
  st_free_table (not_top);
}
#endif /* SIS */
