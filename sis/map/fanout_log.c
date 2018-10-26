
/* file @(#)fanout_log.c	1.2 */
/* last modified on 5/1/91 at 15:50:38 */
#include "sis.h"
#include "map_macros.h"
#include "map_int.h"
#include "fanout_int.h"
#include "gate_link.h"

 /* this file keeps a log of all node creations during fanout optimization and undo them */
 /* nodes are created bottom up; so they can be destroyed in that order as well */

static void unmap_node();

static bin_global_t global;
static array_t *node_log;

 /* EXTERNAL INTERFACE */

void fanout_log_init(options)
bin_global_t *options;
{
  global = *options;
  if (global.fanout_log_on) {
    assert(node_log == NIL(array_t));
    node_log = array_alloc(node_t *, 0);
  }
}

 /* remove all nodes in the log in order */
 /* then scan the network and remove all MAP annotations */

void fanout_log_cleanup_network(network)
network_t *network;
{
  int i;
  node_t *node;
  lsGen gen;

  assert(global.fanout_log_on);
  for (i = array_n(node_log) - 1; i >= 0; i--) {
    node = array_fetch(node_t *, node_log, i);
    unmap_node(node);
    assert(node_num_fanout(node) == 0);
    network_delete_node(network, node);
  }
  array_free(node_log);
  node_log = NIL(array_t);
  foreach_node(network, gen, node) {
    unmap_node(node);
  }
}

void fanout_log_register_node(node)
node_t *node;
{
  if (global.fanout_log_on) {
    array_insert_last(node_t *, node_log, node);
  }
}


 /* INTERNAL INTERFACE */

static void unmap_node(node)
node_t *node;
{
  node_function_t node_fn;

  node_fn = node_function(node);
  if (node_fn == NODE_PI || node_fn == NODE_0 || node_fn == NODE_1) return;
  if (MAP(node)->ninputs > 0) FREE(MAP(node)->save_binding);
  MAP(node)->gate = NIL(lib_gate_t);
  MAP(node)->ninputs = 0;
  MAP(node)->save_binding = NIL(node_t *);
}
