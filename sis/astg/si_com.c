
#ifdef SIS
#include "astg_core.h"
#include "astg_int.h"
#include "bwd_int.h"
#include "si_int.h"
#include "sis.h"

static char file[] = "/tmp/SISXXXXXX";

/* Given a state graph derived from the STG, synthesize a logic
 * which is hazard-free under the unbounded gate delay model.
 */
int com_astg_syn(network, argc, argv) network_t **network;
int argc;
char **argv;
{
  network_t *new_net;
  int c, skip_flow;

  g_debug = 0;
  add_red = 1;
  do_reduce = 1;
  skip_flow = 0;

  util_getopt_reset();
  while ((c = util_getopt(argc, argv, "mrv:x")) != EOF) {
    switch (c) {
    case 'm':
      do_reduce = 0;
      break;
    case 'r':
      add_red = 0;
      break;
    case 'v':
      g_debug = atoi(util_optarg);
      break;
    case 'x':
      skip_flow = 1;
      break;
    default:
      goto usage;
    }
  }

  if (!skip_flow) {
    /* perform token flow */
    if (com_execute(network, "_astg_flow")) {
      return 1;
    }
  }
  new_net = astg_min(*network);

  if (new_net == 0) { /* well, happens for some weird cases */
    return 0;
  }
  network_free(*network);
  *network = new_net;
  return 0;

usage:
  (void)fprintf(siserr, "usage: astg_syn [-m] [-r] [-v debug_level] [-x] \n");
  (void)fprintf(siserr, "       -m   : don't remove MIC/MOC-related hazards\n");
  (void)fprintf(siserr, "       -r   : don't add redundancy (run espresso)\n");
  (void)fprintf(siserr, "       -v debug_level : print debug info\n");
  (void)fprintf(siserr,
                "       -x   : skip token flow and synthesize direclty\n");
  return 1;
}

/* Print simple statistics of the given STG */
int com_astg_print_stat(network, argc, argv) network_t **network;
int argc;
char **argv;
{
  astg_graph *stg;
  astg_scode initial, enabled;
  astg_state *state_p;

  stg = astg_current(*network);
  if (stg == NIL(astg_graph))
    return 1;

  (void)fprintf(sisout, "File Name = %s\n", stg->filename);
  (void)fprintf(sisout, "Total Number of Signals = %d (I = %d/O = %d)\n",
                stg->n_sig, stg->n_sig - stg->n_out, stg->n_out);
  if (astg_initial_state(stg, &initial) != ASTG_OK) {
    (void)fprintf(
        sisout, "Initial State = ?? (can't find live, safe initial marking\n");
    (void)fprintf(sisout, "Total Number of States = ??\n");
    return 1;
  }
  state_p = astg_find_state(stg, initial, /* don't create */ 0);
  assert(state_p != NIL(astg_state));
  enabled = astg_state_enabled(state_p);
  (void)fprintf(sisout, "Initial State = ");
  print_state(stg, initial);
  print_enabled(stg, initial, enabled);
  if (astg_token_flow(stg, /* silent */ ASTG_FALSE) == ASTG_OK) {
    (void)fprintf(sisout, "Total Number of States = %d\n",
                  astg_state_count(stg));
  } else {
    (void)fprintf(sisout, "Total Number of States = ?? (csc violation)\n");
    return 1;
  }
  return 0;
}

/* Print the state graph */
int com_astg_print_sg(network, argc, argv) network_t **network;
int argc;
char **argv;
{
  print_state_graph(astg_current(*network));
  return 0;
}

/* global initialization routine */
void si_cmds() {
  /* speed-independent commands */
  com_add_command("astg_syn", com_astg_syn, 1);
  com_add_command("astg_print_sg", com_astg_print_sg, 0);
  com_add_command("astg_print_stat", com_astg_print_stat, 0);
}
#endif /* SIS */
