/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/sim/interpret.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:46 $
 *
 */
#include "sis.h"
#include "sim_int.h"

/*
 *  simulate a node
 *	assumes the values are already stored for the fanin's
 *	sets the new value at the node 
 */

void
simulate_node(node)
node_t *node;
{
    int i, value;
    node_t *fanin, *func, *func1, *lit;

    func = node_dup(node);
    foreach_fanin(node, i, fanin) {
	value = GET_VALUE(fanin);

	lit = node_literal(fanin, value);
	func1 = node_cofactor(func, lit);
	node_free(func);
	node_free(lit);
	func = func1;

	if (node_function(func) == NODE_0) {
	    break;
	}
    }

    if (node_function(func) == NODE_0) {
	SET_VALUE(node, 0);
    } else {
	SET_VALUE(node, 1);
    }

    node_free(func);
}

/*
 *  simulate a network
 */

array_t *
simulate_network(network, node_vec, in_value)
network_t *network;
array_t *node_vec;		/* dfs order for nodes */
array_t *in_value;		/* values for Primary Inputs */ 
{
    int i, value;
    node_t *node, *pi, *po;
    array_t *po_value;
    lsGen gen;

    if (array_n(in_value) != network_num_pi(network)) {
	fail("network_simulate: too few input values supplied");
    }

    /* set the values on the primary inputs */
    i = 0;
    foreach_primary_input(network, gen, pi) {
	value = array_fetch(int, in_value, i);
	SET_VALUE(pi, value);
	i++;
    }

    /* simulate the values through the network */
    for(i = 0; i < array_n(node_vec); i++) {
	node = array_fetch(node_t *, node_vec, i);
	if (node->type == INTERNAL) {
	    simulate_node(node);
	}
    }

    /* retrieve the values on the outputs (from their driving nodes) */
    po_value = array_alloc(int, 10);
    foreach_primary_output(network, gen, po) {
	value = GET_VALUE(node_get_fanin(po, 0));
	array_insert_last(int, po_value, value);
    }

    return po_value;
}


#ifdef SIS

static int
equivtrans(x, y)
char *x, *y;
{
    char c;
    
    while (c = *x++) {
	if (c != '-' && *y != '-' && c != *y) {
	    return(FALSE);
	}
	y++;
    }
    return(TRUE);
}


array_t *
simulate_stg(stg, in_value, state)
graph_t *stg;
array_t *in_value;
vertex_t **state;
{
    vertex_t *current, *temp;
    edge_t *edge;
    lsGen gen;
    int value;
    char *input, *output;
    char *buf;
    int i;
    array_t *out_char;

    buf = ALLOC(char, array_n(in_value) + 1);
    for (i = 0; i < array_n(in_value); i++) {
	value = array_fetch(int, in_value, i);
	if (value == 0) {
	    buf[i] = '0';
	} else if (value == 1) {
	    buf[i] = '1';
	} else {
	    fail("simulate_stg: bad value in simulation\n");
	}
    }
    buf[array_n(in_value)] = '\0';
    current = stg_get_current(stg);
    *state = NIL(vertex_t);
    out_char = array_alloc(char, stg_get_num_outputs(stg));
    foreach_state_outedge(current, gen, edge) {
	input = stg_edge_input_string(edge);
	if (equivtrans(input, buf)) {
	    output = stg_edge_output_string(edge);
	    *state = stg_edge_to_state(edge);
            stg_set_current(stg, *state);
	    for (i = 0; i < stg_get_num_outputs(stg); i++) {
		array_insert_last(char, out_char, *output++);
	    }
	}
    }
    if (*state == NIL(vertex_t)) {
	if ((current = stg_get_state_by_name(stg, "ANY")) != NIL(vertex_t) ||
	    (temp = stg_get_state_by_name(stg, "*")) != NIL(vertex_t)) {
	    current = (current != NIL(vertex_t)) ? current : temp;
	    foreach_state_outedge(current, gen, edge) {
		input = stg_edge_input_string(edge);
		if (equivtrans(input, buf)) {
		    output = stg_edge_output_string(edge);
		    *state = stg_edge_to_state(edge);
		    stg_set_current(stg, *state);
		    for (i = 0; i < stg_get_num_outputs(stg); i++) {
			array_insert_last(char, out_char, *output++);
		    }
		}
	    }
	}
    }
		    
    FREE(buf);
    return out_char;
}

#endif

 /* routines to do simulation 32 bits at a time */
 /* implements sim_verify */

static int simulate_network_32_rec();
static int simulate_node_32();
static void gen_random_pattern();
static void print_pattern();
static void report_error();
static void simulate_network_32();

 /* EXTERNAL INTERFACE */

int sim_verify(network0, network1, n_patterns)
network_t *network0;
network_t *network1;
int n_patterns;
{
  int i, j, k;
  int n_pi = network_num_pi(network0);
  int n_po = network_num_po(network0);
  int *pi_values = ALLOC(int, n_pi);
  int **po_values = ALLOC(int *, 2);
  network_t **networks = ALLOC(network_t *, 2);

  n_patterns >>= 5;
  networks[0] = network0; 
  networks[1] = network1;

  for (j = 0; j < 2; j++) {
    po_values[j] = ALLOC(int, n_po);
  }

  for (i = 0; i < n_patterns; i++) {
    int seed = (int) network0 + (int) network1 + (int) pi_values;
    gen_random_pattern(n_pi, pi_values, seed);
    for (j = 0; j < 2; j++) {
      simulate_network_32(networks[j], pi_values, po_values[j]);
    }
    for (k = 0; k < n_po; k++) {
      if (po_values[0][k] != po_values[1][k]) {
	report_error(pi_values, po_values, networks, k);
	FREE(pi_values);
	for (i = 0; i < 2; i++)
	  FREE(po_values[i]);
	FREE(po_values);
	return 1;
      }
    }
  }
  FREE(pi_values);
  for (i = 0; i < 2; i++)
    FREE(po_values[i]);
  FREE(po_values);
  (void) fprintf(misout, "Passed verification with %d random input vectors\n", n_patterns << 5);
  return 0;
}

static void report_error(pi_values, po_values, networks, faulty_po)
int *pi_values;
int **po_values;
network_t **networks;
int faulty_po;
{
  int i;
  int diff;
  int n_pi = network_num_pi(networks[0]);
  int n_po = network_num_po(networks[1]);
  int mask;

  diff = po_values[0][faulty_po] ^ po_values[1][faulty_po];
  assert(diff);
  i = 0;
  while (diff % 2 == 0) {
    diff >>= 1;
    i++;
  }
  mask = 1 << i;
  (void) fprintf(misout, "verification failed on input value: ");
  print_pattern(n_pi, pi_values, mask);
  (void) fprintf(misout, "\n");
  (void) fprintf(misout, "internal network outputs: ");
  print_pattern(n_po, po_values[0], mask);
  (void) fprintf(misout, "\n");
  (void) fprintf(misout, "read-in network outputs: ");
  print_pattern(n_po, po_values[1], mask);
  (void) fprintf(misout, "\n");
  (void) fprintf(misout, "Note that the inputs and outputs are matched up by order, not by name\n");
}

static void print_pattern(n, pattern, mask)
int n;
int *pattern;
int mask;
{
  int i;
  for (i = 0; i < n; i++)
    fprintf(misout, "%d ", (pattern[i] & mask) != 0);
}


 /* minor hack: random generates numbers from 0 to 2^31 only */

static void gen_random_pattern(n, pattern, seed)
int n;
int *pattern;
int seed;
{
  int i;
  srandom(seed);
  for (i = 0; i < n; i++) {
    pattern[i] = (random() << 16) | (random() & 0xffff);
  }
}

static void simulate_network_32(network, pi_values, po_values)
network_t *network;
int *pi_values;
int *po_values;
{
  int i;
  lsGen gen;
  node_t *pi, *po;
  st_table *table = st_init_table(st_ptrcmp, st_ptrhash);

  i = 0;
  foreach_primary_input(network, gen, pi) {
    st_insert(table, (char *) pi, (char *) pi_values[i]);
    i++;
  }
  i = 0;
  foreach_primary_output(network, gen, po) {
    node_t *input = node_get_fanin(po, 0);
    po_values[i] = simulate_network_32_rec(network, table, input);
    i++;
  }
}

static int simulate_network_32_rec(network, table, node)
network_t *network;
st_table *table;
node_t *node;
{
  int i;
  char *value;
  int result;
  int n_inputs = node_num_fanin(node);
  int **inputs;

  if (st_lookup(table, (char *) node, &value)) return (int) value;
  assert(node->type != PRIMARY_INPUT);
  inputs = ALLOC(int *, 4);
  for (i = 0; i < 4; i++) {
    inputs[i] = ALLOC(int, n_inputs);
  }
  for (i = 0; i < n_inputs; i++) {
    node_t *input = node_get_fanin(node, i);
    inputs[ONE][i] = simulate_network_32_rec(network, table, input);
    inputs[ZERO][i] = ~ inputs[ONE][i];
    inputs[TWO][i] = 0xffffffff;
  }
  result = simulate_node_32(node, inputs);
  st_insert(table, (char *) node, (char *) result);
  for (i = 0; i < 3; i++)
    FREE(inputs[i]);
  FREE(inputs);
  return result;
}

static int simulate_node_32(node, inputs)
node_t *node;
int **inputs;
{
  register int i, j;
  node_t *fanin;
  node_cube_t cube;
  int num_cubes = node_num_cube(node);
  int and_result;
  int  result = 0;

  for (i = 0; i < num_cubes; i++) {
    and_result = 0xffffffff;
    cube = node_get_cube(node, i);
    foreach_fanin(node, j, fanin) {
      int literal = node_get_literal(cube, j);
      and_result &= inputs[literal][j];
      /*
	switch (literal) {
	case ZERO:
	and_result &= inputs[1][j];
	break;
	case ONE:
	and_result &= inputs[0][j];
	break;
	case TWO:
	break;
	default:
	fail("bad literal");
	}
	*/
    }
    result |= and_result;
  }
  return result;
}
