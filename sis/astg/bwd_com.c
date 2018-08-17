
#ifdef SIS
/* bwd_com_astg.c -- SIS command interface to bounded wire delay STG
 * synthesis sub-package
 */

#include "sis.h"
#include "astg_int.h"
#include "astg_core.h"
#include "bwd_int.h"
#include "si_int.h"

static void
bwd_alloc_daemon (astg, dummy)
astg_graph *astg, *dummy;
{
    bwd_t *bwd;

    bwd = ALLOC (bwd_t, 1);
    bwd->hazard_list = st_init_table (strcmp, st_strhash);
    bwd->slowed_amounts = st_init_table (strcmp, st_strhash);
    bwd->pi_names = array_alloc (char *, 0);

    BWD_SET (astg, bwd);
}

static void
bwd_dup_daemon (new_astg, old_astg)
astg_graph *new_astg, *old_astg;
{
    st_table *old_hazard_list, *old_slowed_amounts;
    st_table *new_hazard_list, *new_slowed_amounts;
    st_generator *gen;
    char *n;
    int i;
    array_t *old_hazards, *new_hazards, *old_pi_names, *new_pi_names;
    hazard_t old_hazard, new_hazard;
    double *old_slowed, *new_slowed;
    bwd_t *new_bwd, *old_bwd;

    old_bwd = BWD_GET(old_astg);
    new_bwd = BWD_GET(new_astg);

    /* get it from the new ASTG, since it is different !! */
    new_bwd->change_count = astg_change_count (new_astg);

    new_hazard_list = new_bwd->hazard_list;
    old_hazard_list = old_bwd->hazard_list;

    if (old_hazard_list != NIL(st_table)) {
        st_foreach_item (old_hazard_list, gen, &n, (char **) &old_hazards) {
            if (n != NIL(char) && old_hazards != NIL(array_t)) {
                new_hazards = array_alloc (hazard_t, 0);
                for (i = 0; i < array_n (old_hazards); i++) {
                    old_hazard = array_fetch (hazard_t, old_hazards, i);
                    bwd_dup_hazard (&new_hazard, &old_hazard);
                    array_insert_last (hazard_t, new_hazards, new_hazard);
                }
                st_insert (new_hazard_list, util_strsav (n),
                    (char *) new_hazards);
            }
        }
    }

    new_slowed_amounts = new_bwd->slowed_amounts;
    old_slowed_amounts = old_bwd->slowed_amounts;

    if (old_slowed_amounts != NIL(st_table)) {
        st_foreach_item (old_slowed_amounts, gen, &n, (char **) &old_slowed) {
            if (n != NIL(char) && old_slowed != NIL(double)) {
                new_slowed = ALLOC (double, 1);
                *new_slowed = *old_slowed;
                st_insert (new_slowed_amounts, util_strsav (n),
                    (char *) new_slowed);
            }
        }
    }

    old_pi_names = old_bwd->pi_names;
    new_pi_names = new_bwd->pi_names;
    if (old_pi_names != NIL(array_t)) {
        for (i = 0; i < array_n (old_pi_names); i++) {
            array_insert_last (char *, new_pi_names,
                util_strsav (array_fetch (char *, old_pi_names, i)));
        }
    }
}

static void
bwd_free_daemon (astg, dummy)
astg_graph *astg, *dummy;
{
    bwd_t *bwd;
    st_table *hazard_list, *slowed_amounts;
    st_generator *gen;
    char *n;
    int i;
    array_t *hazards, *pi_names;
    hazard_t hazard;
    double *slowed;

    bwd = BWD_GET (astg);
    hazard_list = bwd->hazard_list;
    slowed_amounts = bwd->slowed_amounts;
    pi_names = bwd->pi_names;

    if (hazard_list != NIL(st_table)) {
        st_foreach_item (hazard_list, gen, &n, (char **) &hazards) {
            if (n != NIL(char)) {
                FREE(n);
            }
            if (hazards != NIL(array_t)) {
                for (i = 0; i < array_n (hazards); i++) {
                    hazard = array_fetch (hazard_t, hazards, i);
                    bwd_free_hazard (&hazard);
                }
                array_free (hazards);
            }
        }
        st_free_table (hazard_list);
    }

    if (slowed_amounts != NIL(st_table)) {
        st_foreach_item (slowed_amounts, gen, &n, (char **) &slowed) {
            if (n != NIL(char)) {
                FREE (n);
            }
            if (slowed != NIL(double)) {
                FREE (slowed);
            }
        }
        st_free_table (slowed_amounts);
    }
    if (pi_names != NIL (array_t)) {
        for (i = 0; i < array_n (pi_names); i++) {
            n = array_fetch (char *, pi_names, i);
            FREE (n);
        }
        array_free (pi_names);
    }

    FREE(bwd);

    BWD_SET (astg, NIL(bwd_t));
}

static void
bwd_invalid_daemon (astg, dummy)
astg_graph *astg, *dummy;
{
    bwd_free_daemon (astg, dummy);
    bwd_alloc_daemon (astg, dummy);
}

/* global initialization routine */
void
bwd_cmds ()
{
    com_add_command ("astg_to_f", com_bwd_astg_to_f, 1);
    com_add_command ("astg_to_stg", com_bwd_astg_to_stg, 1);
    com_add_command ("astg_slow", com_bwd_astg_slow_down, 1);
    com_add_command ("astg_stg_scr", com_bwd_astg_stg_scr, 1);
    com_add_command ("stg_to_astg", com_bwd_stg_to_astg, 1);
    com_add_command ("astg_state_min", com_bwd_astg_state_minimize, 1);
    com_add_command ("astg_add_state", com_bwd_astg_add_state, 1);
    com_add_command ("astg_encode", com_bwd_astg_encode, 1);
    com_add_command ("_stg_scc", com_stg_scc, 0);
    com_add_command ("_write_sg", com_bwd_astg_write_sg, 0);

    astg_register_daemon (ASTG_DAEMON_ALLOC, bwd_alloc_daemon);
    astg_register_daemon (ASTG_DAEMON_DUP, bwd_dup_daemon);
    astg_register_daemon (ASTG_DAEMON_INVALID, bwd_invalid_daemon);
    astg_register_daemon (ASTG_DAEMON_FREE, bwd_free_daemon);
}

/* Produce on-set and offset covers for the next state function of each
 * output signal in the STG, and generate the list of potential hazards.
 * Also each next-state function is decomposed into two primary oututs, one for
 * the set input and one for the reset input of SR flip-flops. Primary outputs
 * are added so that optimization does not throw these away...
 * If we want to make S and R disjoint, then this is done also for purely
 * combinational output signals (by creating a flip-flop with equation
 * q = s q + s), and we make the set and reset functions disjoint, by
 * using the equation q = m q + s q + s (redundant, but we use scan FF's
 * anyway).
 */

static char *astg_to_f_usage[] = {
    "usage: %s [-v debug_level] [-f] [-o] [-l] [-d] [-h] [PO_name ...]",
    "    Generates a function for each output signal directly from the STG",
    "    -f: force even if CSC problem (vertices assigned to on-set)",
    "    -o: use the old DAC algorithm (default: the more recent one)",
    "    -r: keep PI's without fanout (default: remove them)",
    "    -l: does not create the asynchronous latches (for debugging)",
    "    -d: the functions of S and R are made disjoint",
    "    -h: do not check potential hazards",
    "    PO_name: ensure mapping of the specified signals into SR flip-flops",
    NULL
};

int
com_bwd_astg_to_f (network_p,argc, argv)
network_t **network_p;
int    argc;
char **argv;
{
  int c, result, flag, index, use_old, i, find_hazards;
  int keep_red, use_s_r, disjoint, do_latch, force;
  astg_retval status;
  astg_graph *astg;
  astg_signal *sig_p;
  astg_generator sgen;
  astg_scode state;
  array_t *fanin, *to_del, *pi_names, *node_vec;
  node_t *node;
  st_table *hazard_list, *s_r_names;
  char *name;
  latch_t *l;

  result = 0;
  flag = 0;
  use_old = 0;
  keep_red = 0;
  use_s_r = 0;
  disjoint = 0;
  force = 0;
  do_latch = 1;
  find_hazards = 1;
  util_getopt_reset ();
  while ((c=util_getopt(argc,argv,"forv:dlh")) != EOF) {
    switch (c) {
    case 'f': force = 1; break;
    case 'o': use_old = 1; break;
    case 'r': keep_red = 1; break;
    case 'l': do_latch = 0; break;
    case 'd': disjoint = 1; break;
    case 'h': find_hazards = 0; break;
    case 'v': flag = atoi (util_optarg); break;
      default : result = 1; break;
    }
  }
  s_r_names = st_init_table (strcmp, st_strhash);
  for (; util_optind < argc; util_optind++) {
    st_insert (s_r_names, argv[util_optind], NIL(char));
  }
  if (result) {
    astg_usage (astg_to_f_usage,argv[0]);
    astg_debug_flag = 0;
    return result;
  }

  if (flag > 100) {
    astg_debug_flag = flag;
  }

  if ((astg=astg_current (*network_p)) == NULL) {
    astg_debug_flag = 0;
    return 1;
  }

  /* look for the initial marking, and if not defined find one (it might not
   * always work..)
   */
  if (! astg->has_marking) {
    if (! astg_one_sm_token(astg)) {
      fprintf (siserr, "cannot find the initial state\n");
      fprintf (siserr, "please define one in the STG with .marking\n");
      astg_debug_flag = 0;
      return 1;
    }
  }

  /* derive the state graph by token flow */
  status = astg_initial_state (astg, &state);
  if (status != ASTG_OK && status != ASTG_NOT_USC) {
    fprintf (siserr, "inconsistency during token flow\n");
    astg_sel_show (astg);
    astg_debug_flag = 0;
    return 1;
  }

  astg_debug_flag = flag;
  name_mode = LONG_NAME_MODE;
  BWD_GET(astg)->change_count = astg_change_count(astg);
  if (find_hazards) {
      hazard_list = BWD_GET(astg)->hazard_list;
  }
  else {
    hazard_list = NIL(st_table);
  }
  network_set_name (*network_p, util_strsav (astg_name (astg)));

  /* remove all nodes from the old network */
  to_del = network_dfs_from_input (*network_p);
  for (i = 0; i < array_n (to_del); i++) {
    node = array_fetch (node_t *, to_del, i);
    l = latch_from_node (node);
    if (l != NIL(latch_t)) {
        network_delete_latch(*network_p, l);
    }
    network_delete_node (*network_p, node);
  }
  array_free (to_del);

  /* create the PI's and PO's of the network */
  fanin = array_alloc (node_t *, 0);
  pi_names = BWD_GET(astg)->pi_names;
  if (astg_debug_flag > 1) {
    fprintf (sisout, "inputs ");
  }
  astg_foreach_signal (astg, sgen, sig_p) {
    if (astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
        continue;
    }

    if (astg_signal_type (sig_p) == ASTG_INPUT_SIG) {
        name = bwd_po_name (sig_p->name);
    }
    else {
        name = bwd_fake_pi_name (sig_p->name);
    }

    node = node_alloc ();
    network_add_primary_input(*network_p, node);
    network_change_node_name (*network_p, node, util_strsav (name));

    if (astg_debug_flag > 1) {
      fprintf (sisout, "%s (%x) ", node_long_name (node),
           astg_state_bit (sig_p));
    }
    array_insert_last (node_t *, fanin, node);
    array_insert_last (char *, pi_names, util_strsav (bwd_po_name (name)));
  }
  if (astg_debug_flag > 1) {
    fprintf (sisout, "\n");
  }

  /* now generate the next-state for each non-input signal */
  index = 0;
  astg_foreach_signal (astg, sgen, sig_p) {
    if (astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
        continue;
    }
    if (astg_signal_type (sig_p) == ASTG_OUTPUT_SIG ||
        astg_signal_type (sig_p) == ASTG_INTERNAL_SIG) {
      if (astg_debug_flag > 1) {
        fprintf (sisout,
         "generating function for signal %s\n", sig_p->name);
      }
      use_s_r = st_is_member (s_r_names, sig_p->name) ||
        st_is_member (s_r_names, "*");
      bwd_astg_to_f (*network_p, astg, sig_p, fanin, state,
             hazard_list, index, use_old, use_s_r, disjoint, force);
    }
    index++;
  }

  if (do_latch) {
      /* create the latches */
      bwd_astg_latch (*network_p, ASYNCH, keep_red);
  }

  astg_debug_flag = 0;

  if (result) {
    astg_usage (astg_to_f_usage,argv[0]);
  }

  astg_debug_flag = 0;
  st_free_table (s_r_names);
  return result;
}

/* Transform an AFSM into an AFSM with the single cube restriction
 */
static char *astg_stg_scr_usage[] = {
    "usage: %s [-v debug_level]",
    "    Transforms an STG into an STG with the Single Cube Restriction",
    NULL
};

int
com_bwd_astg_stg_scr (network_p,argc, argv)
network_t **network_p;
int    argc;
char **argv;
{
  int c, result;
  graph_t *stg;
  lsGen gen;
  vertex_t *state;
  astg_graph *astg;
  network_t *new_net;

  astg_debug_flag = 0;
  result = 0;
  util_getopt_reset ();
  while ((c=util_getopt(argc,argv,"v:")) != EOF) {
    switch (c) {
    case 'v': astg_debug_flag = atoi (util_optarg); break;
      default : result = 1; break;
    }
  }

  if (result || network_stg(*network_p) == NIL(graph_t)) {
    astg_usage (astg_stg_scr_usage,argv[0]);
    astg_debug_flag = 0;
    return 1;
  }

  stg = bwd_stg_scr (network_stg(*network_p));
  if (stg == NIL(graph_t)) {
    return 1;
  }
  network_set_stg (*network_p, stg);

  astg_debug_flag = 0;
  return result;
}

/* Transform an AFSM into an STG. Experimental.
 */
static char *stg_to_astg_usage[] = {
    "usage: %s [-v debug_level]",
    "    Generates an STG from a synchronous state transition graph.",
    NULL
};

int
com_bwd_stg_to_astg (network_p,argc, argv)
network_t **network_p;
int    argc;
char **argv;
{
  int c, result;
  astg_graph *astg;

  astg_debug_flag = 0;
  result = 0;
  util_getopt_reset ();
  while ((c=util_getopt(argc,argv,"v:")) != EOF) {
    switch (c) {
    case 'v': astg_debug_flag = atoi (util_optarg); break;
      default : result = 1; break;
    }
  }

  if (result || network_stg(*network_p) == NIL(graph_t)) {
    astg_usage (stg_to_astg_usage,argv[0]);
    astg_debug_flag = 0;
    return 1;
  }

  astg = bwd_stg_to_astg(network_stg(*network_p), *network_p);
  if (astg == NIL(astg_graph)) {
    return 1;
  }

  astg_set_current (network_p, astg, ASTG_TRUE);

  astg_debug_flag = 0;
  return result;
}

/* Transform an STG into a state graph through token flow. Experimental.
 */
static char *astg_to_stg_usage[] = {
    "usage: %s [-v debug_level] [-o] [-i] [-m]",
    "    Generates a synchronous state transition graph from the STG.",
    "    -o: the OUTPUT signals are NOT used as INPUTS of the graph",
    "    -m: perform a pre-minimization of the STG (for state encoding)",
    NULL
};

int
com_bwd_astg_to_stg (network_p,argc, argv)
network_t **network_p;
int    argc;
char **argv;
{
  int c, result, use_output, pre_minimize, d, i, len, j;
  astg_retval status;
  graph_t *stg;
  astg_graph *astg;
  astg_generator sgen;
  astg_scode code;
  astg_signal *sig_p;
  array_t *inputs, *outputs, *to_del, *components, *component;
  node_t *node;
  latch_t *l;
  char *name, *str;
  vertex_t *state;

  d = astg_debug_flag = 0;
  result = 0;
  use_output = 1;
  pre_minimize = 0;
  util_getopt_reset ();
  while ((c=util_getopt(argc,argv,"v:om")) != EOF) {
    switch (c) {
    case 'v': d = atoi (util_optarg); break;
    case 'o': use_output = 0; break;
    case 'm': pre_minimize = 1; break;
      default : result = 1; break;
    }
  }

  if (result) {
    astg_usage (astg_to_stg_usage,argv[0]);
    astg_debug_flag = 0;
    return result;
  }
  if ((astg=astg_current (*network_p)) == NULL) {
    astg_debug_flag = 0;
    return 1;
  }

  if (! astg->has_marking) {
    if (! astg_one_sm_token(astg)) {
      fprintf (siserr, "cannot find the initial state\n");
      fprintf (siserr, "please define one in the STG with .marking\n");
      return 1;
    }
  }

  /* derive the state graph by token flow */
  status = astg_initial_state (astg, &code);
  if (status != ASTG_OK && status != ASTG_NOT_USC) {
    fprintf (siserr, "inconsistency during token flow\n");
    astg_sel_show (astg);
    return 1;
  }

  astg_debug_flag = d;
  /* store the input and output names */
  inputs = array_alloc (char *, 0);
  if (astg_debug_flag > 0) {
    fprintf (sisout, "inputs ");
  }
  astg_foreach_signal (astg, sgen, sig_p) {
    if (astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
        continue;
    }
    if (use_output || astg_signal_type (sig_p) == ASTG_INPUT_SIG) {
        if (astg_signal_type (sig_p) == ASTG_INPUT_SIG) {
          name = util_strsav (bwd_po_name (sig_p->name));
        }
        else {
          name = util_strsav (bwd_fake_pi_name (sig_p->name));
        }
        if (astg_debug_flag > 0) {
          fprintf (sisout, "%s ", name);
        }
        array_insert_last (char *, inputs, name);
    }
  }
  if (astg_debug_flag > 0) {
    fprintf (sisout, "\n");
  }
  outputs = array_alloc (char *, 0);
  if (astg_debug_flag > 0) {
    fprintf (sisout, "outputs ");
  }
  astg_foreach_signal (astg, sgen, sig_p) {
    if (astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
        continue;
    }
    if (astg_signal_type (sig_p) == ASTG_OUTPUT_SIG ||
        astg_signal_type (sig_p) == ASTG_INTERNAL_SIG) {
      name = util_strsav (bwd_fake_po_name (sig_p->name));
      if (astg_debug_flag > 0) {
        fprintf (sisout, "%s ", name);
      }
      array_insert_last (char *, outputs, name);
    }
  }
  if (astg_debug_flag > 0) {
    fprintf (sisout, "\n");
  }
  /* produce the state transition graph */
  stg = network_stg(*network_p);
  if (stg != NIL(graph_t)) {
    stg_free (stg);
  }
  stg = bwd_astg_to_stg (astg, code, use_output, pre_minimize);
  if (stg == NIL(graph_t)) {
    return 1;
  }
  if (!stg_check(stg)) {
    fprintf (siserr, "stg check failed\n");
    astg_debug_flag = 0;
    return 1;
  }
  components = graph_scc (stg);
  if (array_n (components) > 1) {
    fprintf (siserr,
    "internal error: the state transition graph is not strongly connected\n");
    if (astg_debug_flag > 0) {
        for (i = 0; i < array_n (components); i++) {
            component = array_fetch (array_t *, components, i);
            fprintf (sisout, "%d:", i);
            len = 4;
            for (j = 0; j < array_n (component); j++) {
                state = array_fetch (vertex_t *, component, j);
                str = stg_get_state_name (state);
                len += strlen (str) + 1;
                if (len > 80) {
                    fprintf (sisout, "\n   ");
                    len = strlen (str) + 4;
                }
                fprintf (sisout, " %s", str);
            }
            fprintf (sisout, "\n");
        }
    }
    array_free (components);
    return 1;
  }
  array_free (components);
  /* remove all nodes from the old network */
  to_del = network_dfs_from_input (*network_p);
  for (i = 0; i < array_n (to_del); i++) {
    node = array_fetch (node_t *, to_del, i);
    l = latch_from_node (node);
    if (l != NIL(latch_t)) {
        network_delete_latch(*network_p, l);
    }
    network_delete_node (*network_p, node);
  }
  array_free (to_del);

  network_set_stg (*network_p, stg);

  stg_set_names (stg, inputs, /* input names */ 1);
  stg_set_names (stg, outputs, /* output names */ 0);

  astg_debug_flag = 0;
  return result;
}

/* Check if potential hazards actually are possible given the mapped network
 * delays, and if so remove them by selectively slowing down STG signals.
 * Currently we insert inverter pairs at the network INPUTS, while it would be
 * better in principle to insert them at the outputs. But the delay tracing
 * would then become much clumsier, and the final result is the same anyway...
 * Parameter of the procedure:
 * - External delay file: if the other subcircuits have been synthesized
 *   separately, this file gives the relevant minimum input->output delays.
 *   The file is also updated with our own delays.
 * - Tolerance: if the difference between paths is greater than this amount,
 *   then we have a hazard (> 0 means stricter, more conservative checking,
 *   < 0 means overly optimistic...).
 * - Default minimum delay for external signals (if they are not found in the
 *   external delay file). Usually the delay of an inverter or of an inverter
 *   pair, but 0 by default (overly pessimistic).
 * - By default an all-pair shortest path is used to infer unspecified delays
 *   from those measured or given. If this flag is set, then this algorithm
 *   is disabled (resulting in a pessimistic result, on average).
 */

static char *slow_down_usage[] = {
    "usage: %s [[-f|-F] filename] [-d default_delay] [-s] [-i] [-u] [-l]",
    "          [-m min_delay_factor] [-t tolerance]",
    "    Removes hazards, by adding inverter pairs",
    "    -f: specifies the filename containing the minimum delays",
    "        between external signals",
    "    -F: same as above but update the file with the new estimates",
    "    -d: specifies the default mimimum delay between external signals,",
    "        when not available from the file (default 0)",
    "    -s: do not use the shorthest path algorithm to compute d3",
    "    -i: iterate the slowing down process (slower)",
    "    -u: do not remove hazards, only update the external delay file",
    "    -m: multiply all minimum delays by the specified factor",
    "        (0 < min_delay_factor <= 1)",
    "    -t: specifies the time tolerance for hazard checking (default 0)",
    "    -l: use the linear programming solver for hazard removal",
    "    -L: use the LINDO linear programming solver for hazard removal",
    "    -b: use bounding to reduce the linear program calls",
    NULL
};

int
com_bwd_astg_slow_down (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
  int c, update, do_slow, do_bound, lindo, lp, i;
  double tol, default_del, min_delay_factor;
  int result = 0;
  int shortest_path, iterate;
  FILE *file;
  char *filename, *name;
  st_table *external_del, *to_table, *hazard_list, *slowed_amounts, *pi_index;
  array_t *pi_names;
  char pi_buf[80], po_buf[80], buf[256];
  char *pi_name, *po_name;
  min_del_t delay, *delay_p;
  st_generator *pigen, *pogen;
  astg_graph *astg;
  lsGen gen;
  node_t *node;

  tol = 0;
  default_del = 0;
  min_delay_factor = 1;
  shortest_path = 1;
  iterate = 0;
  update = 0;
  do_slow = 1;
  lp = 0;
  lindo = 0;
  do_bound = 0;
  filename = NIL(char);
  external_del = NIL(st_table);
  util_getopt_reset ();
  while ((c=util_getopt(argc,argv,"bv:d:f:F:im:st:ulL")) != EOF) {
    switch (c) {
    case 'b': do_bound = 1; break;
    case 'l': lp = 1; break;
    case 'L': lp = 1; lindo = 1; break;
    case 'v': astg_debug_flag = atoi (util_optarg); break;
    case 'd': default_del = atof (util_optarg); break;
    case 'i': iterate = 1; break;
    case 'm': min_delay_factor = atof (util_optarg); break;
    case 's': shortest_path = 0; break;
    case 't': tol = atof (util_optarg); break;
    case 'f': filename = util_optarg; break;
    case 'u': do_slow = 0; break;
    case 'F': filename = util_optarg; update = 1; break;
    default: result = 1;
    }
  }
  if (min_delay_factor <= 0 || min_delay_factor > 1) {
    result = 1;
  }
  if (result) {
    astg_usage (slow_down_usage,argv[0]);
    astg_debug_flag = 0;
    return result;
  }

  if ((astg=astg_current (*network_p)) == NULL) {
    astg_debug_flag = 0;
    return 1;
  }

  if (astg_change_count(astg) != BWD_GET(astg)->change_count) {
    fprintf (siserr, "the ASTG was changed since the last astg_to_f call\n");
    astg_debug_flag = 0;
    return 1;
  }

  hazard_list = BWD_GET(astg)->hazard_list;

  /* open the external delay file and read it in the symbol table (see
   * bwd_slow.c for details on the table organization)
   */
  if (filename != NIL(char)) {
    external_del = st_init_table (strcmp, st_strhash);

    file = com_open_file (filename,"r",NULL,0);
    if (file == NULL) {
      fprintf (siserr,"Warning: cannot read external delay file '%s'\n",
           filename);
    }
    else {
      while (fgets (buf, sizeof(buf), file) != NULL) {
        if (buf[0] == '#') {
            continue;
        }
          if (sscanf (buf, "%s %s %lf %lf %lf %lf\n",
            pi_buf, po_buf, &delay.rise.rise, &delay.rise.fall,
            &delay.fall.rise, &delay.fall.fall) != 6) {
            fprintf (siserr, "Error: incorrect delay file line:\n");
            fprintf (siserr, "%s", buf);
            continue;
        }
        if (delay.rise.rise < 0) {
            delay.rise.rise = INFINITY;
        }
        if (delay.rise.fall < 0) {
            delay.rise.fall = INFINITY;
        }
        if (delay.fall.rise < 0) {
            delay.fall.rise = INFINITY;
        }
        if (delay.fall.fall < 0) {
            delay.fall.fall = INFINITY;
        }
        if (! st_lookup (external_del, pi_buf, (char **) &to_table)) {
          to_table = st_init_table (strcmp, st_strhash);
          st_insert (external_del, util_strsav (pi_buf),
                 (char *) to_table);
        }
        if (st_lookup (to_table, po_buf, (char **) &delay_p)) {
          fprintf (siserr,
               "Warning: duplicate delay table entry: %s -> %s\n",
               pi_buf, po_buf);
          continue;
        }
        if (astg_debug_flag > 1) {
          fprintf (sisout, "Reading delay %s -> %s ++ %f +- %f -+ %f -- %f\n",
               pi_buf, po_buf, delay.rise.rise, delay.rise.fall,
               delay.fall.rise, delay.fall.fall);
        }
        delay_p = ALLOC (min_del_t, 1);
        *delay_p = delay;
        st_insert (to_table, util_strsav (po_buf), (char *) delay_p);
      }
      fclose (file);
    }
  }

  /* do an initial delay trace (for delay computations) */
  if (! delay_trace (*network_p, DELAY_MODEL_LIBRARY)) {
    fprintf (siserr, "%s\n", error_string ());
    astg_debug_flag = 0;
    return 1;
  }
  if (lp) {
      if (external_del == NIL(st_table)) {
        external_del = st_init_table (strcmp, st_strhash);
      }
      pi_index = st_init_table (strcmp, st_strhash);
      pi_names = BWD_GET(astg)->pi_names;
      if (astg_debug_flag > 1) {
        fprintf (sisout, "signal order:");
      }
      for (i = 0; i < array_n (pi_names); i++) {
        name = array_fetch (char *, pi_names, i);
        st_insert (pi_index, name, (char *) i);
        if (astg_debug_flag > 1) {
          fprintf (sisout, " %s", name);
        }
      }
      if (astg_debug_flag > 1) {
        fprintf (sisout, "\n");
      }
      bwd_lp_slow (astg, *network_p, hazard_list, external_del,
        pi_index, tol, default_del, min_delay_factor, do_bound, lindo);
  }
  else {
      slowed_amounts = BWD_GET(astg)->slowed_amounts;
      /* remove hazards */
      bwd_slow_down (*network_p, hazard_list, slowed_amounts, external_del, tol,
             default_del, min_delay_factor, shortest_path, iterate, do_slow);
  }

  /* reset arrival times on PIs */
  foreach_primary_input (*network_p, gen, node) {
        delay_set_parameter (node, DELAY_ARRIVAL_RISE, (double) -1);
        delay_set_parameter (node, DELAY_ARRIVAL_FALL, (double) -1);
  }

  /* write back the (possibly updated) external delay table */
  if (update && filename != NIL(char)) {
    file = com_open_file (filename,"w",NULL,0);
    if (file == NULL) {
      fprintf (siserr,"Warning: cannot write external delay file '%s'\n",
           filename);
    }
    else {
      fprintf (file, "# signal1 signal2 ++del +-del -+del --del\n");
      st_foreach_item (external_del, pigen, (char **) &pi_name,
               (char **) &to_table) {
        st_foreach_item (to_table, pogen, (char **) &po_name,
                 (char **) &delay_p) {
          fprintf (file, "%s %s %lf %lf %lf %lf\n",
                pi_name, po_name, delay_p->rise.rise, delay_p->rise.fall,
                delay_p->fall.rise, delay_p->fall.fall);
          if (astg_debug_flag > 1) {
            fprintf (sisout, "Writing delay %s -> %s ++ %f +- %f -+ %f -- %f\n",
                 pi_name, po_name, delay_p->rise.rise, delay_p->rise.fall,
                 delay_p->fall.rise, delay_p->fall.fall);
          }
        }
      }
      fclose (file);
    }
  }
  if (external_del != NIL(st_table)) {
      st_foreach_item (external_del, pigen, (char **) &pi_name,
               (char **) &to_table) {
        st_foreach_item (to_table, pogen, (char **) &po_name,
                 (char **) &delay_p) {
          FREE (delay_p);
          FREE (po_name);
        }
        FREE (pi_name);
        st_free_table (to_table);
      }
      st_free_table (external_del);
  }
  astg_debug_flag = 0;
  return result;
}


/* Minimize the flow table corresponding to an ASTG, then find a minimum
 * set of signals to distinguish among non-equivalent state classes.
 * The STG associated with the network can the be encoded using Tracey's
 * algorithm.
 * The equivalent state information (as found by the state minimizer) is stored
 * in the "equiv_states" structure.
 */
static char *astg_state_minimize_usage[] = {
    "usage: %s [-v debug_level] [-p minimized_file] [-c \"command\"]",
    "          [-b|-B] [-g|-G] [-u] [-m|-M] [-o #] [-f signal_cost_file]",
    "    Minimizes the flow table associated with the STG",
    "    -c: use the specified command for flow table minimization",
    "        (default: \"sred -c\")",
    "    -B: use BDDs to find the set of partitioning signals",
    "        (default: iteratively improve the set)",
    "    -b: use BDDs to find the initial set of partitioning signals",
    "        (default: all the signals if removing, no signals if adding)",
    "    -M: use minimum cover to find the set of partitioning signals",
    "        (default: iteratively improve the set)",
    "    -o: if -M is specified, defines the binate covering option",
    "    -m: use minimum cover to find the initial set of partitioning signals",
    "        (default: all the signals if removing, no signals if adding)",
    "    -g: use a greedy algorithm to improve the initial set",
    "        (default: use exhaustive branch and bound)",
    "    -G: use a VERY greedy algorithm to improve the initial set",
    "        (default: use exhaustive branch and bound)",
    "    -u: add signals to an initially empty set",
    "        (default: remove signals from the initial set)",
    "    -p: do not call the minimizer, just read the specified minimized file",
    NULL
};

int
com_bwd_astg_state_minimize (network_p,argc, argv)
network_t **network_p;
int    argc;
char **argv;
{
  int c, result, cost;
  int use_mincov, greedy, go_down, mincov_option, use_mdd, premin;
  graph_t *stg, *red_stg;
  astg_graph *astg;
  equiv_t *equiv_states;
  FILE *to, *from;
  char command[1024];
  char infile[1024];
  char outfile[1024];
  st_table *sig_cost;
  FILE *sig_file;
  st_generator *stgen;
  char buf[80], *str;
  int tempfd;

  strcpy(infile, "/usr/tmp/astg_in.XXXXXX");
  tempfd = mkstemp(infile);
  if(tempfd == -1) {
    fprintf(siserr, "error: cannot open temporary file\n");
    return 1;
  }
  close(tempfd);
  strcpy(outfile, "/usr/tmp/astg_out.XXXXXX");
  tempfd = mkstemp(outfile);
  if(tempfd == -1) {
    fprintf(siserr, "error: cannot open temporary file\n");
    unlink(infile);
    return 1;
  }
  close(tempfd);
  (void) strcpy (command, "sred -c ");

  astg_debug_flag = 0;
  result = 0;
  use_mincov = 0;
  greedy = 0;
  go_down = 1;
  mincov_option = 4096;
  use_mdd = 0;
  premin = 0;
  sig_file = NIL(FILE);
  util_getopt_reset ();
  while ((c=util_getopt(argc,argv,"bBgGmMuf:o:v:c:p:")) != EOF) {
    switch (c) {
    case 'f':
        sig_file = fopen (util_optarg, "r");
        if (sig_file == NULL) {
            perror (util_optarg);
            result = 1;
        }
        break;
    case 'c':
        strcpy(command, util_optarg);
        break;
    case 'p': premin = 1; strcpy (outfile, util_optarg); break;
    case 'b': use_mdd = 1; break;
    case 'B': use_mdd = 2; break;
    case 'u': go_down = 0; break;
    case 'g': greedy = 1; break;
    case 'G': greedy = 2; break;
    case 'm': use_mincov = 1; break;
    case 'M': use_mincov = 2; break;
    case 'o': mincov_option = atoi (util_optarg); break;
    case 'v': astg_debug_flag = atoi (util_optarg); break;
      default : result = 1; break;
    }
  }
  if ((greedy || use_mincov) && ! go_down) {
    result = 1;
  }

  if (result) {
    astg_usage (astg_state_minimize_usage,argv[0]);
    astg_debug_flag = 0;
    return result;
  }

  strcat(command, " < ");
  strcat(command, infile);
  strcat(command, " > ");
  strcat(command, outfile);

  if ((astg=astg_current (*network_p)) == NULL) {
    astg_debug_flag = 0;
    return 1;
  }

  stg = network_stg(*network_p);
  if (stg == NIL(graph_t)) {
    fprintf (siserr, "No State Transition Graph (use astg_to_stg)\n");
    astg_debug_flag = 0;
    return 1;
  }

  if ((to = fopen(infile, "w")) == NULL) {
    (void) fprintf(siserr, "error: can not open temporary file - %s\n", infile);
    (void) unlink(infile);
    (void) unlink(outfile);
    return 1;
  }

  if (!write_kiss(to, network_stg(*network_p))) {
    (void) fprintf(siserr, "error in write_kiss\n");
    (void) fclose(to);
    (void) unlink(infile);
    (void) unlink(outfile);
    return 1;
  }
  (void) fclose(to);

  if (premin) {
    (void) fprintf(siserr,
        "please issue\n%s\nand then\nkill -CONT %d\n", command, getpid ());
    kill (getpid (), SIGSTOP);
    (void) fprintf(siserr, "continuing\n");
  }
  else {
      if (system(command)) {
        (void) fprintf(siserr,
            "state minimization program returned non-zero exit status\n%s\n",
            command);
        (void) unlink(infile);
        (void) unlink(outfile);
        return 1;
      }
  }

  if ((from = fopen(outfile, "r")) == NULL) {
    (void) fprintf(siserr, "error: can not open output file - %s\n", outfile);
    (void) unlink(infile);
    (void) unlink(outfile);
    return 1;
  }

  equiv_states = bwd_read_equiv_states (from, stg);
  if (equiv_states == NIL(equiv_t)) {
    (void) fprintf(siserr, "Error in state minimization program output\n");
    (void) fclose(from);
    (void) unlink(infile);
    (void) unlink(outfile);
    return 1;
  }

  (void) fclose(from);
  (void) unlink(infile);
  if (! premin) {
      (void) unlink(outfile);
  }

  sig_cost = NIL(st_table);
  if (sig_file) {
    sig_cost = st_init_table (strcmp, st_strhash);
    while (fscanf (sig_file, "%s %d", buf, &cost) > 0) {
        st_insert (sig_cost, util_strsav (buf), (char *) cost);
    }
    fclose (sig_file);
  }

  red_stg = bwd_state_minimize (astg, equiv_states, use_mincov, greedy,
      go_down, mincov_option, use_mdd, sig_cost);

  if (sig_cost != NIL(st_table)) {
    st_foreach_item (sig_cost, stgen, (char **) &str, NIL(char*)) {
        FREE (str);
    }
    st_free_table (sig_cost);
  }

  if (red_stg == NIL(graph_t)) {
    astg_debug_flag = 0;
    return 1;
  }

  stg_free (stg);

  if (!stg_check(red_stg)) {
    fprintf (siserr, "stg check failed\n");
    astg_debug_flag = 0;
    return 1;
  }
  network_set_stg (*network_p, red_stg);

  astg_debug_flag = 0;
  return result;
}

/* Add state variables to distinguish between non-equivalent markings
 */
static char *astg_add_state_usage[] = {
    "usage: %s [-v debug_level] [-m]",
    "    Insert state variables to guarantee Complete State Coding",
    "    -m: do not preserve the original marking",
    NULL
};

int
com_bwd_astg_add_state (network_p,argc, argv)
network_t **network_p;
int    argc;
char **argv;
{
  int c, result, cost;
  int restore_marking;
  graph_t *stg;
  astg_graph *astg;

  astg_debug_flag = 0;
  result = 0;
  restore_marking = 1;
  util_getopt_reset ();
  while ((c=util_getopt(argc,argv,"v:mf")) != EOF) {
    switch (c) {
    case 'm': restore_marking = 0; break;
    case 'v': astg_debug_flag = atoi (util_optarg); break;
      default : result = 1; break;
    }
  }

  if (result) {
    astg_usage (astg_add_state_usage,argv[0]);
    astg_debug_flag = 0;
    return result;
  }
  if ((astg=astg_current (*network_p)) == NULL) {
    astg_debug_flag = 0;
    return 1;
  }

  stg = network_stg(*network_p);
  if (stg == NIL(graph_t)) {
    fprintf (siserr, "No State Transition Graph (use astg_encode)\n");
    astg_debug_flag = 0;
    return 1;
  }

  bwd_add_state (stg, astg, restore_marking);

  return 0;
}

/* Perform state assignment using Tracey's technique
 */
int
com_bwd_astg_encode(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
  int c, user, i, ns, heuristic;
  network_t *new_net;
  vertex_t *state;
  char buf1[80], buf2[80], buf3[80];
  astg_graph *astg;
  graph_t *stg;

  g_debug = 0;
  enc_summary = 0;
  heuristic = 0;
  user = 0;
  util_getopt_reset();

  while ((c = util_getopt(argc, argv, "ushv:")) != EOF) {
    switch (c) {
    case 'h':
      heuristic = 1;
      break;
    case 'u':
      user = 1;
      break;
    case 's':
      enc_summary = 1;
      break;
    case 'v':
      g_debug = atoi(util_optarg);
      break;
    default:
      goto usage;
    }
  }

  astg = astg_current(*network);

  if (user) {
     stg = network_stg (*network);
     ns = stg_get_num_states (stg);
     fprintf (siserr, "give the codes in the form state_name binary_code\n");
     for (i = 0; i < ns; i++) {
        fgets (buf1, 80, stdin);
        do {
            sscanf (buf1, "%s %s", buf2, buf3);
            state = stg_get_state_by_name (stg, buf2);
            if (state == NIL(vertex_t)) {
                fprintf (siserr, "state %s not found\n", buf2);
            }
        } while (state == NIL(vertex_t));
        stg_set_state_encoding (state, buf3);
    }
  }
  else {
      new_net = astg_encode(network_stg(*network), astg, heuristic);

      if (new_net == NIL(network_t)) return 0;
      network_set_name(new_net, network_name(*network));
      new_net->stg = stg_dup((*network)->stg);
      new_net->astg = astg_dup((*network)->astg);
      network_free(*network);
      *network = new_net;
  }
  return 0;

 usage:
  (void)fprintf(siserr, "perform hazard-free state assignment\n");
  (void)fprintf(siserr, "encode_tracey [-v #]\n");
  (void)fprintf(siserr, "            -u : user-defined codes\n");
  (void)fprintf(siserr, "            -s : print summary\n");
  (void)fprintf(siserr, "            -v #: debug option\n");
  return 1;
}

/* Write out a state graph (in the format for Peter Beerel's synthesis system)
 */

static char *write_sg_usage[] = {
    "usage: %s [filename]",
    "    Writes out a state graph file",
    NULL
};

int
com_bwd_astg_write_sg (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
  int c, result, d, i, len, j;
  FILE *file;
  astg_retval status;
  graph_t *stg;
  astg_graph *astg;
  astg_generator sgen;
  astg_scode code;
  astg_signal *sig_p;
  array_t *inputs, *outputs;
  node_t *node;
  latch_t *l;
  char *filename, *name, *str;
  vertex_t *state;

  d = astg_debug_flag = 0;
  result = 0;
  filename = NIL(char);
  util_getopt_reset ();
  while ((c=util_getopt(argc,argv,"v:")) != EOF) {
    switch (c) {
    case 'v': d = atoi (util_optarg); break;
      default : result = 1; break;
    }
  }
  if (util_optind < argc) {
    filename = argv[util_optind];
  }
  else {
    filename = "-";
  }
  file = com_open_file (filename,"w",NIL(char *), /* silent */0);
  if (file == NIL(FILE)) {
    return 1;
  }


  if (result) {
    astg_usage (write_sg_usage,argv[0]);
    astg_debug_flag = 0;
    return result;
  }
  if ((astg=astg_current (*network_p)) == NULL) {
    astg_debug_flag = 0;
    return 1;
  }

  if (! astg->has_marking) {
    if (! astg_one_sm_token(astg)) {
      fprintf (siserr, "cannot find the initial state\n");
      fprintf (siserr, "please define one in the STG with .marking\n");
      return 1;
    }
  }

  /* derive the state graph by token flow */
  status = astg_initial_state (astg, &code);
  if (status != ASTG_OK && status != ASTG_NOT_USC) {
    fprintf (siserr, "inconsistency during token flow\n");
    astg_sel_show (astg);
    return 1;
  }

  astg_debug_flag = d;
  /* store the input and output names */
  inputs = array_alloc (char *, 0);
  astg_foreach_signal (astg, sgen, sig_p) {
        if (astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
            continue;
        }
        if (astg_signal_type (sig_p) == ASTG_INPUT_SIG) {
          name = util_strsav (bwd_po_name (sig_p->name));
        }
        else {
          name = util_strsav (bwd_fake_pi_name (sig_p->name));
        }
        array_insert_last (char *, inputs, name);
  }
  outputs = array_alloc (char *, 0);
  astg_foreach_signal (astg, sgen, sig_p) {
    if (astg_signal_type (sig_p) == ASTG_DUMMY_SIG) {
        continue;
    }
    if (astg_signal_type (sig_p) == ASTG_OUTPUT_SIG ||
        astg_signal_type (sig_p) == ASTG_INTERNAL_SIG) {
      name = util_strsav (bwd_fake_po_name (sig_p->name));
      array_insert_last (char *, outputs, name);
    }
  }
  /* produce the state transition graph */
  stg = network_stg(*network_p);
  if (stg != NIL(graph_t)) {
    stg_free (stg);
  }
  stg = bwd_astg_to_stg (astg, code, /* use_output */ 1, /* pre_minimize */ 0);
  if (stg == NIL(graph_t)) {
    return 1;
  }
  if (!stg_check(stg)) {
    fprintf (siserr, "stg check failed\n");
    astg_debug_flag = 0;
    return 1;
  }
  stg_set_names (stg, inputs, /* input names */ 1);
  stg_set_names (stg, outputs, /* output names */ 0);

  bwd_write_sg (file, stg, astg);
  stg_free (stg);

  if (file != stdout) {
    (void) fclose (file);
  }

  return 0;
}

int
com_stg_scc (network_p,argc, argv)
network_t **network_p;
int    argc;
char **argv;
{
    graph_t *stg;
    array_t *components, *component;
    vertex_t *state;
    int i, j, len, result, c;
    char *str;

    result = 0;
    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"v:")) != EOF) {
    switch (c) {
    case 'v': astg_debug_flag = atoi (util_optarg); break;
      default : result = 1; break;
    }
    }
    stg = network_stg(*network_p);

    if (result || stg == NIL(graph_t)) {
        astg_debug_flag = 0;
        return 0;
    }

    components = graph_scc (stg);
    fprintf (sisout, "there are %d components\n", array_n (components));
    if (astg_debug_flag > 0) {
        for (i = 0; i < array_n (components); i++) {
            component = array_fetch (array_t *, components, i);
            fprintf (sisout, "%d:", i);
            len = 4;
            for (j = 0; j < array_n (component); j++) {
                state = array_fetch (vertex_t *, component, j);
                str = stg_get_state_name (state);
                len += strlen (str) + 1;
                if (len > 80) {
                    fprintf (sisout, "\n   ");
                    len = strlen (str) + 4;
                }
                fprintf (sisout, " %s", str);
            }
            fprintf (sisout, "\n");
        }
    }

    astg_debug_flag = 0;
    return 0;
}


#endif /* SIS */
