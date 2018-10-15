/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/seqbdd/prl_extract.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:54 $
 *
 */

 /* this file provides the basic functions of the sequential BDD package */

#include "sis.h"
#include "prl_seqbdd.h"
#include "prl_util.h"

static void   ExtractBddVars 	   ARGS((seq_info_t *, network_t *, array_t *, array_t *));
static int    AddEnvFsm	  	   ARGS((network_t *, network_t *, array_t *));
static bdd_t *MakeFsmInputRelation ARGS((seq_info_t *, array_t *));
static void   FreeConnectionArray  ARGS((array_t *));

typedef struct {
    char *input_name;		/* a copy of the name of an external PI in fsm */

    node_t *copy_output;	/* the node to which 'pi' is attached after performing (env x fsm) */
				/* if the PI is still around, this is NIL */

    node_t *pi;			/* the original PI if it is still in (env x fsm) */
				/* nil otherwise (mutually exclusive with 'copy_output') */
} Connection;

/*
 *----------------------------------------------------------------------
 *
 * Prl_ExtractEnvDc -- EXPORTED ROUTINE
 *
 * Enumerates all the reachable states of the product (env x fsm)
 * and put as combinational logic don't care network
 * the set of possible (fsm states x fsm inputs) given the env constraint.
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	The old DC network of 'fsm_network' is lost.
 *      A new DC network is computed as the set of possible
 *      (input x state) patterns that can occur during operation
 *      of the system (fsm_network x env_network) and is stored
 *      as 'fsm_network->dc_network'.
 * 
 *----------------------------------------------------------------------
 */

int Prl_ExtractEnvDc(fsm_network, env_network, options)
network_t *fsm_network;
network_t *env_network;
prl_options_t *options;
{
    network_t *product_network;
    network_t *dc_network;
    seq_info_t *seq_info;
    bdd_t *reachable_states;
    array_t *env_vars;		   /* input vars as well as state vars of the env_network */
    array_t *fsm_state_vars;
    bdd_t *fsm_reachable_transitions;
    bdd_t *fsm_input_relation;
    bdd_t *fsm_reachable_states;
    bdd_t *seq_dc;
    array_t *controlled_fsm_inputs;
    array_t *input_names;
    int status = 0;
    
    Prl_RemoveDcNetwork(fsm_network);

    /* enumerate product machine */
    controlled_fsm_inputs = array_alloc(Connection, 0);

    product_network = network_dup(fsm_network);
    if (AddEnvFsm(product_network, env_network, controlled_fsm_inputs)) {
	status = 1;
    } else {
	seq_info = Prl_SeqInitNetwork(product_network, options);
	reachable_states = Prl_ExtractReachableStates(seq_info, options);
	env_vars = array_alloc(bdd_t *, 0);
	fsm_state_vars = array_alloc(bdd_t *, 0);
	ExtractBddVars(seq_info, env_network, env_vars, fsm_state_vars);

	/* make the input BDDs */
	fsm_input_relation = MakeFsmInputRelation(seq_info, controlled_fsm_inputs);

	/* smooth away the free inputs and state vars of env network */
	fsm_reachable_transitions = bdd_and_smooth(reachable_states, fsm_input_relation, env_vars);
    
	/* extract fsm seq dc */
	bdd_free(reachable_states);
	array_free(env_vars);
	array_free(fsm_state_vars);
	seq_dc = bdd_not(fsm_reachable_transitions);
	bdd_free(fsm_reachable_transitions);

	/* build dc network */
	if (bdd_is_tautology(seq_dc, 0)) {
	    Prl_RemoveDcNetwork(fsm_network);
	} else {
	    input_names = (*options->extract_network_input_names)(seq_info, options);
	    dc_network = ntbdd_bdd_single_to_network(seq_dc, "foo", input_names);
	    Prl_StoreAsSingleOutputDcNetwork(fsm_network, dc_network);
	    Prl_CleanupDcNetwork(fsm_network);
	    array_free(input_names);
	}
	bdd_free(seq_dc);
	Prl_SeqInfoFree(seq_info, options);
    }

    network_free(product_network);
    FreeConnectionArray(controlled_fsm_inputs);
    return status;
}


EXTERN char *io_name 	 ARGS((node_t *, int));
static int   CopyRealPo  ARGS((network_t *, node_t *, char *));
static void  CopyLatchPo ARGS((network_t *, node_t *, char *, node_t *));

/*
 *----------------------------------------------------------------------
 *
 * AddEnvFsm -- INTERNAL ROUTINE
 *
 * Makes the product of two networks.
 * They can be arbitrary; when the input name of one
 * matches the output name of the other, a connection is created.
 * The resulting connections are returned in the Connect array.
 * Also makes sure that each PI is pointing towards the corresponding PI
 * in the original network.
 * Also, returns an array of pointers, in the result network, to nodes
 * that correspond to primary inputs in to_network and have been realized
 * by logic copied from env_network.
 *
 * Results:
 *	Returns 0 iff everything went all right.
 *
 * Side effects:
 *	1. Appends to the array 'connections' records of the form:
 *	   (pi,po,copy) where 'pi' belongs to 'to_network', 'po' belongs to 'env_network',
 *         and 'copy' belongs to the new network. 'copy' is the copy in
 *         the new network of 'po'. 'pi' is an external PI
 *         in 'to_network'. 'pi' and 'po' have the same external name.
 *	2. The 'copy' fields of the nodes in 'env_network' are modified.
 *	3. 'to_network' is modified in place.
 *
 *----------------------------------------------------------------------
 */

static int AddEnvFsm(to_network, env_network, connections)
network_t *to_network;
network_t *env_network;
array_t *connections; 	/* PI of to_network connected to PO of env_network */
{
    int i;
    char* pi_name;
    lsGen gen;
    node_t *pi;
    node_t* fanout;
    node_t *env_fanin, *env_output;
    node_t *copy_output;
    char* output_name;
    Connection connection;

    /* 
     * 'Prl_SetupCopyFields' guarantees that inputs of 'env_network'
     * that are fed by outputs of 'to_network'
     * are directly connected to the outputs of 'to_network'.
     */

    Prl_SetupCopyFields(to_network, env_network);

    foreach_primary_input(to_network, gen, pi) {
	if (! network_is_real_pi(to_network, pi)) continue;
	connection.pi = pi;
	connection.input_name = util_strsav(pi->name);
	connection.copy_output = NIL(node_t);
	array_insert_last(Connection, connections, connection);
    }

    foreach_primary_output(env_network, gen, env_output) {
	env_fanin = node_get_fanin(env_output, 0);
	copy_output = Prl_CopySubnetwork(to_network, env_fanin);
	output_name = io_name(env_output, 0);
	if (! network_is_real_po(env_network, env_output)) {
	    CopyLatchPo(to_network, copy_output, output_name, env_output);
	} else {
	    pi = network_find_node(to_network, output_name);
	    if (pi == NIL(node_t) || ! network_is_real_pi(to_network, pi)) {
		if (CopyRealPo(to_network, copy_output, output_name)) return 1;
	    } else {
		for (i = 0; i < array_n(connections); i++) {
		    connection = array_fetch(Connection, connections, i);
		    if (connection.pi == pi) {
			connection.copy_output = copy_output;
			array_insert(Connection, connections, i, connection);
			break;
		    }
		}
	    }
	}
    }

    /*
     * We need to make sure that inputs of 'to_network' that are fed by outputs of 'env_network'
     * are directly connected to the outputs of 'to_network'.
     * for that, we need to move the fanout of 'pi' onto 'copy_output'.
     */

    for (i = 0; i < array_n(connections); i++) {
	connection = array_fetch(Connection, connections, i);
	if (connection.copy_output == NIL(node_t)) continue;
	foreach_fanout(connection.pi, gen, fanout) {
	    (void) node_patch_fanin(fanout, connection.pi, connection.copy_output);
	    node_scc(fanout);
	}
	network_delete_node(to_network, connection.pi);
	connection.pi = NIL(node_t);
	array_insert(Connection, connections, i, connection);
    }
    return 0;
}


static node_t *AddPrimaryOutput ARGS((network_t *, node_t *, char *));

/*
 *----------------------------------------------------------------------
 * CopyRealPo -- INTERNAL ROUTINE
 *
 * 'node' is already a node of 'network'.
 * We need to add a node named 'name' and make that node a PO fed by 'node'.
 * If there is already one with that name, it is a fatal error.
 *
 * Results:
 *	0 iff everything went OK.
 *
 * Side Effects:
 *	Creates a new external PO of 'network'.
 *
 *----------------------------------------------------------------------
 */

static int CopyRealPo(network, node, name)
network_t *network;
node_t *node;
char *name;
{
    node_t *product_output;
    node_t *matching_output, *matching_fanin;
    node_t *tmp[2];

    if (network_find_node(network, name)) {
	(void) fprintf(siserr, "Error: network \"%s\" already has a PO named \"%s\"; abort\n", 
		       network->net_name,
		       name);
	return 1;
    }
    (void) AddPrimaryOutput(network, node, util_strsav(name));
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * CopyLatchPo -- INTERNAL ROUTINE
 *
 * Given 'node' which belongs to 'network', makes 'node' the input of 
 * a freshly created latch. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	Change the name of 'node' to a disambiguated version if 
 *	'name' conflicts with existing node names in 'network'.
 *	A new latch is created in 'network'.
 *
 *----------------------------------------------------------------------
 */

static void CopyLatchPo(network, node, name, original_po)
network_t *network;
node_t *node;
char *name;
node_t *original_po;
{
    node_t *product_output;
    node_t *product_po;
    node_t *matching_input;
    latch_t *latch, *product_latch;

    name = Prl_DisambiguateName(network, name, NIL(node_t));
    product_po = AddPrimaryOutput(network, node, name);
    network_swap_names(network, node, product_po);
    matching_input = network_latch_end(original_po);
    network_create_latch(network, &product_latch, product_po, matching_input->copy);
    latch = latch_from_node(original_po);
    latch_set_initial_value(product_latch, latch_get_initial_value(latch));
}

/*
 *----------------------------------------------------------------------
 *
 * AddPrimaryOutput -- INTERNAL ROUTINE
 *
 * The library routine 'network_add_primary_input'
 * does not do the job properly.
 * It swaps the names of the 'fanin' and the newly created node.
 * This is incorrect if the 'fanin' is a PI. Here we insert
 * a buffer when the 'fanin' is a PI to avoid this problem.
 * Here we make sure that the resulting PO is named 'name'.
 *
 * Hack due to the fact that sis does not have an explicit notion of nets
 * and assign names to nodes instead. That creates havoc in cases such as:
 * .latch a b 0
 * .latch b c 0
 * Here 'b' is used both as a name for a PI and a PO.
 * The rule is that PIs keep their names in any circumstance.
 *
 * Results:
 * 	The freshly created PO.
 *
 * Side Effects:
 *	(to be specified).
 *
 *----------------------------------------------------------------------
 */

static node_t *AddPrimaryOutput(network, fanin, name)
network_t *network;
node_t * fanin;
char* name;
{
    node_t *result;

    if (fanin->type == PRIMARY_INPUT) {
	fanin = node_literal(fanin, 1);
	network_add_node(network, fanin);
    }
    if (strcmp(fanin->name, name) != 0) { /* i.e. if not equal */
	network_change_node_name(network, fanin, name);
    }
    result = network_add_primary_output(network, fanin);
    return result;
}



/*
 * env_vars are all smoothed away.
 * fsm_state_vars are only needed to count reachable states.
 */

static void ExtractBddVars(seq_info, env_network, env_vars, fsm_state_vars)
seq_info_t *seq_info;
network_t *env_network;
array_t *env_vars;
array_t *fsm_state_vars;
{
    int i;
    lsGen gen;
    bdd_t* var;
    node_t *input;

    foreach_primary_input(env_network, gen, input) {
	input->copy->copy = input;
    }
    for (i = 0; i < array_n(seq_info->input_nodes); i++) {
	node_t *input = array_fetch(node_t *, seq_info->input_nodes, i);
	var = array_fetch(bdd_t *, seq_info->input_vars, i);
	if (input->copy != NIL(node_t) && node_network(input->copy) == env_network) {
	    array_insert_last(bdd_t *, env_vars, var);
	} else if (! network_is_real_pi(node_network(input), input)) {
	    array_insert_last(bdd_t *, fsm_state_vars, var);
	}
    }
    foreach_primary_input(env_network, gen, input) {
	input->copy->copy = NIL(node_t);
    }
}

 /* 
  *----------------------------------------------------------------------
  *
  * MakeFsmInputRelation -- INTERNAL ROUTINE
  *
  * Builds the AND of expressions of the form: (x[i] XNOR fn[i](input,state))
  * for each input of the FSM that is bound to an output of the environment.
  *
  * Side Effects:
  * For each input of the FSM that is bound to an output of the environment
  * we introduce a new primary input that is added to seq_info->network.
  * That primary input is given a BDD variable (inserted in seq_info->input_vars)
  * and seq_info->input_nodes and seq_info->var_names are updated accordingly.
  *
  * NOTES:
  * The index in seq_info->var_names should always be equal to the
  * BDD variable associated with the corresponding input. This is checked
  * by an assertion.
  *
  *----------------------------------------------------------------------
  */

static bdd_t *MakeFsmInputRelation(seq_info, controlled_fsm_inputs)
seq_info_t *seq_info;
array_t *controlled_fsm_inputs;
{
  int i;
  bdd_t *result;
  bdd_t *fn, *var, *tmp, *new_result;
  node_t *pi;
  Connection connection;
  /* inputs of fsm, outputs of env */
  array_t *new_input_vars = array_alloc(bdd_t *, 0);

  /* Add new variables for the controlled fsm inputs. */
  for (i = 0; i < array_n(controlled_fsm_inputs); i++) {
    connection = array_fetch(Connection, controlled_fsm_inputs, i);
    if (connection.copy_output != NIL(node_t)) {
      bdd_t *var = bdd_create_variable(seq_info->manager);
      array_insert_last(bdd_t *, new_input_vars, var);
    } 
    else {
      /* Unused entries. Should be BDDs for Prl_FreeBddArray. */
      array_insert_last(bdd_t *, new_input_vars, bdd_one(seq_info->manager));
    }
  }

  /* Add corresponding variables to seq_info and primary inputs to the network. */
  for (i = 0; i < array_n(controlled_fsm_inputs); i++) {
    connection = array_fetch(Connection, controlled_fsm_inputs, i);
    if (connection.copy_output != NIL(node_t)) {
      var = array_fetch(bdd_t *, new_input_vars, i);
      var = bdd_dup(var);
      array_insert_last(bdd_t *, seq_info->input_vars, var);
      pi = node_alloc();
      pi->name = util_strsav(connection.input_name);
      network_add_primary_input(seq_info->network, pi);
      array_insert_last(node_t *, seq_info->input_nodes, pi);
      array_insert_last(char *, seq_info->var_names, util_strsav(pi->name));
      assert(bdd_top_var_id(var) == array_n(seq_info->var_names) - 1);
    }
  }

  /* Build the resulting bdd. */
  result = bdd_one(seq_info->manager);
  for (i = 0; i < array_n(controlled_fsm_inputs); i++) {
    connection = array_fetch(Connection, controlled_fsm_inputs, i);
    if (connection.copy_output != NIL(node_t)) {
      fn = ntbdd_node_to_bdd(connection.copy_output, seq_info->manager, seq_info->leaves);
      var = array_fetch(bdd_t *, new_input_vars, i);
      tmp = bdd_xnor(var, fn);
      new_result = bdd_and(tmp, result, 1, 1);
      bdd_free(tmp);
      bdd_free(result);
      result = new_result;
    }
  }
  Prl_FreeBddArray(new_input_vars);
  return result;
}



typedef int (*CheckFn) ARGS((bdd_t *, bdd_t *, seq_info_t *, prl_options_t *, void *));
static  int  CheckCurrentStateTautology ARGS((bdd_t *, bdd_t *, seq_info_t *, prl_options_t *, void *));

static int  BreadthFirstTraversal ARGS((seq_info_t *, prl_options_t *, CheckFn, void *));
static int  MakeProductFsm 	  ARGS((network_t *, network_t *, array_t *, array_t *));
static void ReportEnvErrorTrace   ARGS((network_t *, array_t *, seq_info_t *, prl_options_t *));
static void RemoveUnusedPos       ARGS((network_t *, array_t *, prl_options_t *));

/*
 *----------------------------------------------------------------------
 *
 * Prl_VerifyEnvFsm -- EXPORTED ROUTINE
 *
 * Verifies that two FSMs are equivalent under a given environment FSM.
 * 1. Builds the product network of 'fsm_network' and 'check_network'.
 *    The result is placed in 'check_network'.
 *    The inputs are connected together, and the outputs are duplicated.
 *    The outputs of 'check_network' are kept as is. The copied outputs
 *    of 'fsm_network' are XNORed with their corresponding outputs
 *    in 'check_network' and become new POs. Those two categories of POs
 *    are kept in two different lists.
 * 2. The 'env_network' is added. If it has a PI that matches an output
 *    of 'check_network', the PI/PO pair is connected.
 *    We can tolerate free inputs to 'check_network'
 *    but free outputs of 'env_network' do not make any sense, so we report a warning
 *    and ignore these outputs (done in 3).
 * 3. Any remaining PO of 'check_network' that is not an XNOR is removed.
 * 4. States of the product machine are enumerated. At each step, we check
 *    that all POs are always equal to 1 for any input combination.
 * 5. If all states are enumerated without error, we are done.
 * 6. In case of a verification error, we first compute a sequence of 
 *    input minterms that generate the error. By simulation through 'fsm_network'
 *    and 'env_network', we deduce a sequence of input minterms to 'fsm_network'
 *    that activates the problem.
 *
 * Results:
 *	Returns 0 iff everything went all right.
 *
 * Side effects:
 * 	'fsm_network' is unmodified.
 * 	'env_network' is unmodified.
 *      'check_network' is modified in place.
 *	The caller should take care of deallocating the networks.
 *	In case of verification error, prints on siserr an input sequence
 *	that can be generated by the environment and differentiates the two FSMs.
 * 
 *----------------------------------------------------------------------
 */

int Prl_VerifyEnvFsm(fsm_network, check_network, env_network, options)
network_t *fsm_network;
network_t *check_network;
network_t *env_network;
prl_options_t *options;
{
    seq_info_t *seq_info;
    array_t *xnor_outputs;
    array_t *other_outputs;
    array_t *controlled_fsm_inputs;
    int status = 0;

    /* 'fsm_network' is not modified */
    xnor_outputs  = array_alloc(node_t *,  0);
    other_outputs = array_alloc(node_t *,  0);
    controlled_fsm_inputs = array_alloc(Connection, 0);
    if (MakeProductFsm(check_network, fsm_network, xnor_outputs, other_outputs)
	|| AddEnvFsm(check_network, env_network, controlled_fsm_inputs)) {
	status = 1;
    } else {
	RemoveUnusedPos(check_network, xnor_outputs, options);
	seq_info = Prl_SeqInitNetwork(check_network, options);
	status = BreadthFirstTraversal(seq_info, options, CheckCurrentStateTautology, NIL(void));
	if (status) {
	    ReportEnvErrorTrace(check_network, controlled_fsm_inputs, seq_info, options);
	} else {
	    (void) fprintf(misout, "FSM's are equivalent under given environment\n");
	}
	Prl_SeqInfoFree(seq_info, options);
    }
    array_free(xnor_outputs);
    array_free(other_outputs);
    FreeConnectionArray(controlled_fsm_inputs);
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * BreadthFirstTraversal -- INTERNAL ROUTINE
 *
 * Traverses a machine until all states are reached or a predicate fails.
 * The predicate may have side effects.
 *
 *----------------------------------------------------------------------
 */

static int BreadthFirstTraversal(seq_info, options, checkFn, closure)
seq_info_t *seq_info;
prl_options_t *options;
CheckFn checkFn;
void *closure;
{
    bdd_t *current_set;
    bdd_t *total_set;
    bdd_t *new_current_set;
    bdd_t *new_states;
    bdd_t *care_set;
    bdd_t *new_total_set;

    int status = 0;

    Prl_ReportElapsedTime(options, "begin STG traversal");

    current_set = bdd_dup(seq_info->init_state_fn);
    total_set   = bdd_dup(current_set);

    for (;;) {
	if ((*checkFn)(current_set, total_set, seq_info, options, closure)) {
	    status = 1;
	    break;
	}
	new_current_set = (*options->compute_next_states)(current_set, seq_info, options);
	if (options->verbose >= 1) {
	    double total_onset, new_onset;
	    total_onset = bdd_count_onset(total_set, seq_info->present_state_vars);
	    new_states  = bdd_and(new_current_set, total_set, 1, 0);
	    new_onset   = bdd_count_onset(new_states, seq_info->present_state_vars);
	    bdd_free(new_states);
	    (void) fprintf(sisout, "add %2.0f states to %2.0f states\n", new_onset, total_onset);
	}
	if (bdd_leq(new_current_set, total_set, 1, 1)) {
	    bdd_free(new_current_set);
	    status = 0;
	    break;
	}
	care_set      = bdd_not(total_set);
	current_set   = bdd_cofactor(new_current_set, care_set);
	bdd_free(care_set);
	new_total_set = bdd_or(new_current_set, total_set, 1, 1);
	bdd_free(new_current_set);
	bdd_free(total_set);
	total_set     = new_total_set;
    }

    bdd_free(current_set);
    bdd_free(total_set);
    Prl_ReportElapsedTime(options, "end STG traversal");
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckCurrentStateTautology -- INTERNAL ROUTINE
 *
 * Results:
 * 	returns 0 iff all outputs are at 1 in all states of 'current_set'.
 *
 * Side effects:
 *	Prints an error message if the check fails.
 *	If 'output_index' is not nil and the check fails,
 *	sets 'output_index' to the index of an output
 *	which fails the test.
 *
 *----------------------------------------------------------------------
 */

 /* ARGSUSED */

static int CheckCurrentStateTautology(current_set, total_set, seq_info, options, closure)
bdd_t *current_set;
bdd_t *total_set;
seq_info_t *seq_info;
prl_options_t *options;
void *closure;
{
    int i;
    bdd_t *fn;

    for (i = 0; i < array_n(seq_info->external_output_fns); i++) {
	fn = array_fetch(bdd_t *, seq_info->external_output_fns, i);
	if (! bdd_leq(current_set, fn, 1, 1)) {
	    (void) fprintf(siserr, "does not pass verification\n");
	    return 1;
	}
    }
    return 0;
}



static int  CheckAndRecordStates ARGS((bdd_t *, bdd_t *, seq_info_t *, prl_options_t *, void *));
static void PrintEnvErrorTrace   ARGS((array_t *, network_t *, array_t *, seq_info_t *, prl_options_t *));

typedef struct ClosureStruct {
    array_t *total_sets;
    int output_index;
} Closure;

 /*
  *----------------------------------------------------------------------
  *
  * ReportEnvErrorTrace -- INTERNAL ROUTINE
  *
  * Enumerates states until an error is found.
  * At each iteration, keeps around a copy of 'total_set'.
  * Then go backwards and generates an input sequence
  * justifying the error. This algorithm guarantees
  * the shortest possible error trace.
  *
  * Once the error trace is obtained for the entire network,
  * we simulate it through the 'env_network' and 'fsm_network'
  * to get an error trace for the 'env_network'.
  *
  * 'connections' contain the connections between the env POs
  * and the PIs of the fsm.
  *
  *----------------------------------------------------------------------
  */

static void ReportEnvErrorTrace(product_network, connections, seq_info, options)
network_t *product_network;
array_t *connections;
seq_info_t *seq_info;
prl_options_t *options;
{
    Closure closure;
    int iter_count;
    bdd_t *tmp;
    bdd_t *output_fn;
    bdd_t *total_set;
    bdd_t *incorrect_domain;
    bdd_t *incorrect_state, *incorrect_input;
    array_t *input_trace;

    /* 
     * Generates an array of total_sets, one per iteration, until failure.
     */

    closure.total_sets = array_alloc(bdd_t *, 0);
    assert(BreadthFirstTraversal(seq_info, options, CheckAndRecordStates, (void *) &closure));

    /* 
     * Traverses the machine backwards to generate a trace.
     */

    output_fn        = array_fetch(bdd_t *, seq_info->external_output_fns, closure.output_index);
    iter_count       = array_n(closure.total_sets) - 1;
    total_set        = array_fetch(bdd_t *, closure.total_sets, iter_count);
    incorrect_domain = bdd_and(total_set, output_fn, 1, 0);
    input_trace      = array_alloc(bdd_t *, 0);

    for (;;) {
	Prl_GetOneEdge(incorrect_domain, seq_info, &incorrect_state, &incorrect_input);
	bdd_free(incorrect_domain);
	array_insert(bdd_t *, input_trace, iter_count, incorrect_input);
	if (iter_count == 0) break;
	iter_count--;
	tmp              = (*options->compute_reverse_image)(incorrect_state, seq_info, options);
	total_set        = array_fetch(bdd_t *, closure.total_sets, iter_count);
	incorrect_domain = bdd_and(tmp, total_set, 1, 1);
	bdd_free(tmp);
    }

    /*
     * Cleanup and print the error trace.
     */

    PrintEnvErrorTrace(input_trace, product_network, connections, seq_info, options);

    Prl_FreeBddArray(closure.total_sets);
    Prl_FreeBddArray(input_trace);
}

/*
 *----------------------------------------------------------------------
 *
 * CheckAndRecordStates -- INTERNAL ROUTINE
 *
 * Results:
 * 	returns 0 iff all outputs are at 1 in all states of 'current_set'.
 * 
 * Side effects:
 *	Accumulates in 'closure' an array of bdds
 *	containing the total sets seen at each iteration.
 *
 *----------------------------------------------------------------------
 */

static int CheckAndRecordStates(current_set, total_set, seq_info, options, closure)
bdd_t *current_set;
bdd_t *total_set;
seq_info_t *seq_info;
prl_options_t *options;
void *closure;
{
    int i;
    bdd_t *fn;
    Closure *data = (Closure *) closure;

    array_insert_last(bdd_t *, data->total_sets, bdd_dup(total_set));
    for (i = 0; i < array_n(seq_info->external_output_fns); i++) {
	fn = array_fetch(bdd_t *, seq_info->external_output_fns, i);
	if (! bdd_leq(current_set, fn, 1, 1)) {
	    data->output_index = i;
	    return 1;
	}
    }
    return 0;
}

typedef struct PrintClosureStruct {
    network_t *network;		   /* the product network: fsm x check x env */
    array_t   *input_vars;	   /* a bdd_t* per PI of 'network'; only external PI used */
				   /* should match ordering of 'foreach_primary_input(network)' */
    array_t   *node_vec;	   /* the result of 'network_dfs(network)' */
    array_t   *input_values;	   /* an array of int; match order of 'input_vars' */
    array_t   *fsm_inputs;	   /* an array of node_t *; copies of 'fsm_network' ext. PIs */
} PrintClosure;
			

static PrintClosure *PrintClosureInit 		 ARGS((network_t *, array_t *, seq_info_t *));
static array_t      *PrintClosureSimulateNetwork ARGS((bdd_t *, PrintClosure *));
static void          PrintClosureFree 		 ARGS((PrintClosure *));


 /*
  *----------------------------------------------------------------------
  *
  * PrintEnvErrorTrace -- INTERNAL ROUTINE
  *
  * Given an input trace in terms of the 'env_network',
  * generates an input trace in terms of the 'fsm_network'
  * and prints it. This is achieved by simulating the data in 'input_trace'
  * on the 'product_network', and printing the simulation values
  * of the nodes in 'product_network' that corresponds to the inputs of
  * 'fsm_network' and are stored in the array 'connections'.
  *
  *----------------------------------------------------------------------
  */

static void PrintEnvErrorTrace(input_trace, product_network, connections, seq_info, options)
array_t *input_trace;
network_t *product_network;
array_t *connections;
seq_info_t *seq_info;
prl_options_t *options;
{
    int i;
    PrintClosure *closure;

    /* Then: Initialize the network for simulation */

    closure = PrintClosureInit(product_network, connections, seq_info);

    /* Run the simulation and cleanup */

    for (i = 0; i < array_n(input_trace); i++) {
	bdd_t *input = array_fetch(bdd_t *, input_trace, i);
	PrintClosureSimulateNetwork(input, closure);
    }

    PrintClosureFree(closure);
}

static array_t *ExtractFsmInputs ARGS((network_t *, array_t *));


 /* 
  *----------------------------------------------------------------------
  *
  * PrintClosureInit -- INTERNAL ROUTINE
  *
  * Results:
  *	A correctly initialized copy of a 'PrintClosure' object.
  *
  * Side effects:
  *	'product_network' is reset to its initial state.
  *
  *----------------------------------------------------------------------
  */

static PrintClosure *PrintClosureInit(product_network, connections, seq_info)
network_t *product_network;
array_t *connections;
seq_info_t *seq_info;
{
    lsGen gen;
    node_t *pi;
    latch_t *latch;
    int init_value;
    st_table *pi_to_var_table;
    PrintClosure *closure = ALLOC(PrintClosure, 1);

    pi_to_var_table = Prl_GetPiToVarTable(seq_info);
    closure->network = product_network;
    closure->input_vars = array_alloc(bdd_t *, 0);
    foreach_primary_input(closure->network, gen, pi) {
	bdd_t *var;
	if (network_is_real_pi(closure->network, pi)) {
	    assert(st_lookup(pi_to_var_table, (char *) pi, (char **) &var));
	} else {
	    var = NIL(bdd_t);
	}
	array_insert_last(bdd_t *, closure->input_vars, var);
    }
    st_free_table(pi_to_var_table);

    foreach_latch(closure->network, gen, latch) {
	init_value = latch_get_initial_value(latch);
	latch_set_current_value(latch, init_value);
    }

    closure->node_vec = network_dfs(closure->network);
    closure->input_values = array_alloc(int, network_num_pi(closure->network));
    closure->fsm_inputs = ExtractFsmInputs(closure->network, connections);

    return closure;
}

/*
 *----------------------------------------------------------------------
 *
 * ExtractFsmInputs -- INTERNAL ROUTINE
 *
 * Results:
 * 	an array of node_t *'s representing the external PIs
 *	of 'product_network' before the merging with the 'env_network'.
 *
 *----------------------------------------------------------------------
 */

static array_t *ExtractFsmInputs(network, connections)
network_t *network;
array_t *connections;
{
    int i;
    lsGen gen;
    node_t *node;
    array_t *fsm_inputs;
    Connection connection;

    for (i = 0; i < array_n(connections); i++) {
	connection = array_fetch(Connection, connections, i);
	(void) fprintf(sisout, "%s ", connection.input_name);
    }
    (void) fprintf(sisout, "\n");

    fsm_inputs = array_alloc(node_t *, 0);
    for (i = 0; i < array_n(connections); i++) {
	connection = array_fetch(Connection, connections, i);
	if (connection.pi == NIL(node_t)) {
	    node = connection.copy_output;
	} else {
	    node = connection.pi;
	}
	assert(node != NIL(node_t));
	array_insert_last(node_t *, fsm_inputs, node);
    }
    return fsm_inputs;
}

 /* 
  *----------------------------------------------------------------------
  *
  * PrintClosureSimulateNetwork -- INTERNAL ROUTINE
  *
  *----------------------------------------------------------------------
  */

static array_t *PrintClosureSimulateNetwork(input, closure)
bdd_t *input;
PrintClosure *closure;
{
    int i;
    int value;
    lsGen gen;
    latch_t *latch;
    node_t *pi, *po;
    bdd_t *var;
    array_t *output_values;
    
    i = 0;
    foreach_primary_input(closure->network, gen, pi) {
	if ((latch = latch_from_node(pi)) != NIL(latch_t)) {
	    value = latch_get_current_value(latch);
	} else {
	    assert((var = array_fetch(bdd_t *, closure->input_vars, i)) != NIL(bdd_t));
	    value = (bdd_leq(input, var, 1, 1)) ? 1 : 0;
	}
	array_insert(int, closure->input_values, i, value);
	i++;
    }

    output_values = simulate_network(closure->network, closure->node_vec, closure->input_values);

    i = 0;
    foreach_primary_output(closure->network, gen, po) {
	if ((latch = latch_from_node(po)) != NIL(latch_t)) {
	    value = array_fetch(int, output_values, i);
	    latch_set_current_value(latch, value);
	}
	i++;
    }
    array_free(output_values);

    (void) fprintf(sisout, "simulate ");
    for (i = 0; i < array_n(closure->fsm_inputs); i++) {
	node_t *node = array_fetch(node_t *, closure->fsm_inputs, i);
	value = GET_VALUE(node);
	(void) fprintf(sisout, "%d ", value);
    }
    (void) fprintf(sisout, "\n");
}


 /* 
  *----------------------------------------------------------------------
  *
  * PrintClosureFree -- INTERNAL ROUTINE
  *
  *----------------------------------------------------------------------
  */

static void PrintClosureFree(closure)
PrintClosure *closure;
{
    array_free(closure->input_vars);
    array_free(closure->node_vec);
    array_free(closure->input_values);
    array_free(closure->fsm_inputs);
    FREE(closure);
}

static node_t *AddXnorPo ARGS((network_t *, node_t *, node_t *));

/*
 *----------------------------------------------------------------------
 *
 * MakeProductFsm -- INTERNAL ROUTINE
 *
 * Builds the product network of 'from_network' and 'to_network'.
 *
 * The inputs are connected together, and the outputs are duplicated.
 * The outputs of 'to_network' are kept as is. The copied outputs
 * of 'from_network' are XNORed with their corresponding outputs
 * in 'to_network' and become new POs. Those two categories of POs
 * are kept in two different arrays: 'xnor_outputs' and 'other_outputs'.
 *
 * It is the responsability of the caller to allocate/deallocate
 * the 'xnor_outputs' and 'other_outputs' arrays.
 *
 * Results:
 *	returns 1 iff 'from_network' contains external POs
 *	that do not match by name with external POs in 'to_network'.
 *
 * Side effects:
 * 	The resulting product is placed in 'to_network'.
 *	'from_network' is unchanged.
 * 
 *----------------------------------------------------------------------
 */

static int MakeProductFsm(to_network, from_network, xnor_outputs, other_outputs)
network_t *to_network;
network_t *from_network;
array_t *xnor_outputs;
array_t *other_outputs;
{
    lsGen gen;
    node_t *from_output;
    node_t *from_fanin;
    node_t *to_output;
    node_t *copy_output;
    char *output_name;

    /* 
     * 'Prl_SetupCopyFields' guarantees that extenal PIs of 'from_network'
     * that have homonymous PIs in 'to_network' are used
     * instead of the 'from_network' PIs.
     */

    Prl_SetupCopyFields(to_network, from_network);

    foreach_primary_output(from_network, gen, from_output) {
	output_name = io_name(from_output, 0);
	to_output = network_find_node(to_network, output_name);
	from_fanin = node_get_fanin(from_output, 0);
	copy_output = Prl_CopySubnetwork(to_network, from_fanin);
	if (! network_is_real_po(from_network, from_output)) {
	    CopyLatchPo(to_network, copy_output, output_name, from_output);
	} else if (to_output == NIL(node_t) || ! network_is_real_po(to_network, to_output)) {
	    return 1;
	} else {
	    node_t *xnor_node = AddXnorPo(to_network, to_output, copy_output);
	    array_insert_last(node_t *, xnor_outputs, xnor_node);
	    array_insert_last(node_t *, other_outputs, to_output);
	}
    }
    return 0;
}


/*
 *----------------------------------------------------------------------
 *
 * AddXnorPo -- INTERNAL ROUTINE
 *
 * Given two nodes in a network, one being an external PO, 
 * adds another external PO which is the XNOR of the first external PO
 * with the other node. 
 *
 * Results:
 *	The new external PO.
 *
 *----------------------------------------------------------------------
 */

static node_t *AddXnorPo(network, po, node)
network_t *network;
node_t *po;
node_t *node;
{
    char* name;
    node_t *tmp0, *tmp1;
    node_t *xnor_node;

    tmp0 = node_literal(node_get_fanin(po, 0), 1);
    tmp1 = node_literal(node, 1);
    xnor_node = node_xnor(tmp0, tmp1);
    node_free(tmp0);
    node_free(tmp1);
    name = io_name(po, 0);
    xnor_node->name = Prl_DisambiguateName(network, name, NIL(node_t));
    network_add_node(network, xnor_node);
    return network_add_primary_output(network, xnor_node);
}

/*
 *----------------------------------------------------------------------
 * 
 * RemoveUnusedPos -- INTERNAL ROUTINE
 *
 * Look for any external PO not in 'used_outputs'.
 * When one is found, mark it and remove it later.
 * Generates a warning when one is removed.
 * 
 * Side effects:
 *	1. Use the 'copy' field of the PO nodes to indicate
 *	   whether a node is used or not.
 *	2. Unused POs are discarded.
 *
 *----------------------------------------------------------------------
 */

static void RemoveUnusedPos(network, used_outputs, options)
network_t *network;
array_t *used_outputs;
prl_options_t *options;
{
    int i;
    lsGen gen;
    node_t *po;
    array_t *to_be_removed = array_alloc(node_t *, 0);

    foreach_primary_output(network, gen, po) {
	po->copy = NIL(node_t);
    }
    for (i = 0; i < array_n(used_outputs); i++) {
	po = array_fetch(node_t *, used_outputs, i);
	po->copy = po;
    }
    foreach_primary_output(network, gen, po) {
	if (! network_is_real_po(network, po)) continue;
	if (po->copy == po) continue;
	array_insert_last(node_t *, to_be_removed, po);
    }
    for (i = 0; i < array_n(to_be_removed); i++) {
	po = array_fetch(node_t *, to_be_removed, i);
	if (options->verbose > 0) {
	    (void) fprintf(siserr, "Warning: PO \"%s\" is skipped\n", io_name(po, 0));
	}
	network_delete_node(network, po);
    }
    array_free(to_be_removed);
}

/*
 *----------------------------------------------------------------------
 *
 * FreeConnectionArray -- INTERNAL PROCEDURE
 *
 * simple utility to free strings allocated in 'AddEnvFsm'
 * in Connection records.
 *
 *----------------------------------------------------------------------
 */

static void FreeConnectionArray(connections)
array_t *connections;
{
    int i;
    Connection connection;

    for (i = 0; i < array_n(connections); i++) {
	connection = array_fetch(Connection, connections, i);
	FREE(connection.input_name);
    }
    array_free(connections);
}


