
#include "fanout_int.h"
#include "lib_int.h"
#include "map_delay.h"
#include "map_int.h"
#include "sis.h"

static void check_fanin();

static void check_fanout();

static void concentrate_gate_links_on_source();

static void remove_buffers();

static void do_flatten();

static void do_remove_wires();

static void extract_local_virtual_network();

static void extract_local_virtual_network_rec();

static void make_sources_external();

static int flatten_is_empty();

static int flatten_is_source();

static void remove_intermediate_nodes();

static void virtual_network_check_node();

static void virtual_network_setup_node_gate_links();

static void virtual_setup_inverter();

static void flatten_check_consistency();

/* EXTERNAL INTERFACE */

/* set up gate links. Clear up whatever was there before */

static bin_global_t global;

void virtual_network_setup_gate_links(network, options) network_t *network;
bin_global_t *options;
{
  int i;
  node_t *node;
  array_t *nodevec;
  st_generator *gen;
  char *key, *dummy;
  st_table *delete_table;

  global = *options;
  delete_table = st_init_table(st_numcmp, st_numhash);
  nodevec = network_dfs_from_input(network);
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    gate_link_delete_all(node);
  }
  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    virtual_network_setup_node_gate_links(node, delete_table);
  }
  st_foreach_item(delete_table, gen, &key, &dummy) {
    virtual_network_remove_node((node_t *)key, 1);
  }
  array_free(nodevec);
  st_free_table(delete_table);
}

/* remove the wires and update the gate links properly */

void virtual_network_remove_wires(network) network_t *network;
{ bin_for_all_nodes_outputs_first(network, do_remove_wires); }

/* flattens the network: remove all but two sources */
/* of opposite polarity and update the gate links */
/* only one source if only one polarity is in use */

void virtual_network_flatten(network) network_t *network;
{
  bin_for_all_nodes_inputs_first(network, remove_buffers);
  bin_for_all_nodes_inputs_first(network, do_flatten);
}

/* remove a node from a virtual network */
/* do not check whether the nodes it used to feed are not */
/* still pointing towards it. This should be taken care of before */
/* recursively removing all nodes that have it as unique output */
/* if the flag is on, does it recursively */

void virtual_network_remove_node(node, rec_flag) node_t *node;
int rec_flag;
{
  int pin;
  gate_link_t link;

  if (node->type == PRIMARY_INPUT)
    return;
  if (node->type == PRIMARY_OUTPUT)
    return;
  if (MAP(node)->gate == NIL(lib_gate_t))
    return;

  for (pin = 0; pin < MAP(node)->ninputs; pin++) {
    node_t *input = MAP(node)->save_binding[pin];
    link.node = node;
    link.pin = pin;
    gate_link_remove(input, &link);
    if (rec_flag && gate_link_is_empty(input))
      virtual_network_remove_node(input, rec_flag);
  }
  MAP(node)->load = 0.0;
  MAP(node)->required = MINUS_INFINITY;
  MAP(node)->gate = NIL(lib_gate_t);
  MAP(node)->ninputs = 0;
  FREE(MAP(node)->save_binding);
  MAP(node)->save_binding = NIL(node_t *);
  gate_link_delete_all(node);
}

/* makes the node "source" source of the link */
/* and update the info at the other end */
/* be careful: if link->node == PRIMARY_OUTPUT, link->pin == -1 */

void virtual_network_add_to_gate_link(source, link) node_t *source;
gate_link_t *link;
{
  if (link->node->type == PRIMARY_OUTPUT) {
    if (MAP(link->node)->ninputs == 0) {
      MAP(link->node)->ninputs = 1;
      MAP(link->node)->save_binding = ALLOC(node_t *, 1);
    }
    MAP(link->node)->save_binding[0] = source;
  } else {
    MAP(link->node)->save_binding[link->pin] = source;
  }
  gate_link_put(source, link);
}

/* given new required time at node, update the gate links whose to-node is node
 */

void virtual_network_update_link_required_times(node, required) node_t *node;
delay_time_t *required;
{
  int i;
  node_t *input;
  gate_link_t link;

  link.node = node;
  switch (node->type) {
  case PRIMARY_INPUT:
    break;
  case PRIMARY_OUTPUT:
    if (node_num_fanin(node) == 0)
      return;
    input = map_po_get_fanin(node);
    link.pin = -1;
    assert(gate_link_get(input, &link));
    link.required = MAP(node)->required;
    gate_link_put(input, &link);
    break;
  case INTERNAL:
    assert(MAP(node)->gate != NIL(lib_gate_t));
    for (i = 0; i < MAP(node)->ninputs; i++) {
      input = MAP(node)->save_binding[i];
      link.pin = i;
      assert(gate_link_get(input, &link));
      link.required = required[i];
      gate_link_put(input, &link);
    }
    break;
  default:;
  }
}

/* INTERNAL INTERFACE */

/* the basic convention about gate links */
/* is that if a node is a primary output */
/* the gate_link for its unique input is (po,-1) */

static void virtual_network_setup_node_gate_links(node,
                                                  delete_table) node_t *node;
st_table *delete_table;
{
  int pin;
  node_t *input;
  gate_link_t link;

  if (node->type == PRIMARY_OUTPUT) {
    link.node = node;
    link.pin = -1;
    link.load = 0.0;
    link.required = MINUS_INFINITY;
    input = map_po_get_fanin(node);
    gate_link_put(input, &link);
    return;
  }
  if (MAP(node)->gate == NIL(lib_gate_t))
    return;
  if (gate_link_n_elts(node) == 0) {
    st_insert(delete_table, (char *)node, NIL(char));
    return;
  }
  for (pin = 0; pin < MAP(node)->ninputs; pin++) {
    link.node = node;
    link.pin = pin;
    link.load = 0.0;
    link.required = MINUS_INFINITY;
    input = MAP(node)->save_binding[pin];
    gate_link_put(input, &link);
  }
}

/* remove off the virtual network nodes mapped with wires */

static void do_remove_wires(node) node_t *node;
{
  node_t *input;
  gate_link_t link;
  if (MAP(node)->gate == NIL(lib_gate_t))
    return;
  if (strcmp(MAP(node)->gate->name, "**wire**") != 0)
    return;
  assert(!gate_link_is_empty(node));
  input = MAP(node)->save_binding[0];
  gate_link_first(node, &link);
  do {
    virtual_network_add_to_gate_link(input, &link);
  } while (gate_link_next(node, &link));
  virtual_network_remove_node(node, 0);
}

/* buffers may be introduced by fanout optimization in some cases */
/* to make the job of do_flatten easier, we first do one pass on the network */
/* to remove all the existing buffers and update the underlying and the virtual
 * network accordingly. */
/* Buffers are already supposed to have been introduced by fanout optimization:
 */
/* we check for that implicitly by asserting that all buffers at this stage have
 * been mapped */

static void remove_buffers(node) node_t *node;
{
  int i, n_fanouts;
  lsGen gen;
  node_t *fanin, *fanout;
  gate_link_t link;
  node_t **fanouts;

  if (node_function(node) != NODE_BUF)
    return;
#ifdef SIS
  if (lib_gate_type(lib_gate_of(node)) != COMBINATIONAL)
    return;
#endif /* SIS */
  assert(MAP(node)->gate != NIL(lib_gate_t) && MAP(node)->ninputs == 1);
  fanin = node_get_fanin(node, 0);
  assert(fanin == MAP(node)->save_binding[0]);

  /* remove gate_link between 'fanin' and 'node' */
  link.node = node;
  link.pin = 0;
  gate_link_remove(fanin, &link);

  /* move gate_links and save_binding pointers between 'node' and its fanouts to
   * 'fanin' */
  assert(!gate_link_is_empty(node));
  gate_link_first(node, &link);
  do {
    virtual_network_add_to_gate_link(fanin, &link);
  } while (gate_link_next(node, &link));

  /* update the underlying network */
  i = 0;
  n_fanouts = node_num_fanout(node);
  fanouts = ALLOC(node_t *, n_fanouts);
  foreach_fanout(node, gen, fanout) { fanouts[i++] = fanout; }
  for (i = 0; i < n_fanouts; i++) {
    node_patch_fanin(fanouts[i], node, fanin);
  }
  FREE(fanouts);
  network_delete_node(node_network(node), node);
}

/* there are two networks: the underlying and the virtual */
/* the underlying network is whatever is represented by the network_t data
 * structure */
/* the virtual network is represented by the MAP(node)->save_binding and
 * MAP(node)->gate_link */
/* the purpose of this routine is to perform some preprocessing to simplify
 * fanout optimization */
/* The basic idea is simple: for each node in the underlying network that is not
 * an inverter nor a PO */
/* we look at all the nodes in the virtual network which correspond to the same
 * signal as the */
/* output of that node (possibly of opposite polarity). If a big gate covers the
 * node in the virtual */
/* network, this set may be empty. If it is not empty, we put it as the source
 * of all the positive */
/* polarity signals, and pick up one node as a source for negative polarity
 * signals. */
/* This node is saved in MAP(node)->node_y and may be 0 if there is no negative
 * polarity sinks. */
/* All the other nodes mapped to inverters are removed from the virtual network.
 */

static void do_flatten(node) node_t *node;
{
  int p;
  lsGen gen;
  node_t *fanout;
  node_t *sources[2];
  st_table *nodes[2];

  switch (node_function(node)) {
  case NODE_UNDEFINED:
  case NODE_0:
  case NODE_1:
  case NODE_BUF:
  case NODE_PO:
  case NODE_INV:
    return;
  case NODE_PI:
  case NODE_AND:
  case NODE_OR:
  case NODE_COMPLEX:
#ifdef SIS
    foreach_fanout(node, gen, fanout) {
      switch (lib_gate_type(lib_gate_of(fanout))) {
      case COMBINATIONAL:
      case UNKNOWN:
        break;
      default:
        LS_ASSERT(lsFinish(gen));
        return;
      }
    }
#endif /* SIS */
    break;
  default:
    fail("unexpected node function");
    /* NOTREACHED */
  }

  /* select as sources[POLAR_Y] a mapped fanout if possible */
  /* may have multiple fanout even if fanin > 0 after the first pass of fanout
   * opt */
  sources[POLAR_X] = node;
  sources[POLAR_Y] = NIL(node_t);
  foreach_fanout(node, gen, fanout) {
    if (node_function(fanout) != NODE_INV)
      continue;
    if (sources[POLAR_Y] == NIL(node_t)) {
      sources[POLAR_Y] = fanout;
    } else if (MAP(sources[POLAR_Y])->gate == NIL(lib_gate_t)) {
      sources[POLAR_Y] = fanout;
    } else if (MAP(fanout)->gate != NIL(lib_gate_t) &&
               MAP(fanout)->ninputs > 1) {
      sources[POLAR_Y] = fanout;
    }
  }
  assert(sources[POLAR_Y] != NIL(node_t));
  MAP(sources[POLAR_X])->node_y = sources[POLAR_Y];

  /* extract nodes with nonempty gate link */
  /* make sure sources[POLAR_Y] is not left in these tables */
  foreach_polarity(p) { nodes[p] = st_init_table(st_numcmp, st_numhash); }
  extract_local_virtual_network(nodes, sources[POLAR_X]);
  st_delete(nodes[POLAR_Y], (char **)&sources[POLAR_Y], NIL(char *));

  foreach_polarity(p) {
    concentrate_gate_links_on_source(nodes[p], nodes[POLAR_INV(p)], sources[p]);
  }

  /*
   * If no logic duplication is allowed, make sure that one of the sources is
   * external the other NIL or an inverter to the former. If logic duplication
   * is allowed, keep both external sources.
   */

  make_sources_external(sources, nodes);
  foreach_polarity(p) { flatten_check_consistency(sources[p]); }

  /* cleanup */
  foreach_polarity(p) { remove_intermediate_nodes(nodes[p]); }
  foreach_polarity(p) { st_free_table(nodes[p]); }
}

static void extract_local_virtual_network(nodes, source) st_table **nodes;
node_t *source;
{
  lsGen gen;
  node_t *fanout;

  foreach_fanout(source, gen, fanout) {
    extract_local_virtual_network_rec(nodes, fanout, POLAR_X);
  }
}

static void extract_local_virtual_network_rec(nodes, node,
                                              polarity) st_table **nodes;
node_t *node;
int polarity;
{
  lsGen gen;
  node_t *fanout;
  int node_polarity;

  if (node_function(node) != NODE_INV)
    return;
  node_polarity = POLAR_INV(polarity);
  if (MAP(node)->gate != NIL(lib_gate_t))
    st_insert(nodes[node_polarity], (char *)node, NIL(char));
  foreach_fanout(node, gen, fanout) {
    extract_local_virtual_network_rec(nodes, fanout, node_polarity);
  }
}

static void make_sources_external(sources, nodes) node_t **sources;
st_table **nodes;
{
  int p;
  int is_empty[2];
  int is_source[2];
  node_t *source = sources[POLAR_X];

  foreach_polarity(p) {
    is_empty[p] = flatten_is_empty(sources[p], nodes[POLAR_INV(p)]);
    is_source[p] = flatten_is_source(sources[p]);
  }

  /*
   * There is always a sources[POLAR_Y] at this point.
   * If sources[POLAR_Y] is not mapped to anything, since it cannot possibly be
   * a PI or a constant node, sources[POLAR_X] has to be source (or everything
   * is empty). If there is a sink of polarity POLAR_Y, we set up an inverter on
   * sources[POLAR_Y] to maintain network consistency (i.e. we could stop after
   * do_flatten and get a consistently annotated network). If there is no sink
   * of polarity POLAR_Y, we do not need to do anything.
   */

  assert(sources[POLAR_Y] != NIL(node_t));
  if (MAP(sources[POLAR_Y])->gate == NIL(lib_gate_t)) {
    assert(is_source[POLAR_X] || (is_empty[POLAR_X] && is_empty[POLAR_Y]));
    if (is_empty[POLAR_Y])
      return;
    virtual_setup_inverter(sources[POLAR_X], sources[POLAR_Y]);
    return;
  }

  /*
   * if sources[POLAR_Y] is mapped to an inverter, keep it
   */

  if (MAP(sources[POLAR_Y])->ninputs == 1) {
    assert(!is_empty[POLAR_X] && is_source[POLAR_X]);
    assert(MAP(sources[POLAR_Y])->save_binding[0] == source);
    return;
  }

  /*
   * If sources[POLAR_X] is not mapped to anything
   * if there are sinks of the same polarity, add a inverter
   * rooted at sources[POLAR_Y]. Otherwise, nothing to do.
   */

  if (MAP(source)->gate == NIL(lib_gate_t)) {
    if (is_empty[POLAR_X])
      return;
    virtual_setup_inverter(sources[POLAR_Y], sources[POLAR_X]);
    return;
  }

  if (global.allow_duplication || global.allow_internal_fanout) {
    return;
  } else {
    assert(!is_source[POLAR_X]);
    assert(MAP(source)->ninputs == 1);
    assert(!is_empty[POLAR_X]);
    assert(MAP(source)->save_binding[0] == sources[POLAR_Y]);
    return;
  }
}

/*
 * is empty iff (1) or (2):
 * (1) there is no sink hooked to it
 * (2) the only nodes hooked to it are in table (nodes local to the network of
 * inverters) the other source is not in that table, so it counts as non empty
 * if there.
 */

static int flatten_is_empty(node, table) node_t *node;
st_table *table;
{
  int is_empty = 1;
  gate_link_t link;

  if (node == NIL(node_t))
    return 1;
  if (gate_link_is_empty(node))
    return 1;
  gate_link_first(node, &link);
  do {
    if (!st_lookup(table, (char *)link.node, NIL(char *)))
      is_empty = 0;
  } while (gate_link_next(node, &link));
  return is_empty;
}

/*
 * a source is anything that can generate a signal.
 * Can be a constant node, a PI, or a node mapped with a complex gate.
 * NIL node, unmapped nodes, nodes mapped with a buffer or an inverter are out.
 */

static int flatten_is_source(source) node_t *source;
{
  node_function_t node_fn;

  if (source == NIL(node_t))
    return 0;
  if (MAP(source)->ninputs > 1)
    return 1;
  node_fn = node_function(source);
  if (node_fn == NODE_PI || node_fn == NODE_0 || node_fn == NODE_1)
    return 1;
  return 0;
}

static void virtual_setup_inverter(source, dest) node_t *source;
node_t *dest;
{
  gate_link_t link;

  assert(MAP(dest)->gate == NIL(lib_gate_t));
  MAP(dest)->gate = lib_get_default_inverter();
  MAP(dest)->ninputs = 1;
  MAP(dest)->save_binding = ALLOC(node_t *, 1);
  MAP(dest)->save_binding[0] = source;
  link.node = dest;
  link.pin = 0;
  link.load = 0.0;
  link.required = MINUS_INFINITY;
  gate_link_put(source, &link);
}

static void concentrate_gate_links_on_source(nodes, inv_nodes,
                                             source) st_table *nodes;
st_table *inv_nodes;
node_t *source;
{
  node_t *node;
  char *value;
  st_generator *gen;
  gate_link_t link;

  if (st_count(nodes) == 0)
    return;
  assert(source != NIL(node_t));

  st_foreach_item(nodes, gen, (char **)&node, &value) {
    gate_link_first(node, &link);
    do {
      if (st_lookup(inv_nodes, (char *)link.node, NIL(char *)))
        continue;
      virtual_network_add_to_gate_link(source, &link);
    } while (gate_link_next(node, &link));
  }
}

static void remove_intermediate_nodes(nodes) st_table *nodes;
{
  node_t *node;
  char *dummy;
  st_generator *gen;

  st_foreach_item(nodes, gen, (char **)&node, &dummy) {
    virtual_network_remove_node(node, 1);
  }
}

/* case of PRIMARY_INPUT going nowhere: there may be a single inverter attached
 * to it */
/* that itself goes nowhere. This is due to the trick of adding inverters all
 * over the place */

static void flatten_check_consistency(node) node_t *node;
{
  int alive;
  int empty_fanout;
  node_t *fanout;

  if (node == NIL(node_t))
    return;

  alive = (node->type == PRIMARY_INPUT || MAP(node)->gate != NIL(lib_gate_t));
  empty_fanout = gate_link_is_empty(node);
  if ((alive && !empty_fanout) || (!alive && empty_fanout))
    return;
  if (global.allow_duplication)
    return;

  assert(node->type == PRIMARY_INPUT);
  assert(node_num_fanout(node) == 1);
  fanout = node_get_fanout(node, 0);
  assert(node_num_fanout(fanout) == 0);
}

void virtual_network_check(network) network_t *network;
{
  lsGen gen;
  node_t *node;

  foreach_node(network, gen, node) { virtual_network_check_node(node); }
}

static void virtual_network_check_node(node) node_t *node;
{
  switch (node->type) {
  case INTERNAL:
    if (MAP(node)->gate != NIL(lib_gate_t)) {
      check_fanin(node);
      check_fanout(node);
    }
    break;
  case PRIMARY_INPUT:
    check_fanout(node);
    break;
  case PRIMARY_OUTPUT:
    check_fanin(node);
    break;
  default:
    fail("unexpected node type");
    break;
  }
}

static void check_fanout(node) node_t *node;
{
  gate_link_t link;
  assert(!gate_link_is_empty(node));
  gate_link_first(node, &link);
  do {
    if (link.node->type == PRIMARY_OUTPUT) {
      assert(node == map_po_get_fanin(link.node));
    } else {
      assert(MAP(link.node)->gate != NIL(lib_gate_t));
      assert(node == MAP(link.node)->save_binding[link.pin]);
    }
  } while (gate_link_next(node, &link));
}

static void check_fanin(node) node_t *node;
{
  int i;
  node_t *fanin;
  gate_link_t link;

  if (node->type == PRIMARY_OUTPUT) {
    fanin = map_po_get_fanin(node);
    link.node = node;
    link.pin = -1;
    assert(gate_link_get(fanin, &link));
    return;
  }
  for (i = 0; i < MAP(node)->ninputs; i++) {
    fanin = MAP(node)->save_binding[i];
    link.node = node;
    link.pin = i;
    assert(gate_link_get(fanin, &link));
  }
}
