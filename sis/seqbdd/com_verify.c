/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/seqbdd/com_verify.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:54 $
 *
 */
 /* file %M% release %I% */
 /* last modified: %G% at %U% */
#ifdef SIS
#include <setjmp.h>
#include <signal.h>
#include "sis.h"
#include "prl_seqbdd.h"

static int com_extract_seq_dc();
static int com_extract_env_seq_dc();
static int com_verify_fsm();
static int com_env_verify_fsm();
static int com_remove_latches();
static int com_latch_output();
static int com_equiv_nets();
static int com_remove_dependencies();
static int com_free_dc();

static void seqbdd_print_usage();
static network_t *read_optional_network();

static jmp_buf timeout_env;

 /* ARGSUSED */
static void timeout_handle(x)
int x;
{
  longjmp(timeout_env, 1);
}

/*
 *    int
 *    function(network)
 *    network_t **network;    - value/return
 */

static struct {
    char *name;
    int (*function)();
    int changes_network;
} table[] = {
  { "verify_fsm", 		com_verify_fsm,        		TRUE    },
  { "extract_seq_dc", 		com_extract_seq_dc,        	TRUE    },
  { "env_seq_dc",	 	com_extract_env_seq_dc,		TRUE	},
  { "env_verify_fsm",	 	com_env_verify_fsm,		FALSE	},
  { "remove_latches", 	 	com_remove_latches,		TRUE	},
  { "latch_output", 	 	com_latch_output,		TRUE	},
  { "equiv_nets", 	 	com_equiv_nets,			TRUE	},
  { "remove_dep", 	 	com_remove_dependencies,        TRUE	},
  { "free_dc", 	 		com_free_dc,        		TRUE	},
};

/*
 *    init_bdd - the mis-defined bdd-package initialization
 *
 *    return nothing (why does it return an int then?)
 */
int
init_seqbdd()
{
    int i;
    int size;

    size = sizeof(table)/sizeof(table[0]);
    for (i = 0; i < size; i++) {
        com_add_command(table[i].name,  table[i].function, table[i].changes_network);
    }
#ifdef old_code
    for (i=0; i< sizeof_el(table); i++) {
        com_add_command(table[i].name,  table[i].function, table[i].changes_network);
    }
#endif
}

end_seqbdd()
{
}

/* verifies the equivalence of two finite state machine using implicit
 * state enumeration techniques. First the product machine is built.
 * It finds all the reachable states starting from the initial state
 * in the product machine and checks if the output in the product
 * machine always produces 1 for any reachable state of the product
 * machine.
 */

static int
com_verify_fsm(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
  int status;
  network_t *network1, *network2;
  verif_options_t options;

  network1 = *network;
  if (network1  == NIL(network_t)) {
      return 1;
  }
  options.does_verification = 1;
  if (bdd_range_fill_options(&options, &argc, &argv)) goto usage;
  if (options.alloc_range_data == 0) goto usage;
  if (argc - util_optind != 1) goto usage;

  argv = &argv[util_optind];
  network2  = read_optional_network("read_blif", argv[0]);
  if (network2  == NIL(network_t)) {
      return 1;
  }


  if (options.timeout > 0) {
    (void) signal(SIGALRM, timeout_handle);
    (void) alarm((unsigned int) options.timeout);
    if (setjmp(timeout_env) > 0) {
      fprintf(misout, "timeout occurred after %d seconds\n", options.timeout);
      return 1;
    }
  }
  status = seq_verify_interface(network1, network2, &options);
  if (options.timeout > 0) (void) alarm(options.timeout);
  network_free(network2);
  return (options.stop_if_verify) ? (!status) : (status);
 usage:
  seqbdd_print_usage("verify_fsm", "network2.blif");
  return 1;
}


/* Starting from initial states, finds all the reachable states.
 * Any state that is not reachable is a don't care because it
 * is not a valid input. An external don't care network is built
 * representing all the unreachable states. These are don't cares
 * for every primary output in the circuit which could be a real
 * primary output or output of a latch.
 */

static void store_as_dc_network();
static int com_extract_seq_dc(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
  network_t *new_network ;
  network_t *dup_net;
  verif_options_t options;

  if (bdd_range_fill_options(&options, &argc, &argv)) goto usage;
  if (argc - util_optind != 0) goto usage;
  options.keep_old_network = 0;
  options.n_iter = INFINITY;

  if ((*network)  == NIL(network_t)) 
      return 0;
  if (network_num_internal(*network) == 0)
      return 0;
  if (network_num_latch(*network) == 0)
      return 0;

  if (options.timeout > 0) {
    (void) signal(SIGALRM, timeout_handle);
    (void) alarm(options.timeout);
    if (setjmp(timeout_env) > 0) {
      fprintf(misout, "timeout occurred after %d seconds\n", options.timeout);
      return 1;
    }
  }
  dup_net = network_dup(*network);
  new_network = range_computation_interface(dup_net, &options);
  /* dup_net, the "old network" used to be freed in breadth_first_traversal,
     because it is no longer needed after that routine.  However, the manager
     was freed in options->free_range_data, which tries to access the
     network.  So the network is now freed here, AFTER the manager has been
     freed. */
  network_free(dup_net);
  if (new_network == 0) {
    (void) fprintf(siserr, "%s", error_string());
    return 1;
  }
  Prl_StoreAsSingleOutputDcNetwork(*network, new_network);
  return 0;

 usage:
  seqbdd_print_usage("extract_seq_dc", "");
  return 1;
}


/* Generic routines to fill the option parameters
 */

static
int bdd_range_fill_method(options, method_name)
verif_options_t *options;
char *method_name;
{
  /* Set function pointers in options depending on the method name. */

  if (strcmp (method_name, "consistency") == 0) {
    options->type        = CONSISTENCY_METHOD;
    options->alloc_range_data    = consistency_alloc_range_data;
    options->compute_next_states = consistency_compute_next_states;
    options->compute_reverse_image = consistency_compute_reverse_image;
    options->free_range_data    = consistency_free_range_data;
    options->check_output    = consistency_check_output;
    options->bdd_sizes        = consistency_bdd_sizes;
  }
  else if (strcmp (method_name, "bull") == 0) {
    options->type        = BULL_METHOD;
    options->alloc_range_data    = bull_alloc_range_data;
    options->compute_next_states = bull_compute_next_states;
    options->compute_reverse_image = bull_compute_reverse_image;
    options->free_range_data    = bull_free_range_data;
    options->check_output    = bull_check_output;
    options->bdd_sizes        = bull_bdd_sizes;
  }
  else if (strcmp (method_name, "product") == 0) {
    options->type        = PRODUCT_METHOD;
    options->alloc_range_data    = product_alloc_range_data;
    options->compute_next_states = product_compute_next_states;
    options->compute_reverse_image = product_compute_reverse_image;
    options->free_range_data    = product_free_range_data;
    options->check_output    = product_check_output;
    options->bdd_sizes        = product_bdd_sizes;
  }
  else {
    return 0;    /* unrecognized method */
  }
  return 1;
}

int bdd_range_fill_options(options, argc_addr, argv_addr)
verif_options_t *options;
int *argc_addr;
char ***argv_addr;
{
  int c;
  int argc = *argc_addr;
  char **argv = *argv_addr;
  char *s;
  
  options->timeout = 0;
  options->keep_old_network = 1;
  options->n_iter = 1;
  options->verbose = 0;
  options->use_manual_order = 0;
  options->order_network = NIL(network_t);
  options->last_time = util_cpu_time();
  options->total_time = 0;
  options->ordering_depth = 2;
  options->sim_file = NIL(char);
  options->stop_if_verify = 0;
  s = "product";
  bdd_range_fill_method (options,s);

  util_getopt_reset();
  while ((c = util_getopt(argc, argv, "i:m:o:O:t:v:s:V")) != EOF) {
    switch(c) {
    case 'i':
      options->n_iter = atoi(util_optarg);
      if (options->n_iter < 0 || options->n_iter > INFINITY) return 1;
      break;
    case 'm':
      if (bdd_range_fill_method (options,util_optarg) == 0) return 1;
      break;
    case 'o':
      options->ordering_depth = atoi(util_optarg);
      break;
    case 'O':
      options->use_manual_order = 1;
      options->order_network_name = util_strsav(util_optarg);      
      break;
    case 't':
      options->timeout = atoi(util_optarg);
      if (options->timeout < 0 || options->timeout > 3600 * 24 * 365) return 1;
      break;
    case 'v':
      options->verbose = atoi(util_optarg);
      break;
    case 's':
      options->sim_file = util_strsav(util_optarg);
      break;
    case 'V':
      options->stop_if_verify = 1;
      break;
    default:
      return 1;
    }
  }
  /* because util_optarg use global variables, 
   * we can't call read_optional_network within the loop 
   */
  if (options->use_manual_order) {
    options->order_network = read_optional_network("read_eqn", options->order_network_name);
    FREE(options->order_network_name);
    options->order_network_name = NIL(char);
    if (options->order_network == NIL(network_t)) {
      return 1;
    }
  }
  *argc_addr = argc;
  *argv_addr = argv;
  return 0;
}

static void seqbdd_print_usage(name, uniq_options)
char *name;
char *uniq_options;
{
  (void) fprintf(siserr, "usage: %s [-o d] [-t s] [-v n] [-V] -m method %s\n", name, uniq_options);
  (void) fprintf(siserr, "    [-o d]: depth of search for good variable ordering; d == 0 greedy\n");
  (void) fprintf(siserr, "    [-t s]: time out after s seconds of cpu time\n");
  (void) fprintf(siserr, "    [-v n]: verbosity level\n");
  (void) fprintf(siserr, "    [-V]: returns an error status if verification succeeds\n");
  (void) fprintf(siserr, "    [-s filename]: save a distinguishing input vector to filename\n");
  (void) fprintf(siserr, "    method is one of: ");
#define use(NAME,STRING,UPPERCASE)\
  (void) fprintf(siserr, "%s ", STRING);
use(consistency,"consistency",CONSISTENCY)
use(bull,"bull",BULL)
use(product,"product",PRODUCT)
#undef use
  (void) fprintf(siserr, "\n");
}

static network_t *read_optional_network(command, filename)
char *command;
char *filename;
{
  int status;
  char *buffer;
  network_t *result;
  int save_optind = util_optind;
  char *save_optarg = util_optarg;
  
  buffer = ALLOC(char, 20 + strlen(command) + strlen(filename));
  (void) sprintf(buffer, "%s %s", command, filename);
  result = network_alloc();
  status = com_execute(&result, buffer);
  FREE(buffer);
  if (status) {
    network_free(result);
    result = NIL(network_t);
  }
  util_optind = save_optind;
  util_optarg = save_optarg;
  return result;
}



/* FROM NOW ON: INTERFACE CODE FOR THE PRL FILES
 * IMPLEMENTS THE FOLLOWING NEW COMMANDS:
 * env_seq_dc,remove_latches,latch_output,equiv_nets
 */

static void Prl_PrintUsage  ARGS((char *, char *));
static int  Prl_FillOptions ARGS((prl_options_t *, int *, char ***));


/* Given another network, passed as an argument to the command, do:
 * (1) connect the current network to that other network
 * (2) enumerate all the states of the product networks
 * (3) use the complement of resulting set of reachable states 
 *     as external don't cares.
 * This command extracts the reachability don't care set under
 * an environment constraint. It is strictly more more powerful
 * than 'extract_seq_dc'.
 * The two networks are connected by names. An external PI (resp. PO)
 * of the current network is connected to a matching external PO
 * (resp. PI) of the network passed as argument.
 */

static int com_extract_env_seq_dc(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int status;
    prl_options_t options;
    network_t *fsm_network, *env_network;

    if (Prl_FillOptions(&options, &argc, &argv)) goto usage;

    fsm_network = *network;
    if (fsm_network == NIL(network_t)) return 0;
    if (network_num_internal(fsm_network) == 0) return 0;
    if (network_num_latch(fsm_network) == 0) return 0;

    if (argc - util_optind == 0) {
	(void) fprintf(siserr, "Warning: no network specified as argument\n");
	env_network = NIL(network_t);
    } else if (argc - util_optind == 1) {
	env_network = read_optional_network("read_blif", argv[util_optind]);
	if (env_network == NIL(network_t)) {
	    (void) fprintf(siserr, "Warning: can't read network \"%s\";", argv[util_optind]);
	}
    } else {
	goto usage;
    }

    if (env_network == NIL(network_t)) {
	(void) fprintf(siserr, "\"extract_seq_dc\" is being called instead\n");
	return com_execute(network, "extract_seq_dc");
    }

    if (options.timeout > 0) {
	(void) signal(SIGALRM, timeout_handle);
	(void) alarm(options.timeout);
	if (setjmp(timeout_env) > 0) {
	    fprintf(sisout, "timeout occurred after %d seconds\n", options.timeout);
	    return 1;
	}
    }
    status = Prl_ExtractEnvDc(fsm_network, env_network, &options);
    return status;
  usage:
    Prl_PrintUsage("env_seq_dc", "<network>");
    return 1;
}


/* 
 *----------------------------------------------------------------------
 *
 * com_env_verify_fsm -- INTERNAL ROUTINE
 *
 * Verifies two FSMs under a given environment.
 * If the environment is not specified, defaults to 'com_verify_fsm'.
 *
 *----------------------------------------------------------------------
 */

static int com_env_verify_fsm(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int status;
    prl_options_t options;
    network_t *fsm_network, *check_network, *env_network;

    if (Prl_FillOptions(&options, &argc, &argv)) goto usage;
    fsm_network = *network;
    if (fsm_network == NIL(network_t)) return 0;

    if (argc - util_optind != 2) goto usage;

    env_network = read_optional_network("read_blif", argv[util_optind+1]);
    if (env_network == NIL(network_t)) {
	(void) fprintf(siserr, "Warning: can't read network \"%s\";", argv[util_optind+1]);
	(void) fprintf(siserr, " verify_fsm is being called instead\n");
	return com_verify_fsm(network, argc - 1, argv);
    }

    check_network = read_optional_network("read_blif", argv[util_optind]);
    if (check_network == NIL(network_t)) {
	(void) fprintf(siserr, "can't read network %s\n", argv[util_optind]);
	return 1;
    }

    if (options.timeout > 0) {
	(void) signal(SIGALRM, timeout_handle);
	(void) alarm(options.timeout);
	if (setjmp(timeout_env) > 0) {
	    fprintf(sisout, "timeout occurred after %d seconds\n", options.timeout);
	    return 1;
	}
    }
    status = Prl_VerifyEnvFsm(fsm_network, check_network, env_network, &options);
    network_free(check_network);
    network_free(env_network);
    return (options.stop_if_verify) ? (! status) : (status);
  usage:
    Prl_PrintUsage("env_verify_fsm", "<check network> <env network>");
    return 1;
}


 /* 'equiv_nets' performs the following optimization:
  * 1. for each net in the network, computes the BDD representing
  *    the Boolean function of that net in terms of PIs.
  * 2. use the external don't care (e.g. reachability)
  *    and cofactor the BDD at each net by the EXDC.
  * 3. group the nets by equivalence class, were n1 and n2
  *    are equivalent iff they are always equal or always
  *    the negation of each other in the don't care set.
  * 4. for each equivalence class, select a net of smallest depth
  *    and move the fanouts of all the other nets of that class
  *    to that net, inserting inverters whenever necessary.
  * 5. sweeps the network to remove the dead logic.
  */

static int com_equiv_nets(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
  prl_options_t options;

  if (Prl_FillOptions(&options, &argc, &argv)) goto usage;
  if (argc - util_optind != 0) goto usage;

  if (*network == NIL(network_t)) {
    (void) fprintf(siserr, "no network specified\n");
    return 1;
  }
  Prl_EquivNets(*network, &options);
  network_sweep(*network);
  return 0;
 usage:
  Prl_PrintUsage("remove_latches", "");
  return 1;
}


/* 1. Remove latches that are functionally deducible from the others.
 *    Latches whose equivalent combinational logic has too many inputs
 *    (exceeding some threshold specified as a command line parameter)
 *    are not removed.
 * 2. In addition, performs some local retiming by moving latches
 *    forward if that reduces the total latch count.
 * 3. Finally, tries to remove boot latches (i.e. latches fed by a constant
 *    but initialized by a different constant) by looking for a state
 *    equivalent to the initial state in which the initial value of the latch
 *    is equal to the value of its constant input.
 *
 * The network is modified in place.
 */

static int com_remove_latches(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    prl_options_t options;

    if (Prl_FillOptions(&options, &argc, &argv)) goto usage;
    if (argc - util_optind != 0) goto usage;

    if (options.timeout > 0) {
	(void) signal(SIGALRM, timeout_handle);
	(void) alarm(options.timeout);
	if (setjmp(timeout_env) > 0) {
	    fprintf(sisout, "timeout occurred after %d seconds\n", options.timeout);
	    return 1;
	}
    }

    if (*network == NIL(network_t)) {
	(void) fprintf(siserr, "no network specified\n");
	return 1;
    }
    if (network_num_latch(*network) == 0) {
	/* nothing to do */
	return 0;
    }
    Prl_RemoveLatches(*network, &options);
    return 0;

  usage:
    Prl_PrintUsage("equiv_nets", "");
    return 1;
}


/* 'latch_output' forces the listed external POs to be fed by a latch
 * by forward retiming of latches in the transitive fanin of the PO.
 * If one of the POs depends combinationally on one of the external PIs
 * the routine reports an error status code.
 * This function is useful out there in the real world; i.e.
 * if we want to control a memory chip, 'latch_output' is a simple way to make sure 
 * that the write_enable signal does not glitch.
 */

static int com_latch_output(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c;
    array_t *node_vec;
    int status = 0;
    int verbosity = 0;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "v:")) != EOF) {
	switch(c) {
	  case 'v':
	    verbosity = atoi(util_optarg);
	    break;
	  default:
	    goto usage;
	}
    }
    node_vec = com_get_true_nodes(*network, argc-util_optind+1, argv+util_optind-1);
    status = Prl_LatchOutput(*network, node_vec, verbosity);
    array_free(node_vec);
    return status;
  usage:
    (void) fprintf(siserr, "latch_output [-v verbosity_level] <node_list>\n");
    return 1;
}

/* 'remove_dep' forces the listed external POs not to depend on the given PI.
 * It is assumed that the dependency is structural, not logical.
 * This fact is checked with the -p option.
 */

static int com_remove_dependencies(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c;
    array_t *node_vec;
    prl_removedep_t options;
    int status = 0;

    options.verbosity = 0;
    options.perform_check = 0;
    options.insert_a_one = 0;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "opv:")) != EOF) {
	switch(c) {
	  case 'o':
	    options.insert_a_one = 1;
	    break;
	  case 'p':
	    options.perform_check = 1;
	    break;
	  case 'v':
	    options.verbosity = atoi(util_optarg);
	    break;
	  default:
	    goto usage;
	}
    }
    node_vec = com_get_true_nodes(*network, argc-util_optind+1, argv+util_optind-1);
    status = Prl_RemoveDependencies(*network, node_vec, &options);
    array_free(node_vec);
    return status;
  usage:
    (void) fprintf(siserr, "remove_dep [-o] [-v verbosity_level] <input> <output list>\n");
/*  (void) fprintf(siserr, "\t[-p] perform dependency check (not implemented)\n"); */
    (void) fprintf(siserr, "\t[-o] inserts a one instead of a zero\n");
    return 1;
}


/* 
 *----------------------------------------------------------------------
 *
 * com_free_dc -- INTERNAL ROUTINE
 *
 * Frees the don't care network, if any.
 *
 *----------------------------------------------------------------------
 */

static int com_free_dc(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    Prl_RemoveDcNetwork(*network);
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * Prl_FillOptions -- 
 *
 *----------------------------------------------------------------------
 */

static int Prl_FillOptions(options, argc_addr, argv_addr)
prl_options_t *options;
int *argc_addr;
char ***argv_addr;
{
    int c;
    int argc = *argc_addr;
    char **argv = *argv_addr;
    char *method_name = NIL(char);
    char *red_latch_method_name = NIL(char);
  
    /* global options */
    options->verbose = 0;

    /* ordering heuristic: branch & bound depth */
    options->ordering_depth = 1;

    /* timing control */
    options->timeout = 0;
    options->last_time = util_cpu_time();
    options->total_time = 0;

    /* for env_verify_fsm */
    options->stop_if_verify = 0;

    /* for removing latches */
    options->remlatch.max_cost = INFINITY;
    options->remlatch.max_level = INFINITY;
    options->remlatch.local_retiming = 1;
    options->remlatch.remove_boot = 1;

    error_init();
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "o:p:f:t:v:l:riV")) != EOF) {
	switch(c) {
	  case 'o':
	    options->ordering_depth = atoi(util_optarg);
	    break;
	  case 't':
	    options->timeout = atoi(util_optarg);
	    if (options->timeout < 0 || options->timeout > 3600 * 24 * 365) return 1;
	    break;
	  case 'v':
	    options->verbose = atoi(util_optarg);
	    break;
	    /* the rest is for "remove_latches" */
	  case 'l':
	    options->remlatch.max_level = atoi(util_optarg);
	    if (options->remlatch.max_level <= 1) return 1;
	    break;
	  case 'f':
	    options->remlatch.max_cost = atoi(util_optarg);
	    if (options->remlatch.max_cost < 1) return 1;
	    break;
	  case 'r':
	    options->remlatch.local_retiming = 0;
	    break;
	  case 'i':
	    options->remlatch.remove_boot = 0;
	    break;
	  case 'V':
	    options->stop_if_verify = 1;
	    break;
	  default:
	    return 1;
	}
    }

    /* select a method: only one supported here */
    method_name = "product";
    options->type = PRODUCT_METHOD;
    options->method_name = "product";
    options->bdd_order = Prl_ProductBddOrder;
    options->init_seq_info = Prl_ProductInitSeqInfo;
    options->free_seq_info = Prl_ProductFreeSeqInfo;
    options->compute_next_states = Prl_ProductComputeNextStates;
    options->compute_reverse_image = Prl_ProductReverseImage;
    options->extract_network_input_names = Prl_ProductExtractNetworkInputNames;

    *argc_addr = argc;
    *argv_addr = argv;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Prl_PrintUsage -- 
 *
 *----------------------------------------------------------------------
 */

static void Prl_PrintUsage(name, msg)
char* name;
char* msg;
{
  (void) fprintf(siserr, "usage: %s [-o d] [-t s] [-v n] [-l m] [-f n] [-r] [-b] [-V] %s\n", name, msg);
  (void) fprintf(siserr, "---- options for: env_seq_dc, remove_latches, equiv_nets, env_verify_fsm ----\n");
  (void) fprintf(siserr, "	[-o d]: depth of search for good variable ordering;");
  (void) fprintf(siserr, " d == 0 means greedy; default is d = 1\n");
  (void) fprintf(siserr, "	[-t s]: time out after s seconds of cpu time\n");
  (void) fprintf(siserr, "	[-v n]: verbosity level\n");
  (void) fprintf(siserr, "---- options specific to remove_latches ----\n");
  (void) fprintf(siserr, "	[-f n]: remove a latch only if fanin of its recoding fn is <= n\n");
  (void) fprintf(siserr, "	[-l n]: remove a latch only if its recoding fn has a depth <= n\n");
  (void) fprintf(siserr, "	[-r]:   do not perform local retiming before remove latches\n");
  (void) fprintf(siserr, "	[-i]:   do not attempt to remove boot latches\n");
  (void) fprintf(siserr, "---- options specific to env_verify_fsm ----\n");
  (void) fprintf(siserr, "	[-V]:   returns an error status if verification succeeds\n");
}

#endif /* SIS */
