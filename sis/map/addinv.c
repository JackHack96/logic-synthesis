
/* file @(#)addinv.c	1.4 */
/* last modified on 9/17/91 at 14:29:24 */
#include "sis.h"
#include "map_int.h"


static node_t *
create_inverter(nodelist, fanin)
lsList nodelist;
node_t *fanin;
{
    node_t *inv;

    inv = node_alloc();
    node_replace(inv, node_literal(fanin, 0));
    LS_ASSERT(lsNewEnd(nodelist, (char *) inv, LS_NH));
    return inv;
}


static int
feeds_po(node)
node_t *node;
{
    lsGen gen;
    node_t *fanout;

    foreach_fanout(node, gen, fanout) {
	if (fanout->type == PRIMARY_OUTPUT) {
	    LS_ASSERT(lsFinish(gen));
	    return 1;
	}
    }
    return 0;
}

static int
feeds_latch(node)
node_t *node;
{
#ifndef SIS
  return 0;
#else 
  lsGen gen;
  node_t *fanout;

  foreach_fanout(node, gen, fanout) {
    if (fanout->type == PRIMARY_OUTPUT) {
      if (latch_from_node(fanout) != NIL(latch_t)) {
	LS_ASSERT(lsFinish(gen));
	return 1;
      }
    }
  }
  return 0;
#endif /* SIS */
}

static void
add_inverters(nodelist, node, deleted)
lsList nodelist;
node_t *node;
st_table *deleted;
{
  int i;
  node_t *inv1, *inv2, *inv, *fanout, *other_inv;
  array_t *output_inv;
  lsGen gen;

  /* Find if the node already has an inverter at the output */
  output_inv = array_alloc(node_t *, 0);
  foreach_fanout(node, gen, fanout) {
    if (node_function(fanout) == NODE_INV) {
      array_insert_last(node_t *, output_inv, fanout);
    }
  }

  /* Create an inverter if none found so far */
  if (array_n(output_inv) == 0) {
    inv = create_inverter(nodelist, node);
  } else {
    inv = array_fetch(node_t *, output_inv, 0);
  }

  /* Handle the positive-phase fanouts (i.e., fanouts of 'node') */
  foreach_fanout(node, gen, fanout) {
    if (node_function(fanout) != NODE_INV) {
      inv1 = create_inverter(nodelist, inv);
      assert(node_patch_fanin(fanout, node, inv1));
    }
  }

  /* Handle the negative-phase fanouts (i.e., fanouts of original inverter) */
  /* and put them as fanout of 'inv' */
 /* all inverter nodes but 'inv' are then removed */
  for (i = 0; i < array_n(output_inv); i++) {
    other_inv = array_fetch(node_t *, output_inv, i);
    foreach_fanout(other_inv, gen, fanout) {
      if (node_function(fanout) != NODE_INV && fanout->type != PRIMARY_OUTPUT) {
	inv1 = create_inverter(nodelist, inv);
	inv2 = create_inverter(nodelist, inv1);
	assert(node_patch_fanin(fanout, other_inv, inv2));
      } else if (other_inv != inv) {
	assert(node_patch_fanin(fanout, other_inv, inv));
      }
    }
    if (other_inv != inv) {
      assert(node_num_fanout(other_inv) == 0);
      assert(node_function(other_inv) == NODE_INV);
      st_insert(deleted, (char *) other_inv, NIL(char));
    }
  }
  array_free(output_inv);
}

void 
map_add_inverter(network, add_at_pipo)
network_t *network;
int add_at_pipo;
{
  lsList nodelist;
  st_table *deleted;
  node_t *node;
  int need_inv;
  lsGen gen;
  st_generator *del_gen;

  nodelist = lsCreate();
  deleted = st_init_table(st_ptrcmp, st_ptrhash);

  foreach_node(network, gen, node) {
    if (st_lookup(deleted, (char *) node, NIL(char *))) continue;
    switch(node_function(node)) {
    case NODE_PI:
      need_inv = node_num_fanout(node) > 1 || add_at_pipo;
      break;
    case NODE_PO:
    case NODE_INV:
      need_inv = 0;
      break;
    case NODE_0:
    case NODE_1:
      need_inv = 0;
      break;
    case NODE_AND:
    case NODE_OR:
      /* special case for single-fanout outputs on primitives */
      if (node_num_fanout(node) == 1 && feeds_po(node)) {
	need_inv = feeds_latch(node)?0:add_at_pipo;
      } else {
	need_inv = 1;
      }
     break;
    default:
      fail("map_add_inverter: what is this doing here ?");
      /* NOTREACHED */
    }
    if (need_inv) {
      add_inverters(nodelist, node, deleted);
    }
  }

  lsForeachItem(nodelist, gen, node) {
    network_add_node(network, node);
  }
  st_foreach_item(deleted, del_gen, (char **) &node, NIL(char *)) {
    network_delete_node(node->network, node);
  }
  st_free_table(deleted);
  LS_ASSERT(lsDestroy(nodelist, (void (*)()) 0));
}

void
map_remove_inverter(network, report_data)
network_t *network;
VoidFn report_data;
{
  int i, j, num_fanout;
  node_t *inv1, *inv2, *node, *fanout, *fanout1;
  node_t **fanouts;
  array_t *nodes;
  lsGen gen, gen1;

  if (report_data) {
    fprintf(sisout, ">>> before removing serial inverters <<<\n"); 
    (*report_data)(network);
  }
  /* step 1: check for sequential inverters */
  nodes = network_dfs(network);
  for(i = 0; i < array_n(nodes); i++) {
    inv2 = array_fetch(node_t *, nodes, i);
    if (node_function(inv2) == NODE_INV) {

      /* see if the fanin is also an inverter ... */
      inv1 = node_get_fanin(inv2, 0);
      if (node_function(inv1) == NODE_INV) {
	node = node_get_fanin(inv1, 0);
	num_fanout = node_num_fanout(inv2);
	fanouts = ALLOC(node_t *, num_fanout);
	j = 0;
	foreach_fanout(inv2, gen, fanout) {
	  fanouts[j++] = fanout;
	}
	for (j = 0; j < num_fanout; j++) {
	  assert(node_patch_fanin(fanouts[j], inv2, node));
	}
	FREE(fanouts);
	network_delete_node(network, inv2);
      }
    }
  }
  array_free(nodes);
  if (report_data) {
    fprintf(sisout, ">>> before removing parallel inverters <<<\n"); 
    (*report_data)(network);
  }
  /* step 2: check for parallel inverters */
  nodes = network_dfs_from_input(network);
  for(i = 0; i < array_n(nodes); i++) {
    node = array_fetch(node_t *, nodes, i);
    if (node_function(node) != NODE_INV) {

      /* find if more than 2 inverters fanout from 'node' */
      inv1 = NIL(node_t);
      foreach_fanout(node, gen, fanout) {
	if (node_function(fanout) == NODE_INV) {
	  if (inv1 == NIL(node_t)) {
	    inv1 = fanout;
	  } else {
	    /* move fanouts of 'fanout' to fanouts of inv1 */
	    num_fanout = node_num_fanout(fanout);
	    fanouts = ALLOC(node_t *, num_fanout);
	    j = 0;
	    foreach_fanout(fanout, gen1, fanout1) {
	      fanouts[j++] = fanout1;
	    }
	    for (j = 0 ; j < num_fanout; j++) {
	      assert(node_patch_fanin(fanouts[j], fanout, inv1));
	    }
	    FREE(fanouts);
	    network_delete_node(network, fanout);
	  }
	}
      }
    }
  }
  array_free(nodes);

  /* cleanup dangling nodes  (?) -- are there any ?? */
  (void) network_cleanup(network);
}
