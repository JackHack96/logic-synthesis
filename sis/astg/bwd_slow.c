/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/astg/bwd_slow.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:59 $
 *
 */
#ifdef SIS
/* Routines that insert inverter pairs where a potential hazard can actually 
 * occur. 
 * The static symbol table slowed_amounts keeps an information on how much
 * each signal is slowed.
 */
#include "sis.h"
#include "astg_int.h"
#include "astg_core.h"
#include "bwd_int.h"
#include <math.h>

#define XOR(a,b)    (((a)&&!(b))||(!(a)&&(b)))
/* tests on floating point equality */
#define EPS 1.e-24
#define EQ(a,b) (fabs((a)-(b)) < EPS)

static delay_time_t delay_simulate ();

/* Write back into the external delay table the MINIMUM delays PI->PO as updated
 * after the synthesis/hazard removal procedure.
 * Inform the user if the table was actually changed, so that iteration is
 * required.
 * The table is organized as a double-level symbol table, indexed by signal
 * names in the STG form (i.e. the true PO form)
 * The first level is indexed by the name of the signal from which the delay
 * is requested. Each entry at the first level is a symbol table indexed by 
 * the name of the signal to which the delay is requested. Each entry at the
 * second level is a delay record.
 */
void
bwd_external_delay_update (network, external_del, min_delay_factor, silent)
network_t * network;
st_table *external_del;
double min_delay_factor;
int silent;
{
  st_table *to_table;
  node_t *pi, *po, *node;
  delay_time_t delay;
  min_del_t new_delay, *old_delay;
  lsGen pigen, pogen;
  int updated, found;

  if (external_del == NIL(st_table)) {
    return;
  }
  updated = 0;
  foreach_primary_input (network, pigen, pi) {
    foreach_primary_output (network, pogen, po) {
      if (network_is_real_po(network, po)) {
	/* skip real PO's connected directly to the PI */
		node = node_get_fanin (po, 0);
		if (node_type (node) == PRIMARY_INPUT) {
			continue;
		}
      }
      delay = delay_simulate (network, pi, po, /* is_max */ 0, 
				  min_delay_factor);
      /* INFINITY needs not to be stored */
      if ((delay.rise > (INFINITY / 100.0)) &&
			(delay.fall > (INFINITY / 100.0))) {
			continue;
      }
	  new_delay.rise = delay;
	  new_delay.fall = delay;

      /* DO NOT update only if already present AND smaller */
      if ((found = st_lookup (external_del, bwd_po_name (node_long_name (pi)), 
		     (char **) &to_table)) &&
		  (found = st_lookup (to_table, bwd_po_name (node_long_name (po)), 
		     (char **) &old_delay)) &&
		  (old_delay->rise.rise < new_delay.rise.rise ||
		   EQ(old_delay->rise.rise, new_delay.rise.rise)) &&
		  (old_delay->rise.fall < new_delay.rise.fall ||
		   EQ(old_delay->rise.fall, new_delay.rise.fall)) &&
		  (old_delay->fall.rise < new_delay.fall.rise ||
		   EQ(old_delay->fall.rise, new_delay.fall.rise)) &&
		  (old_delay->fall.fall < new_delay.fall.fall ||
		   EQ(old_delay->fall.fall, new_delay.fall.fall))) {
		continue;
      }

      updated = 1;
      if (astg_debug_flag > 1) {
		if (found) {
			fprintf (sisout, 
	 "Updating the delay %s -> %s ++ %f->%f +- %f->%f -+ %f->%f -- %f->%f\n",
			 node_long_name (pi), node_long_name (po), 
			 old_delay->rise.rise, new_delay.rise.rise, 
			 old_delay->rise.fall, new_delay.rise.fall, 
			 old_delay->fall.rise, new_delay.fall.rise, 
			 old_delay->fall.fall, new_delay.fall.fall);
		}
		else {
			fprintf (sisout, 
			 "Inserting the delay %s -> %s %f %f %f %f\n",
			 node_long_name (pi), node_long_name (po), 
			 new_delay.rise.rise, 
			 new_delay.rise.fall, 
			 new_delay.fall.rise, 
			 new_delay.fall.fall);
		}
      }

      if (! st_lookup (external_del, bwd_po_name (node_long_name (pi)), 
		(char **) &to_table)) {
			to_table = st_init_table (strcmp, st_strhash);
			st_insert (external_del, 
				util_strsav (bwd_po_name (node_long_name (pi))), 
			   (char *) to_table);
      }
      if (! st_lookup (to_table, bwd_po_name (node_long_name (po)), (char **)
		       &old_delay)) {
			old_delay = ALLOC (min_del_t, 1);
			st_insert (to_table, 
				util_strsav (bwd_po_name (node_long_name (po))), 
				(char *) old_delay);
      }
      *old_delay = new_delay;
    }
  }
  if (! silent && updated) {
    fprintf (siserr, "Warning: the delay table has been updated\n");
  }
}

/* Perform a delay trace to the network (eventually this must become a delay
 * simulation, as the name suggests) by setting:
 * 1) the arrival time of "from" to 0.
 * 2) all other arrival times to INFINITY or -INFINITY (depending on whether we
 *    check for max or min delay).
 * We return the maximum or minimum arrival time at "to" (depending on
 * "is_max").
 */
static delay_time_t
delay_simulate (network, from, to, is_max, min_delay_factor)
network_t *network;
node_t *from, *to;
int is_max;
double min_delay_factor;
{
  lsGen gen;
  node_t *pi, *node;
  delay_time_t delay1;

  foreach_primary_input (network, gen, pi) {
    if (is_max) {
      delay_set_parameter (pi, DELAY_ARRIVAL_RISE, (double) -INFINITY);
      delay_set_parameter (pi, DELAY_ARRIVAL_FALL, (double) -INFINITY);
    }
    else {
      delay_set_parameter (pi, DELAY_ARRIVAL_RISE, (double) INFINITY);
      delay_set_parameter (pi, DELAY_ARRIVAL_FALL, (double) INFINITY);
    }
  }
  delay1.rise = (double) -INFINITY;
  delay1.fall = (double) -INFINITY;

  delay_set_parameter (from, DELAY_ARRIVAL_RISE, (double) 0);
  delay_set_parameter (from, DELAY_ARRIVAL_FALL, (double) 0);
  delay_set_parameter (to, DELAY_OUTPUT_LOAD, (double) 1.0);

  if (is_max) {
    assert (delay_trace (network, DELAY_MODEL_LIBRARY));
  }
  else {
    assert (bwd_min_delay_trace (network, DELAY_MODEL_LIBRARY));
  }

  if (astg_debug_flag > 3) {
    foreach_node (network, gen, node) {
      delay1 = delay_arrival_time (node);
      fprintf (sisout, "%s arrival %s: %2.2f %2.2f\n", 
	       is_max ? "max" : "min",
	       node_long_name (node), delay1.rise, delay1.fall);
    }
  }

  delay1 = delay_arrival_time (to);
  if (! is_max) {
    /* we should really support min-max delays ... */
    delay1.rise *= min_delay_factor;
    delay1.fall *= min_delay_factor;
  }
  if (astg_debug_flag > 2) {
    fprintf (sisout, "%s delay %s->%s: %2.2f %2.2f\n", 
	     is_max ? "max" : "min",
	     node_long_name (from), node_long_name(to), delay1.rise, delay1.fall);
  }
  return delay1;
}

/* Retrieve or compute the values of "d3" for all PI-PO pairs. 
 * Each delay is stored as a double into a square matrix, where the first index
 * runs across signals "from" which the delay is computed, and the second index
 * runs across signals "to" which the delay is computed (we have "nd3s" signals
 * of each kind, so the matrix is nd3s x nd3s).
 * The "pio" symbol table is indexed by primary INPUT names, and contains for 
 * each name the index (valid both as "from" and "to" index) in the "d3s" 
 * matrix.
 * We use, in order:
 * 1) the delay_simulate routine, for "to" signals we are synthesizing now.
 * 2) the external_del symbol table, for signal pairs that were synthesized
 *    previously.
 * 3) the default delay.
 * 4) the Floyd-Warshall all pair shortest path algorithm (if "shortest_path"
 *    is true).
 */
static void
fill_d3s (network, d3s, nd3s, pio, external_del, shortest_path, default_del, min_delay_factor)
network_t *network;
double **d3s;
int nd3s;
st_table *pio, *external_del;
int shortest_path;
double default_del, min_delay_factor;
{
  int from, to, iter;
  lsGen gen, gen1;
  node_t *po, *pi, *pi1, *node;
  delay_time_t delay;
  min_del_t *delay_p;
  char *from_name, *to_name;
  st_generator *pigen, *pogen;
  st_table *to_table;
  double new_del;

  /* initialize the array */
  for (from = 0; from < nd3s; from++) {
    for (to = 0; to < nd3s; to++) {
      d3s[from][to] = INFINITY;
    }
  }

  /* first try to get delays for signals we are synthesizing */
  foreach_primary_input (network, gen, pi) {
    assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi)), &from));
    foreach_primary_output (network, gen1, po) {
      if (network_is_real_po(network, po)) {
	/* skip real PO's connected directly to the PI */
		node = node_get_fanin (po, 0);
		if (node_type (node) == PRIMARY_INPUT) {
			continue;
		}
      }
      assert (st_lookup_int (pio, bwd_po_name (node_long_name (po)), &to));
      delay = delay_simulate (network, pi, po, /* is_max */ 0, 
			      min_delay_factor);
      d3s[from][to] = MIN (delay.rise, delay.fall);
      if (d3s[from][to] < 0) {
		/* this means no path inside our network between "from" and "to" */
		d3s[from][to] = INFINITY;
      }
    }
  }

  if (external_del != NIL(st_table)) {
    /* print out debugging information */
    if (astg_debug_flag > 2) {
      fprintf (sisout, "before external delay:\n");
      foreach_primary_input (network, gen, pi) {
		assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi)), &from));
		foreach_primary_input (network, gen1, pi1) {
		  assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi1)), &to));
		  fprintf (sisout, "d3: %s->%s : %2.2f\n", 
			   node_long_name (pi), node_long_name (pi1), 
			   d3s[from][to]);
		}
      }
    }

    /* now get as much information as possible from the external delay table */
    st_foreach_item (external_del, pigen, (char **) &from_name,
		     (char **) &to_table) {
      assert (st_lookup_int (pio, from_name, &from));
      st_foreach_item (to_table, pogen, (char **) &to_name,
		       (char **) &delay_p) {
		assert (st_lookup_int (pio, to_name, &to));
		new_del = MIN (MIN (delay_p->rise.rise, delay_p->rise.fall),
			MIN (delay_p->fall.rise, delay_p->fall.fall));
		if (d3s[from][to] > new_del) {
		  d3s[from][to] = new_del;
		}
      }
    }
  }

  if (shortest_path) {
    /* print out debugging information */
    if (astg_debug_flag > 2) {
      fprintf (sisout, "before shortest path:\n");
      foreach_primary_input (network, gen, pi) {
		assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi)), &from));
		foreach_primary_input (network, gen1, pi1) {
		  assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi1)), &to));
		  fprintf (sisout, "d3: %s->%s : %2.2f\n", 
			   node_long_name (pi), node_long_name (pi1), 
			   d3s[from][to]);
		}
      }
    }
    /* use Floyd-Warshall for all pairs shortest path */
    for (iter = 0; iter < nd3s; iter++) {
      for (from = 0; from < nd3s; from++) {
		for (to = 0; to < nd3s; to++) {
		  if (d3s[from][to] > 
			  (d3s[from][iter] + d3s[iter][to])) {
			d3s[from][to] =
			  d3s[from][iter] + d3s[iter][to];
		  }
		}
      }
    }
  }

  /* print out debugging information */
  if (astg_debug_flag > 2) {
    fprintf (sisout, "before default delay:\n");
    foreach_primary_input (network, gen, pi) {
      assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi)), &from));
      foreach_primary_input (network, gen1, pi1) {
		assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi1)), &to));
		fprintf (sisout, "d3: %s->%s : %2.2f\n", 
			 node_long_name (pi), node_long_name (pi1), 
			 d3s[from][to]);
      }
    }
  }

  /* now set all still unspecified delays to the default value */
  for (from = 0; from < nd3s; from++) {
    for (to = 0; to < nd3s; to++) {
      /* I don't like this test, but... */
      if (d3s[from][to] > (INFINITY / 100.0)) {
		d3s[from][to] = default_del;
      }
    }
  }

  /* print out debugging information */
  if (astg_debug_flag > 2) {
    fprintf (sisout, "final d3 table:\n");
    foreach_primary_input (network, gen, pi) {
      assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi)), &from));
      foreach_primary_input (network, gen1, pi1) {
		assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi1)), &to));
		fprintf (sisout, "d3: %s->%s : %2.2f\n", 
			 node_long_name (pi), node_long_name (pi1), 
			 d3s[from][to]);
      }
    }
  }
}

/* For each "i" signal in the network (hazard.s1, pi1) we check that the effect
 * of its "j" input (hazard.s2, pi2) has finished propagating in the circuit 
 * for "t" (po).
 * This amount to check that, if
 * d1 = min delay (pi1 -> po)
 * d2 = max delay (pi2 -> po) 
 * d3 = min delay (pi2 -> pi1) 
 * then we have a hazard if d2 is less than d3 plus d1, using the tolerance
 * "tol". In case of problems, we slow down the worst such pi1 for each po, and
 * loop.
 * The external delays (i.e. from output signals to output signals) are
 * looked up in the "external_del" table. Otherwise "default_del" is used.
 * Moreover an all-pair shortest path algorithm is used (if "shortest_path" is
 * 1) to infer missing.sh delay pairs (i.e. if a -> b is 5 and b -> c is 3, then
 * a -> c is assumed to be 8...).
 * When computing MINIMUM delays, they are MULTIPLIED by min_delay_factor
 * (until we will be able to provide somtheing better for min/max delays).
 * If do_slow is 0, then only the external delays are updated as appropriate.
 */

void
bwd_slow_down (network, hazard_list, slowed_amounts, external_del, tol, default_del, min_delay_factor, shortest_path, iterate, do_slow)
network_t *network;
st_table *hazard_list, *slowed_amounts, *external_del;
double tol, default_del, min_delay_factor;
int shortest_path, iterate, do_slow;
{
  char *buf;
  node_t *po, *pi, *pi1, *pi2, *inv1, *inv2, *fanout, *pi1_sav, *pi2_sav;
  node_t *node;
  char dir1, dir2;
  lib_gate_t *inv, *lib_get_default_inverter();
  array_t *hazards, *old_hazards;
  hazard_t hazard, hazard1;
  delay_time_t delay1, delay2, slowed_delay;
  double d1, d2, d3, max_diff, *slowed, zero, curr_slow;
  double **d3s;
  int i, j, failed, from, to, nd3s;
  lsGen gen, gen1;
  st_table *pio, *to_table;
  st_generator *pigen, *pogen;
  char *from_name, *to_name;
  double *delay_p;

  inv = lib_get_default_inverter();
  zero = 0;

  /* initialize the PI/PO name -> index table and the d3 matrix */
  pio = st_init_table (strcmp, st_strhash);
  nd3s = 0;
  foreach_primary_input (network, gen, pi) {
    st_insert (pio, util_strsav (bwd_po_name (node_long_name (pi))), 
		(char *) nd3s++);
  }
  foreach_primary_output (network, gen, po) {
	if (! st_is_member (pio, bwd_po_name (node_long_name (po)))) {
		st_insert (pio, util_strsav (bwd_po_name (node_long_name (po))), 
			(char *) nd3s++);
	}
  }
  if (external_del != NIL(st_table)) {
    st_foreach_item (external_del, pigen, (char **) &from_name,
		     (char **) &to_table) {
      if (! st_lookup (pio, from_name, NIL (char *))) {
		st_insert (pio, util_strsav (from_name), (char *) nd3s++);
      }
      st_foreach_item (to_table, pogen, (char **) &to_name,
		       (char **) &delay_p) {
		if (! st_lookup (pio, to_name, NIL (char *))) {
		  st_insert (pio, util_strsav (to_name), (char *) nd3s++);
		}
      }
    }
  }

  d3s = ALLOC (double *, nd3s);
  for (i = 0; i < nd3s; i++) {
    d3s[i] = ALLOC (double, nd3s);
  }

  fill_d3s (network, d3s, nd3s, pio, external_del, shortest_path, 
	    default_del, min_delay_factor);

  if (do_slow) {
  /* now slow down each PO in the network */
  foreach_primary_output (network, gen, po) {
    if (network_is_real_po(network, po)) {
	/* skip real PO's connected directly to the PI */
		node = node_get_fanin (po, 0);
		if (node_type (node) == PRIMARY_INPUT) {
			continue;
		}
    }
    buf = bwd_po_name (node_long_name (po));
    assert (st_lookup (hazard_list, buf, (char **) &old_hazards));
	/* avoid uninteresting ones (here we are less strict than when we
	 * store them, since we do not care about rising or falling delays...)
	 */
	hazards = array_alloc (hazard_t, 0);
	if (astg_debug_flag > 0) {
		fprintf (sisout, "Hazards checked for %s:\n", buf);
	}
	for (i = 0; i < array_n (old_hazards); i++) {
		hazard = array_fetch (hazard_t, old_hazards, i);
#if 0
		/* use them all, for error reporting */
		for (j = 0; j < array_n (hazards); j++) {
			hazard1 = array_fetch (hazard_t, hazards, j);
			if (! strcmp (hazard.s1, hazard1.s1) && 
				! strcmp (hazard.s2, hazard1.s2)) {
				break;
			}
		}
		if (j >= array_n (hazards)) {
#endif
			array_insert_last (hazard_t, hazards, hazard);
			if (astg_debug_flag > 0) {
				fprintf (sisout, "  %s %s\n", hazard.s2, hazard.s1);
			}
#if 0
		}
#endif
	}

    /* loop until we do not have hazards at this PO */
    do {
      /* max_diff contains the amount to slow down pi1_sav, that is the
       * pi1 which has the currently highest d2 - (d3 + d1) 
       */
      max_diff = -INFINITY;
      pi1_sav = NIL(node_t);
      pi2_sav = NIL(node_t);
      failed = 0;

      for (i = 0; i < array_n (hazards); i++) {
		/* now check each potential hazard in turn */
		hazard = array_fetch (hazard_t, hazards, i);

		/* we must subtract the slowing time of pi1 because we
		 * should be measuring d1 AFTER the added delay
		 */
		pi1 = network_find_node (network, hazard.s1);
		if (! st_lookup (slowed_amounts, bwd_po_name (node_long_name (pi1)), 
			(char **) &slowed)) {
		  slowed = & zero;
		}
		delay1 = delay_simulate (network, pi1, po, /* is_max */ 0,
					 min_delay_factor);
		d1 = MIN (delay1.rise, delay1.fall) - (*slowed);
		if (d1 < 0) {
		  d1 = 0;
		}
		if (astg_debug_flag > 1) {
		  fprintf (sisout, "d1: %s->%s : %2.2f\n", 
			   node_long_name (pi1), node_long_name (po), d1);
		}

		/* we must subtract the slowing time of pi2 because we
		 * should be measuring d2 AFTER the added delay
		 */
		pi2 = network_find_node (network, hazard.s2);
		if (! st_lookup (slowed_amounts, bwd_po_name (node_long_name (pi2)), 
			(char **) &slowed)) {
		  slowed = & zero;
		}
		delay2 = delay_simulate (network, pi2, po, /* is_max */ 1,
					 min_delay_factor);
		d2 = MAX (delay2.rise, delay2.fall) - (*slowed);
		if (d2 < 0) {
		  d2 = 0;
		}
		if (astg_debug_flag > 1) {
		  fprintf (sisout, "d2: %s->%s : %2.2f\n", 
			   node_long_name (pi2), node_long_name (po), d2);
		}

		/* we must add the slowing time of pi1 because we
		 * should be measuring d3 AFTER the added delay
		 */
		if (! st_lookup (slowed_amounts, bwd_po_name (node_long_name (pi1)), 
			(char **) &slowed)) {
		  slowed = & zero;
		}
		assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi2)), &from));
		assert (st_lookup_int (pio, bwd_po_name (node_long_name (pi1)), &to));
		d3 = d3s[from][to] + (*slowed);
		if (d3 < 0) {
		  d3 = 0;
		}
		if (astg_debug_flag > 1) {
		  fprintf (sisout, "d3: %s->%s : %2.2f\n", 
			   node_long_name (pi2), node_long_name (pi1), d3);
		}

		if (pi1_sav == NIL(node_t) || (d2 - (d1 + d3)) > max_diff) {
			  pi1_sav = pi1;
			  pi2_sav = pi2;
			  dir1 = hazard.dir1;
			  dir2 = hazard.dir2;
			  max_diff = d2 - (d1 + d3);
			  if (astg_debug_flag > 0) {
				fprintf (sisout, "d2 > d1 + d3 by %2.2f\n", max_diff);
			  }
		}
      }

      /* now check if we have a hazard to remove */
      if (max_diff + tol > 0) {
		failed = 1;
		fprintf (sisout, 
			"Hazard %s%c -> %s%c -> %s by %2.2f (slowing %s)\n", 
			 node_long_name (pi2_sav), dir2, node_long_name (pi1_sav), 
			 dir1, node_long_name (po), (max_diff + tol),
			 node_long_name (pi1_sav));
		curr_slow = 0;

		do {
			inv1 = node_literal (pi1_sav, 0);
			network_add_node (network, inv1);
			inv2 = node_literal (inv1, 0);
			network_add_node (network, inv2);

			buf = lib_gate_pin_name (inv, 0, 1);
			assert (buf != NIL(char));
			assert (lib_set_gate (inv1, inv, &buf, &pi1_sav, 1));
			assert (lib_set_gate (inv2, inv, &buf, &inv1, 1));

			foreach_fanout (pi1_sav, gen1, fanout) {
			  if (fanout != inv1) {
				assert (node_patch_fanin (fanout, pi1_sav, inv2));
			  }
			}
			slowed_delay = delay_simulate (network, pi1_sav, inv2, 
							   /* is_max */ 0, min_delay_factor);
			if (! st_lookup (slowed_amounts, 
				bwd_po_name (node_long_name(pi1_sav)), (char **) &slowed)) {
			  slowed = ALLOC (double, 1);
			  *slowed = 0;
			  st_insert (slowed_amounts, 
					 util_strsav (bwd_po_name (node_long_name(pi1_sav))),
					 (char *) slowed);
			}
			*slowed += MIN (slowed_delay.rise, slowed_delay.fall);
			curr_slow += MIN (slowed_delay.rise, slowed_delay.fall);
		} while (! iterate && curr_slow < max_diff + tol);
      }
    } while (failed);

    array_free (hazards);
  }
  }

  /* now save the new delay information (if required) and wrap up */
  bwd_external_delay_update (network, external_del, min_delay_factor,
	  /*silent*/ 0);

  for (i = 0; i < nd3s; i++) {
    FREE (d3s[i]);
  }
  FREE (d3s);
  st_foreach_item (pio, pigen, &buf, NIL(char *)) {
    FREE (buf);
  }
  st_free_table (pio);

  foreach_primary_input (network, gen, pi) {
    delay_set_parameter (pi, DELAY_ARRIVAL_RISE, (double) 0);
    delay_set_parameter (pi, DELAY_ARRIVAL_FALL, (double) 0);
  }
}
#endif /* SIS */
