
#ifdef SIS
#include "sis.h"
#include "astg_int.h"
#include "astg_core.h"
#include "si_int.h"

static void astg_do_min();
static st_table *link_state_minterm();
static pcube find_minterm();
static void do_consensus();
static void astg_do_reduce();
static array_t *sort_states();
static int possible_glitch();
static int remove_glitch();
static int compare_fanouts();
static int perform_reduction();
static int logic_hazard();
static int cube_empty();
static void fprint_cube();
static int constant_one();
static int fc_marking();
static node_t *find_real_po();
static void alloc_pipo_names();
static void free_pipo_names();
static void add_latch();

static array_t *real_pi_names, *real_po_names;

#define STG_INLABEL(i)  (array_fetch(char *, real_pi_names, (i)))
#define STG_OUTLABEL(i) (array_fetch(char *, real_po_names, (i)))

typedef struct state_rec {
  astg_scode state;
  astg_scode enabled; /* set of enabled transitions */
  int num_fanouts; /* number of next states */
  int index; /* index in state_array */
} state_rec;

network_t *
astg_min(network)
network_t *network;
{
  network_t *new_net;
  pPLA PLA;

  PLA = network_to_pla(network);
  if (PLA == 0) return 0;
  network_sweep(network);
  alloc_pipo_names(network, PLA);

  if (PLA->R != 0) sf_free(PLA->R);
  if (PLA->D != 0) {
    PLA->R = complement(cube2list(PLA->F,PLA->D));
  } else {
    PLA->D = new_cover(0);
    PLA->R = complement(cube1list(PLA->F));
  }

  if (!add_red) {
    so_espresso(PLA, 0);
  } else {
    astg_do_min(PLA, astg_current(network));
  }

  /* avoid hazards due to simultaneously changing signals */
  if (do_reduce) {
    astg_do_reduce(PLA, astg_current(network));
  }

  new_net = pla_to_network_single(PLA);
  new_net->stg = stg_dup(network->stg);
  new_net->astg = network->astg;
  network->astg = NIL(astg_t);
  network_set_name(new_net, network_name(network));
  add_latch(network, new_net);
  discard_pla(PLA);
  free_pipo_names();
  return new_net;
}

static void
alloc_pipo_names(network, PLA)
network_t *network;
pPLA PLA;
{
  node_t *node, *fake_pi, *real_po;
  int i;

  if (g_debug) {
    (void)fprintf(sisout, "PLA Input Names = ");
    for (i = 0; i < NUMINPUTS; i++) (void)fprintf(sisout, "%s ", INLABEL(i));
    (void)fprintf(sisout, "\nPLA Output Names = ");
    for (i = 0; i < NUMOUTPUTS; i++) (void)fprintf(sisout, "%s ", OUTLABEL(i));
    (void)fprintf(sisout, "\n");
  }

  real_pi_names = array_alloc(char *, NUMINPUTS);
  for (i = NUMINPUTS - 1; i >= 0; i--) {
    node = network_find_node(network, INLABEL(i));
    assert (node != NIL(node_t));
    if (network_is_real_pi(network, node)) {
      array_insert(char *, real_pi_names, i, node->name);
    } else {
      real_po = find_real_po(node);
      assert(real_po != NIL(node_t));
      array_insert(char *, real_pi_names, i, real_po->name);
    }
  }

  real_po_names = array_alloc(char *, NUMOUTPUTS);
  for (i = NUMOUTPUTS - 1; i >= 0; i--) {
    node = network_find_node(network, OUTLABEL(i));
    assert(node != NIL(node_t));
    if (network_is_real_po(network, node)) {
      array_insert(char *, real_po_names, i, node->name);
    } else {
      fake_pi = network_latch_end(node);
      assert (fake_pi != NIL(node_t));
      real_po = find_real_po(fake_pi);
      assert(real_po != NIL(node_t));
      array_insert(char *, real_po_names, i, real_po->name);
      /* also change the fake name to real PO name */
      FREE(OUTLABEL(i));
      OUTLABEL(i) = util_strsav(node_name(node_get_fanin(node, 0)));
    }
  }

  if (g_debug) {
    (void)fprintf(sisout, "PLA Input Names = ");
    for (i = 0; i < NUMINPUTS; i++) (void)fprintf(sisout, "%s ", INLABEL(i));
    (void)fprintf(sisout, "\nPLA Output Names = ");
    for (i = 0; i < NUMOUTPUTS; i++) (void)fprintf(sisout, "%s ", OUTLABEL(i));
    (void)fprintf(sisout, "\n");
  }
}

static void
free_pipo_names()
{
  array_free(real_pi_names);
  array_free(real_po_names);
}


static void
add_latch(old, new)
network_t *old, *new;
{
  lsGen gen;
  latch_t *latch_old, *latch_new;
  node_t *in_old, *in_new, *out_old, *out_new;
  node_t *node, *node2;

  foreach_latch(old, gen, latch_old) {
    in_old = latch_get_input(latch_old);
    in_new = network_find_node(new, in_old->name);
    if (in_new == NIL(node_t)) {
      in_new = network_find_node(new, node_name(node_get_fanin(in_old, 0)));
      assert(in_new != NIL(node_t));
    }
    out_old = latch_get_output(latch_old);
    out_new = network_find_node(new, out_old->name);
    assert(out_new != NIL(node_t));

    if (node_num_fanout(out_new) == 1) {
      node = node_get_fanout(out_new, 0);
      if (node_type(node) == INTERNAL) {
	node2 = node_get_fanout(node, 0);
	assert(node_num_fanout(node) == 1 && node_type(node2) == PRIMARY_OUTPUT);
	network_delete_node(new, node2);
      } else {
	assert(node_type(node) == PRIMARY_OUTPUT);
      }
      network_delete_node(new, node);
      network_delete_node(new, out_new);
    } else {
      network_create_latch(new, &latch_new, in_new, out_new);
      if (!network_is_real_po(new, in_new)) {
	/* hack to preserve the fake PO name - is this needed? */
	network_swap_names(new, in_new, node_get_fanin(in_new, 0));
      }
      latch_set_initial_value(latch_new, latch_get_initial_value(latch_old));
      latch_set_current_value(latch_new, latch_get_current_value(latch_old));
      latch_set_type(latch_new, latch_get_type(latch_old));
    } 
  }
}


static void
astg_do_min(PLA, stg)
pPLA PLA;
astg_graph *stg;
{
  pcover C, Fnew;
  st_table *state_minterms;
  pPLA PLA1;
  int i;

  /* sanity check */
  C = cv_intersect(PLA->F, PLA->R);
  if (C->count) fail("on-set and off-set are not disjoint\n");
  sf_free(C);

  /* associate each state with a minterm in the PLA */
  state_minterms = link_state_minterm(PLA, stg);

  /* insert necessary consensus cubes */
  do_consensus(PLA, stg, state_minterms);

  if (g_debug) {
    (void)fprintf(sisout, "Before Expansion\n");
    cprint(PLA->F);
  }

  /* code borrowed from cvrm.c in epsresso */
  /* loop for each output function */
  Fnew = new_cover(0);
  for(i = 0; i < cube.part_size[cube.output]; i++) {

    /* cofactor on the output part */
    PLA1 = new_PLA();
    PLA1->F = cof_output(PLA->F, i + cube.first_part[cube.output]);
    PLA1->R = cof_output(PLA->R, i + cube.first_part[cube.output]);
    PLA1->D = cof_output(PLA->D, i + cube.first_part[cube.output]);

    /* eliminate cubes contained in other cubes */
    PLA1->F = sf_contain(PLA1->F);

    /* expand against offset */
    PLA1->F = expand(PLA1->F, PLA1->R, 0 /* expand every variable */);

    /* intersect with the particular output part again */
    PLA1->F = uncof_output(PLA1->F, i + cube.first_part[cube.output]);

    Fnew = sf_append(Fnew, PLA1->F);
    PLA1->F = NULL;
    free_PLA(PLA1);
  }

  sf_free(PLA->F);
  PLA->F = Fnew;

  if (g_debug) {
    (void)fprintf(sisout, "After Expansion\n");
    cprint(PLA->F);
  }
  st_free_table(state_minterms);
}

static st_table *
link_state_minterm(PLA, stg)
pPLA PLA;
astg_graph *stg;
{
  st_table *table;
  astg_generator gen;
  astg_state *state_p;
  astg_scode state;
  pcube c;

/*  assert(astg_state_count(stg) == (PLA->F->count + PLA->R->count)); */

  table = st_init_table(st_numcmp, st_numhash);
  astg_foreach_state(stg, gen, state_p) {
    state = astg_state_code(state_p);
    c = find_minterm(PLA, stg, state);
    if (c != (pcube) 0) {
      assert(!st_insert(table, (char*) state, (char*) c));
    }
  }
  if (g_debug > 1) {
    astg_foreach_state(stg, gen, state_p) {
      state = astg_state_code(state_p);
      print_state(stg, state);
      if (st_lookup(table, (char*) state, (char**) &c)) {
	fprint_cube(c, PLA);
	(void)fprintf(sisout, "\n");
      } else {
	(void)fprintf(sisout, " offset minterm\n");
      }
    }
  }
  return table;
}

/* find a minterm in the PLA->F corresponding to the state 'state' */
static pcube
find_minterm(PLA, stg, state)
pPLA PLA;
astg_graph *stg;
astg_scode state;
{
  pcube last, p, c;
  register int i;
  register astg_signal *s;
  int val, match;

  c = NULL;
  foreach_set(PLA->F, last, p) {
    match = 1;
    for (i = NUMINPUTS - 1; i >= 0; i--) {
      s = astg_find_named_signal(stg, STG_INLABEL(i));
      assert(s != NIL(astg_signal));
      val = (s->state_bit & state)?1:0;
      switch(GETINPUT(p, i)) {
      case ZERO: 
	if (val) match = 0;
	break;
      case ONE: 
	if (!val) match = 0;
	break;
      default:
	match = 0;
/*
	fail("PLA should contain only minterms");
*/
	break;
      }
      if (!match) break;
    }
    if (match) {
      if (c == NULL) {
	c = set_save(p);
      } else {
	for (i = cube.first_part[cube.output]; 
	     i < cube.first_part[cube.output] + NUMOUTPUTS; i++) {
	  if (is_in_set(p, i)) set_insert(c, i);
	}
      }
    }
  }
  return c;
}

static void
do_consensus(PLA, stg, state_minterms)
pPLA PLA;
astg_graph *stg;
st_table *state_minterms;
{
  astg_generator gen, sgen;
  astg_state *state_p;
  astg_scode state, next_state, enabled;
  pcube c, c1, c2;
  astg_signal *s;

  astg_foreach_state(stg, gen, state_p) {

    state = astg_state_code(state_p);
    enabled = astg_state_enabled(state_p);

    /* if this state belongs to any output onset, do the following */
    if (st_lookup(state_minterms, (char*) state, (char**) &c1)) {

      /* for each next state */
      astg_foreach_signal(stg, sgen, s) {
	if (s->state_bit & enabled) {
	  next_state = state ^ s->state_bit;
	  if (st_lookup(state_minterms, (char*)next_state, (char**) &c2)) {

	    /* distance of 1 between 2 state minterms means that there is at least one
	     *  output which should stay at 1 during the state change
	     */
	    if (cdist(c1, c2) == 1) {
	      c = new_cube();
	      consensus(c, c1, c2);
	      PLA->F= sf_addset(PLA->F, c);
	      if (g_debug > 1) {
		(void)fprintf(sisout, "#### Adding a cube ");
		print_state(stg, state);
		print_state(stg, next_state);
		fprint_cube(c, PLA);
		(void)fprintf(sisout, "\n");
	      }
	    }
	  }
	}
      } /* end of for each next state */
    }
  }
}

static void
astg_do_reduce(PLA, stg)
pPLA PLA;
astg_graph *stg;
{
  array_t *state_array;
  int i, index, reducing;
  state_rec *state_info;
  pcube c;
  pcover F;

  if (stg == 0) return;

  /* sort states in decreasing order of number of fanout edges */
  state_array = sort_states(stg);

  F = sf_save(PLA->F);
  do {
    reducing = 0;
    for (i = 0; i < array_n(state_array); i++) {
      state_info = array_fetch(state_rec *, state_array, i);
      foreachi_set (F, index, c) {
	if (remove_glitch(index, c, state_info, state_array, stg, F, PLA)) {
	  reducing = 1;
	}
      }
    }
  } while (reducing);
  free_cover(PLA->F);
  PLA->F = F;

  for (i = 0; i < array_n(state_array); i++) {
    state_info = array_fetch(state_rec *, state_array, i);
    FREE(state_info);
  }
  array_free(state_array);
}    

/* try to remove a glitch by introducing a constant 0 literal */
static int
remove_glitch(index, c, state_info, state_array, stg, F, PLA)
int index; /* cube index */
pcube c;
state_rec *state_info;
array_t *state_array;
astg_graph *stg;
pcover F;
pPLA PLA;
{
  if (possible_glitch(c, state_info, stg, F, PLA)) {
    if (perform_reduction(index, c, state_info, state_array, stg, F, PLA)) return 1;
  }
  return 0;
}

static int
possible_glitch(c, state_info, stg, F, PLA)
pcube c;
state_rec *state_info;
astg_graph *stg;
pcover F;
pPLA PLA;
{
  astg_signal *signal;
  int i, init_val, changing;
  int rising, falling;

  rising = falling = 0;

  /* check if this cube can glitch */
  for (i = NUMINPUTS - 1; i >= 0; i--) {
    signal = astg_find_named_signal(stg, STG_INLABEL(i));
    assert(signal != NIL(astg_signal));
    if (GETINPUT(c, i) == TWO) continue;
    init_val = (signal->state_bit & state_info->state)?1:0;
    changing = (signal->state_bit & state_info->enabled)?1:0;
    if (GETINPUT(c, i) == ZERO) {
      if (!changing && init_val == 1) return 0;
      if (changing) {
	if (init_val) rising++;
	else falling++;
      }
    } else {
      if (!changing && init_val == 0) return 0;
      if (changing) {
	if (init_val) falling++;
	else rising++;
      }
    }
  }

  if (rising && falling) {
    if (g_debug) {
      (void)fprintf(sisout, "glitching cube = "); 
      fprint_cube(c, PLA);
      (void)fprintf(sisout, "\n");
      print_enabled(stg, state_info->state, state_info->enabled);
    }
    return (!constant_one(c, state_info, stg, F, PLA));
  }
  return 0;
}

/* check for the presence of constant 1 cube during the concurrent firing
 * of all the enabled transitions */
static int
constant_one(c, state_info, stg, F, PLA)
pcube c;
state_rec *state_info;
astg_graph *stg;
pcover F;
pPLA PLA;
{
  pcube c1, c2, p1, p2;
  astg_signal *s;
  int i;
  pcube last, p;
  int *const1;
  int constant_1 = 1;

  const1 = ALLOC(int, NUMOUTPUTS);
  for (i = NUMOUTPUTS - 1; i >= 0; i--) {
    const1[i] = 0;
  }

  c1 = new_cube();
  c2 = new_cube();
  p1 = new_cube();
  p2 = new_cube();

  for (i = NUMINPUTS - 1; i >= 0; i--) {
    s = astg_find_named_signal(stg, STG_INLABEL(i));
    assert(s != NIL(astg_signal));
    if (s->state_bit & state_info->state) {
      PUTINPUT(c1, i, ONE);
      if (s->state_bit & state_info->enabled) PUTINPUT(c2, i, ZERO);
      else PUTINPUT(c2, i, ONE);
    } else {
      PUTINPUT(c1, i, ZERO);
      if (s->state_bit & state_info->enabled) PUTINPUT(c2, i, ONE);
      else PUTINPUT(c2, i, ZERO);
    }
  }
  for (i = 0 ; i < NUMOUTPUTS; i++) {
    set_insert(c1, cube.first_part[cube.output] + i);
    set_insert(c2, cube.first_part[cube.output] + i);
  }
  if (g_debug) {(void)fprintf(sisout,"\t..> checking for constant 1 : c1 = (%s), c2 = (%s)\n",
			      pc1(c1), pc2(c2));}
  foreach_set(F, last, p) {
    (void) set_and(p1, p, c1);
    (void) set_and(p2, p, c2);
    for (i = NUMOUTPUTS - 1; i >= 0; i--) {
      if (GETOUTPUT(c, i) && GETOUTPUT(p1, i) && GETOUTPUT(p2, i)) {
	if (!cube_empty(p1) && !cube_empty(p2)) const1[i]++;
      }
    }
  }
  for (i = NUMOUTPUTS - 1; i >= 0; i--) {
    if (GETOUTPUT(c, i)) {
      constant_1 *= const1[i];
    }
  }
  FREE(const1);
  free_cube(c1);
  free_cube(c2);
  free_cube(p1);
  free_cube(p2);
  if (constant_1) {
    if (g_debug) (void)fprintf(sisout,"\t..> detected a constant 1 cube\n");
    return 1;
  }
  if (g_debug) (void)fprintf(sisout,"\t..< no constant 1 cube\n");
  return 0;
}

static int
perform_reduction(index, c, state_info, state_array, stg, F, PLA)
int index; /* cube index */
pcube c;
state_rec *state_info;
array_t *state_array;
astg_graph *stg;
pcover F;
pPLA PLA;
{
  pcube *FD, c1;
  pcover F_old;
  int i, j;
  astg_signal *signal;
  int *cost; /* cost of reducing each literal */
  state_rec *s;
  int best_literal, best_cost;

  F_old = sf_save(PLA->F); 
  sf_delset(F_old, index);
  FD = cube2list(F_old, PLA->D);

  cost = ALLOC(int, NUMINPUTS);
  for (i = NUMINPUTS - 1; i >= 0; i--) {
    signal = astg_find_named_signal(stg, STG_INLABEL(i));
    assert(signal != NIL(astg_signal));
    cost[i] = 0;
    if (signal->state_bit & state_info->enabled || GETINPUT(c,i) != TWO) {
      cost[i] = INFINITY;
      continue;
    }
    /* try to reduce this literal */
    if (signal->state_bit & state_info->state) {
      /* need to preserve the positive unateness of output */
      if (signal->sig_type != ASTG_INPUT_SIG) {
	PUTINPUT(c, i, ZERO);
      } else {
	if (g_debug) (void)fprintf(sisout,"\t..>< invalid reduction because of neg output %s\n", signal->name);
	cost[i] = INFINITY;
	continue;
      }
    } else {
      PUTINPUT(c, i, ONE);
    }
    if (g_debug){
      (void)fprintf(sisout, "\t..> trying reduced cube = "); 
      fprint_cube(c, PLA);
      (void)fprintf(sisout, "\n");
    }

    /* check if the reduction is valid */
    c1 = set_save(c);
    if (GETINPUT(c, i) == ONE) {
      PUTINPUT(c1, i, ZERO);
    } else {
      PUTINPUT(c1, i, ONE);
    }
    if (cube_is_covered(FD, c1)) {

      /* check if this reduction doesn't undo the previous add_redundant step */
      if (logic_hazard(F, PLA, signal, stg)) {
	cost[i] = INFINITY;
      } else {
	/* check how many additional conflicts this reduction can cause */
	for (j = array_n(state_array) - 1 ; j >= 0; j--) {
	  s = array_fetch(state_rec *, state_array, j);
	  if (j == state_info->index) continue;
	  if (possible_glitch(c, s, stg, F, PLA)) {
	    if (j < state_info->index) { 
	      /* we processed this state already--can't go back */
	      cost[i] = INFINITY;
	      break;
	    } else {
	      cost[i]++;
	    }
	  }
	}
	if (g_debug)(void)fprintf(sisout, "\t..< reduction is valid (%d additional glitches)\n", cost[i]);
      }
    } else {
      if (g_debug){
	(void)fprintf(sisout,"\t..< invalid reduction because this cube is no longer covered : ");
	fprint_cube(c1, PLA);
	(void)fprintf(sisout, "\n");
      }
      cost[i] = INFINITY;
    }
    free_cube(c1);
    PUTINPUT(c, i, TWO);
  }

  best_literal = -1;
  best_cost = INFINITY;
  for (i = NUMINPUTS - 1; i >= 0; i--) {
    if (cost[i] < best_cost) {
      best_literal = i;
      best_cost = cost[i];
    }
  }

  FREE(cost);
  free_cover(F_old);
  free_cubelist(FD);

  if (best_literal == -1) {
    (void)fprintf(sisout,"warning: no valid reduction found for cube :"); 
    fprint_cube(c, PLA);
    (void)fprintf(sisout,"\n\t");
    print_state(stg, state_info->state);
    print_enabled(stg, state_info->state, state_info->enabled);
    return 0;
  } else {
    signal = astg_find_named_signal(stg, STG_INLABEL(best_literal));
    if (signal->state_bit & state_info->state) {
      PUTINPUT(c, best_literal, ZERO);
    } else {
      PUTINPUT(c, best_literal, ONE);
    }
  }
  return 1;
}

static array_t *
sort_states(stg)
astg_graph *stg;
{
  array_t *state_array;
  astg_generator gen, _gen;
  astg_signal *signal;
  astg_state *state_p;
  astg_scode state, enabled;
  state_rec *s;
  int num_fanouts;
  array_t *fc_places;
  astg_place *p;
  astg_trans *t;
  int i;

  /* mark all the free choice places */
  if (astg_is_marked_graph(stg)) {
    fc_places = NIL(array_t);
  } else {
    fc_places = array_alloc(astg_place *, 0);
    astg_foreach_place(stg, gen, p) {
      i = 0;
      astg_foreach_output_trans(p, _gen, t) {
	i++;
      }
      if (i > 1) array_insert_last(astg_place *, fc_places, p);
    }
  }
  if (g_debug) (void)fprintf(sisout, "## %d free-choice places found\n", (fc_places!=0)?array_n(fc_places):0);

  state_array = array_alloc(state_rec *, 0);

  astg_foreach_state(stg, gen, state_p) {
    state = astg_state_code(state_p);
    enabled = astg_state_enabled(state_p);
    num_fanouts = 0;
    astg_foreach_signal(stg, _gen, signal) {
      if (enabled & signal->state_bit) num_fanouts++;
    }
    if (num_fanouts > 1) {
      /*  check if this marking involves a free-choice place */
      if ( !fc_marking(state_p, fc_places)) {
	s = ALLOC(state_rec, 1);
	s->state = state;
	s->enabled = enabled;
	s->num_fanouts = num_fanouts;
	array_insert_last(state_rec *, state_array, s);
      }
    }
  }

  array_sort(state_array, compare_fanouts);
  for (i = array_n(state_array) - 1; i >= 0; i--) {
    s = array_fetch(state_rec *, state_array, i);
    s->index = i;
  }
  if (g_debug > 1) {
    int i;
    for (i = 0; i < array_n(state_array); i++) {
      s = array_fetch(state_rec *, state_array, i);
      print_state(stg, s->state);
      print_enabled(stg, s->state, s->enabled);
      (void)fprintf(sisout, "\t(%d)\n", s->num_fanouts);
    }
  }

  if (fc_places != NIL(array_t)) array_free(fc_places);
  return state_array;
}

/* place states with high fanouts near the beginning */
static int
compare_fanouts(s1, s2)
char *s1, *s2;
{
  int diff;
  state_rec *r1 = *(state_rec **) s1;
  state_rec *r2 = *(state_rec **) s2;

  assert(r1 != 0 && r2 != 0);
  diff = r1->num_fanouts - r2->num_fanouts;
  if (diff > 0) return -1;
  if (diff < 0) return 1;
  return 0;
}

/* check if this state marking involves any of the free-choice places */
static int
fc_marking(state_p, fc_places)
astg_state *state_p;
array_t *fc_places;
{
  astg_generator gen;
  astg_marking *marking;
  int i;
  astg_place *p;

  if (fc_places == NIL(array_t)) return 0;
  astg_foreach_marking(state_p, gen, marking) {
    for (i = array_n(fc_places) - 1; i >= 0; i--) {
      p = array_fetch(astg_place *, fc_places, i);
      if (astg_get_marked(marking, p)) return 1;
    }
  }
  return 0;
}

/* check if there is a logic hazard in cover F due to the firing of
 * this signal 'signal'
 */
static int
logic_hazard(F, PLA, signal, stg)
pcover F;
pPLA PLA;
astg_signal *signal;
astg_graph *stg;
{
  register int i;
  astg_scode state, enabled;
  astg_generator gen;
  astg_state *state_p;
  astg_signal *s;
  pcube c1, c2, p1, p2, p, last;
  int *rising, *falling, *const1;
  int p1_zero, p2_zero, hazard;

  rising = ALLOC(int, NUMOUTPUTS);
  falling = ALLOC(int, NUMOUTPUTS);
  const1 = ALLOC(int, NUMOUTPUTS);
  c1 = new_cube();
  c2 = new_cube();
  p1 = new_cube();
  p2 = new_cube();
  hazard = 0;

  astg_foreach_state(stg, gen, state_p) {
    state = astg_state_code(state_p);
    enabled = astg_state_enabled(state_p);

    if (signal->state_bit & enabled) {
      if (g_debug) {
	(void)fprintf(sisout,"\t....>> checking for logic hazard : "); 
	print_state(stg, state); 
	(void)fprintf(sisout,"\t");
	print_enabled(stg, state, enabled);
      }
      for (i = NUMINPUTS - 1; i >= 0; i--) {
	s = astg_find_named_signal(stg, STG_INLABEL(i));
	assert(s != NIL(astg_signal));
	if (s->state_bit & state) {
	  PUTINPUT(c1, i, ONE);
	  if (s != signal) PUTINPUT(c2, i, ONE);
	  else PUTINPUT(c2, i, ZERO);
	} else {
	  PUTINPUT(c1, i, ZERO);
	  if (s != signal) PUTINPUT(c2, i, ZERO);
	  else PUTINPUT(c2, i, ONE);
	}
      }
      for (i = 0 ; i < NUMOUTPUTS; i++) {
	set_insert(c1, cube.first_part[cube.output] + i);
	set_insert(c2, cube.first_part[cube.output] + i);
      }
      if (g_debug) {(void)fprintf(sisout,"\t....>> c1 = (%s), c2 = (%s) [changing signal = %s]\n",
				  pc1(c1), pc2(c2), signal->name);}
      for (i = NUMOUTPUTS - 1; i >= 0; i--) {
	rising[i] = falling[i] = const1[i] = 0;
      }
      foreach_set(F, last, p) {
	(void) set_and(p1, p, c1);
	(void) set_and(p2, p, c2);
	for (i = NUMOUTPUTS - 1; i >= 0; i--) {
	  if (GETOUTPUT(p1, i) && GETOUTPUT(p2, i)) {
	    p1_zero = cube_empty(p1);
	    p2_zero = cube_empty(p2);
	    if (p1_zero && !p2_zero) rising[i]++;
	    if (!p1_zero && p2_zero) falling[i]++;
	    if (!p1_zero && !p2_zero) const1[i]++;
	  }
	}
      }
      for (i = NUMOUTPUTS - 1; i >= 0; i--) {
	if (rising[i] && falling[i] && !const1[i]) {
	  if (g_debug) (void)fprintf(sisout,"\t....<< found logic hazard in %s (r=%d,f=%d,1=%d)\n",
				     STG_OUTLABEL(i), rising[i], falling[i], const1[i]);
	  hazard = 1;
	  break;
	}
      }
      if (hazard) {
	goto hazard_end;
      }
      if (g_debug) (void)fprintf(sisout,"\t....<< no logic hazard in %s\n", STG_OUTLABEL(i));
    }
  }

 hazard_end:
  FREE(rising);
  FREE(falling);
  FREE(const1);
  free_cube(c1);
  free_cube(c2);
  free_cube(p1);
  free_cube(p2);
  if (hazard) return 1;
  return 0;
}

/* should be in espresso.h ? */
static int
cube_empty(c)
pcube c;
{
  register int i;
  
  for (i = NUMINPUTS - 1; i >= 0; i--) {
    if (GETINPUT(c, i) == 0) return 1;
  }
  return 0;
}

/* find a PO fanned out by this node */
static node_t *
find_real_po(node)
node_t *node;
{
  lsGen gen;
  node_t *fanout, *real_po; 
  
  real_po = NIL(node_t);
  foreach_fanout(node, gen, fanout) {
    if (node_type(fanout) == PRIMARY_OUTPUT) {
      assert(real_po == NIL(node_t)); /* assumes that there's a single PO */
      real_po = fanout;
    }
  }
  return real_po;
}

/* --- debug routines --- */
/* print present state */
void
print_state(stg, state)
astg_graph *stg;
astg_scode state;
{
  astg_signal *s;
  astg_generator agen;

  (void)fprintf(sisout, "state %d : (", state);
  astg_foreach_signal(stg, agen, s) {
    (void)fprintf(sisout,"%s=%d ", s->name, (s->state_bit & state)?1:0);
  }
  (void)fprintf(sisout, ")\n");
}

/* print the set of enabled transitions in the present state */
void
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

void
print_state_graph(stg)
astg_graph *stg;
{
  astg_scode state, enabled;
  astg_generator gen;
  astg_state *state_p;

  if (stg == NIL(astg_graph)) return;
  if (astg_token_flow (stg, /* print error messages */ ASTG_TRUE) == ASTG_OK) {
    (void)fprintf(sisout, "STATE GRAPH\n");
    astg_foreach_state(stg, gen, state_p) {
      state = astg_state_code(state_p);
      enabled = astg_state_enabled(state_p);
      print_state(stg, state);
      print_enabled(stg, state, enabled);
    }
    (void)fprintf(sisout, "\n");
  }
}

static void
fprint_cube(c, PLA)
pcube c;
pPLA PLA;
{
  int i;
  
  for (i = NUMINPUTS - 1; i >= 0; i--) {
    switch(GETINPUT(c,i)) {
    case TWO: break;
    case ONE: (void)fprintf(sisout,"%s ",STG_INLABEL(i));
      break;
    case ZERO: (void)fprintf(sisout,"!%s ", STG_INLABEL(i));
      break;
    case 0: (void)fprintf(sisout, "?%s ", STG_INLABEL(i));
      break;
    default:
      break;
    }
  }
  (void)fprintf(sisout,"|| ");
  for (i = NUMOUTPUTS - 1; i >= 0; i--) {
    if (GETOUTPUT(c,i)) {
      (void)fprintf(sisout, "%s ", STG_OUTLABEL(i));
    }
  }
}
#endif /* SIS */
