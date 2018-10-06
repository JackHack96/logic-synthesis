
#ifdef SIS
#include "retime_int.h"
#include "sis.h"

typedef struct retime_fanout_struct {
  node_t *fanout;
  int fanin_id;
  int weight;
} re_fanout_t;

typedef struct retime_array_struct {
  array_t *bfs;
  st_table *visited;
} re_data_t;

static void retime_set_gate();
static void retime_add_latches();
static void retime_share_latches();
static void retime_add_latches_to_network();
static void retime_assure_init_values();
static node_t *retime_get_latch_input();
static array_t *retime_gen_weights();
static latch_t **re_gen_latches();
static void retime_add_bfs_nodes();
static void re_patch_fanouts();

enum st_retval re_table_to_array();

#define is_feedback_latch(g) (lib_gate_latch_pin((g)) != -1)

/* ARGSUSED */
/*
 * We assume the following ---
 *
 * The user has made sure that the network is one that is retimable i.e.
 * the routine   "retime_is_network_retimable()" should return 1
 *
 * if (not_mapped) then no buffers between PI and PO are used.. use the value
 *     delay_r to specify the i/p o/p delay thru the latch
 * if (mapped) then annotate the node in the retime graph with the delay thru
 *     the latch (ignore the delay_r parameter)..
 *
 * REMEMBER::: In cases where cycles consisting of only latches are found
 *           some nodes are added to the network, Remember to delete these
 *           using the routine 	"re_delete_temp_nodes()" !!!!
 */
re_graph *retime_network_to_graph(network_t *network, int size, int use_mapped)
{
  lsGen gen;
  re_graph *graph;
  st_table *l_table; /* Table of all the latches that must be traversed */
  st_table *node_to_id_table; /* Stores correp between nodes and id */
  st_generator *stgen;
  char *dummy, name[128];
  latch_synch_t type, s_type;
  latch_t *latch;
  lib_gate_t *d_latch = NIL(lib_gate_t);
  int i, n, index;
  re_node *re_no;
  node_t *node, *temp, **fo_array, *cntrl, *control = NIL(node_t);
  re_data_t latch_end_data;
  array_t *nodevec, *temp_array;

  /* Allocation & initializations */
  s_type = UNKNOWN;
  graph = re_graph_alloc();
  node_to_id_table = st_init_table(st_ptrcmp, st_ptrhash);

  nodevec = array_alloc(node_t *, 0);
  foreach_primary_input(network, gen, node) {
    if (network_latch_end(node) != NIL(node_t)) {
      /* Keep the synchronization type around */
      latch = latch_from_node(node);
      if ((type = latch_get_type(latch)) != UNKNOWN) {
        s_type = type;
      }
      if ((cntrl = latch_get_control(latch)) != NIL(node_t)) {
        control = cntrl;
      }
    }
  }
  graph->s_type = s_type; /* For use in converting the re_graph to netw */
  if (control != NIL(node_t)) {
    /* The name of the signal stored is that of the fanin */
    graph->control_name =
        util_strsav(node_long_name(node_get_fanin(control, 0)));
  }

  /*
   * check if an appropriate d-latch is present: This is essential since
   * we require the generic latch so that we can move latches
   */
  if (use_mapped) {
    d_latch = lib_choose_smallest_latch(lib_get_library(), "f=a;", s_type);
    if (d_latch == NIL(lib_gate_t)) {
      (void)fprintf(siserr, "%s D-FlipFlop not present in library\n",
                    (s_type == RISING_EDGE ? "rising edge" : "falling_edge"));
      return NIL(re_graph);
    }
  }

  foreach_node(network, gen, node) {
    /*
     * Do not add PI/PO nodes corresponding to latches or nodes that
     * correspond to the logic of the D-FlipFlops, or to control nodes of
     * latches, or internal nodes that are part of mapped D-latches
     */
    if (network_latch_end(node) != NIL(node_t) ||
        network_is_control(network, node) ||
        (node->type == INTERNAL && use_mapped && lib_gate_of(node) == d_latch))
      continue;
    re_graph_add_node(graph, node, use_mapped, d_latch, node_to_id_table);
  }

  /*
   * Starting from the inputs start adding edges in a breadth
   * first manner. Take care when wrapping around due to latches
   * Keep a table of latches and delete the latches that are traversed.
   * Then we can find out if all lathes are traversed or not....
   */
  foreach_node(network, gen, node) {
    if (network_latch_end(node) == NIL(node_t) && node_num_fanin(node) == 0) {
      array_insert_last(node_t *, nodevec, node);
    }
  }
  l_table = st_init_table(st_ptrcmp, st_ptrhash);
  foreach_latch(network, gen, latch) {
    (void)st_insert(l_table, (char *)latch, NIL(char));
  }

  /* Add the edges encountered starting from the inputs */
  retime_add_bfs_nodes(graph, nodevec, l_table, node_to_id_table, d_latch);
  array_free(nodevec);

  /*
   * In pathological case there may be sections of the circuit left out---
   * those that have no inputs. These
   * we traverse starting from the nodes that are outputs of latches
   */
  if (st_count(l_table) > 0) {
    latch_end_data.bfs = array_alloc(node_t *, 0);
    latch_end_data.visited = st_init_table(st_ptrcmp, st_ptrhash);
    st_foreach(l_table, re_table_to_array, (char *)&latch_end_data);
    retime_add_bfs_nodes(graph, latch_end_data.bfs, l_table, node_to_id_table,
                         d_latch);
    array_free(latch_end_data.bfs);
    st_free_table(latch_end_data.visited);
  }
  /* At this stage in really pathological cases in which cycles of
   * latches exist without any logic there are latches left unvisited.
   * Since the retime graph has no representation for such cycles
   * (vertices have to exist for weighted edges to exist between them)
   */
  if (st_count(l_table) > 0) {
    /*
     * !!!!	CYCLE WITH NO COMBINATIONAL GATES EXISTS !!!!
     * Add gates temporarily to the network so that vertices in
     * the retime graph can be defined. These vertices are special
     */
    temp_array = array_alloc(node_t *, 0);

    st_foreach_item(l_table, stgen, (char **)&latch, (char **)&dummy) {
      node = latch_get_output(latch);
      if ((n = node_num_fanout(node)) > 1) {
        /* Record the current fanouts of node */
        i = 0;
        fo_array = ALLOC(node_t *, n);
        foreach_fanout(node, gen, temp) { fo_array[i++] = temp; }
        /* Add buffer;  Put a ' ' in  name to recognize it later */
        temp = node_literal(node, 0);
        network_add_node(network, temp);
        sprintf(name, "%s temp", temp->name);
        network_change_node_name(network, temp, util_strsav(name));
        /* Patch the fanouts of latch output to the buffer */
        for (i = n; i-- > 0;) {
          node_patch_fanin(fo_array[i], node, temp);
        }
        FREE(fo_array);

        array_insert_last(node_t *, temp_array, temp);
        re_graph_add_node(graph, temp, use_mapped, d_latch, node_to_id_table);
        /* Set the area and delay of the buffer to be 0.0 */
        assert(st_lookup_int(node_to_id_table, (char *)temp, &index));
        re_no = re_get_node(graph, index);
        re_no->final_delay = 0.0;
        re_no->final_area = 0.0;
      }
    }

    retime_add_bfs_nodes(graph, temp_array, l_table, node_to_id_table, d_latch);

    array_free(temp_array);
  }

  assert(st_count(l_table) == 0);

  st_free_table(l_table);
  st_free_table(node_to_id_table);
  return (graph);
}

/*
 * Routine called to generate the fanouts of the latches so as to
 * be able to carry on the traversal of components that dont have
 * inputs
 */

/* ARGSUSED */
enum st_retval re_table_to_array(char *key, char *value, char *arg)
{
  lsGen gen;
  node_t *node, *fo;
  latch_t *latch = (latch_t *)key;
  re_data_t *data = (re_data_t *)arg;

  node = latch_get_output(latch);
  foreach_fanout(node, gen, fo) {
    if (fo->type == INTERNAL &&
        !st_insert(data->visited, (char *)fo, NIL(char))) {
      array_insert_last(node_t *, data->bfs, fo);
    }
  }

  return ST_CONTINUE;
}

/*
 * This routine takes the nodes in "nodevec" and then will do a
 * breadth_first_traversal and add the nodes it encounters
 */
static void retime_add_bfs_nodes(re_graph *graph, array_t *nodevec, st_table *l_table, st_table *node_to_id_table, lib_gate_t *d_latch)
{

  array_t *fan;
  node_t *node;
  re_edge *re_ed;
  st_table *add_table; /* Stores tempory data during traversal */
  re_fanout_t *fanout;
  int i, j, first, last;

  /* The actual breadth first traversal */
  first = 0;
  add_table = st_init_table(st_ptrcmp, st_ptrhash);
  do {
    last = array_n(nodevec);
    for (i = first; i < last; i++) {
      node = array_fetch(node_t *, nodevec, i);
      if (!st_is_member(add_table, (char *)node)) {
        /* Generate the fanouts and the weights if not visited */
        fan = retime_gen_weights(node, d_latch);

        for (j = array_n(fan); j-- > 0;) {
          fanout = array_fetch(re_fanout_t *, fan, j);
          if (!st_is_member(add_table, (char *)(fanout->fanout))) {
            array_insert_last(node_t *, nodevec, fanout->fanout);
          }
          /*
           * Now generate the latches from the fanout to node
           */
          re_ed = re_graph_add_edge(graph, node, fanout->fanout, fanout->weight,
                                    1.0 /*breadth=1*/, fanout->fanin_id,
                                    node_to_id_table);
          re_ed->latches = re_gen_latches(fanout->fanout, fanout->fanin_id,
                                          fanout->weight, l_table, d_latch);

          /*
           * To check the translation etc. I want to put the initial
           * values in the initial_values filed at this stage
           */

          /*
          if (re_ed->weight > re_ed->num_val_alloc){
          re_ed->num_val_alloc = re_ed->weight;
          re_ed->initial_values = REALLOC(int,
              re_ed->initial_values, re_ed->num_val_alloc);
          }
          for (k = re_ed->weight; k-- > 0; ) {
          re_ed->initial_values[k] =
              latch_get_initial_value(re_ed->latches[k]);
          }
          */
          FREE(fanout);
        }
        (void)st_insert(add_table, (char *)node, NIL(char));
        array_free(fan);
      }
    }
    first = last;
  } while (first < array_n(nodevec));
  st_free_table(add_table);
}

/*
 * Generate the number of registers between the node
 * and its fanouts. Take care since primary outputs of the
 * network may be true po or inputs to latches. Also insert the
 * initial values of the latches in the fanout structure.
 */
static array_t *retime_gen_weights(node_t *node, lib_gate_t *d_latch)
{
  lsGen gen;
  char *dummy;
  st_table *table;
  re_fanout_t *new_fan;
  node_t *fo, *end, *np;
  array_t *fan, *nodevec;
  int i, pin, first, last, depth, more_to_come;

  fan = array_alloc(re_fanout_t *, 0);

  if (node->type == PRIMARY_OUTPUT && (network_latch_end(node) != NIL(node_t)))
    return (fan);
  if (node->type == PRIMARY_INPUT && (network_latch_end(node) != NIL(node_t)))
    return (fan);

  table = st_init_table(st_ptrcmp, st_ptrhash);
  nodevec = array_alloc(node_t *, 0);
  (void)st_insert(table, (char *)node, (char *)0);
  array_insert_last(node_t *, nodevec, node);

  more_to_come = TRUE;
  first = 0;
  while (more_to_come) {
    more_to_come = FALSE;
    last = array_n(nodevec);
    for (i = first; i < last; i++) {
      np = array_fetch(node_t *, nodevec, i);
      if (st_lookup(table, (char *)np, &dummy)) {
        depth = (int)dummy;
      } else {
        (void)fprintf(sisout, "Hey !! node not in table \n");
        continue;
      }

      if (np->type == PRIMARY_OUTPUT) {
        /*
         * A primary o/p at this stage is a latch end
         * or a true PO which has no fanouts anyway in which
         * case no work needs to be done
         */
        if ((end = network_latch_end(np)) != NIL(node_t)) {
          if (st_lookup(table, (char *)np, &dummy)) {
            depth = (int)dummy;
            if (!st_is_member(table, (char *)end)) {
              more_to_come = TRUE;
              array_insert_last(node_t *, nodevec, end);
              (void)st_insert(table, (char *)end, (char *)(depth + 1));
            }
          } else {
            (void)fprintf(sisout, "Hey !! po not in table \n");
          }
        }
      } else {
        foreach_fanout_pin(np, gen, fo, pin) {
          end = retime_network_latch_end(fo, d_latch);
          if (end == NIL(node_t)) {
            /* Check if it is a dummy PO for control of latches */
            if (network_is_control(node_network(node), fo))
              continue;
            /* This is a fanout of the node */
            new_fan = ALLOC(re_fanout_t, 1);
            new_fan->fanout = fo;
            new_fan->fanin_id = pin;
            new_fan->weight = depth;
            array_insert_last(re_fanout_t *, fan, new_fan);
          } else {
            /* latch i/p  -- Add to list being processed */
            more_to_come = TRUE;
            array_insert_last(node_t *, nodevec, fo);
            (void)st_insert(table, (char *)fo, (char *)depth);
          }
        }
      }
    }
    first = last;
  }
  array_free(nodevec);
  st_free_table(table);
  return (fan);
}

/* Utility routine
 * If node is the input of a latch, it will generate its o/p else NIL
 */
node_t *retime_network_latch_end(node_t *node, lib_gate_t *d_latch)
{
  node_t *po, *end;

  if ((end = network_latch_end(node)) == NIL(node_t) &&
      d_latch != NIL(lib_gate_t) && node->type == INTERNAL &&
      lib_gate_of(node) == d_latch) {
    po = node_get_fanout(node, 0);
    end = network_latch_end(po);
    assert(end != NIL(node_t) && po->type == PRIMARY_OUTPUT);
  }
  return end;
}

/*
 * Utility: Move the fanouts of "from" to be fanouts of the "to" node
 */
static void re_patch_fanouts(node_t *from, node_t *to){
  int j = 0, n;
  lsGen gen;
  node_t **fo_array, *fanout;

  n = node_num_fanout(from);
  fo_array = ALLOC(node_t *, n);
  foreach_fanout(from, gen, fanout) { fo_array[j++] = fanout; }
  for (j = n; j-- > 0;) {
    node_patch_fanin(fo_array[j], from, to);
  }
  FREE(fo_array);
}
/*
 * Routine to delete the temoporary nodes added to make sure that a
 * retime graph can be generated from the network in case cycles with
 * only latches are encountered in the circuit...
 */
void re_delete_temp_nodes(network_t *network)
{
  lsGen gen;
  node_t *node, *temp;

  foreach_node(network, gen, node) {
    if (node->type != INTERNAL)
      continue;
    if (index(node->name, ' ') == NULL)
      continue;
    temp = node_get_fanin(node, 0);
    re_patch_fanouts(node, temp); /* move fanouts from node to temp */
    network_delete_node_gen(network, gen);
  }
}

/*
 * If use_mapped is set... then we need to get the appropriate dff from the
 * library and inset them. Furthermore for the original mapped latches
 * in the network, we need to ensure that the latches are captured correctly
 */
network_t *retime_graph_to_network(re_graph *graph, int use_mapped)
{
  char *name;
  int i, j, k;
  lsGen gen, gen2;
  latch_t *latch;
  re_edge *edge;
  re_node *re_no;
  st_table *node_to_id_table; /* Need to maintain correspondence */
  st_table *gate_table;
  extern void delay_dup(); /* Should actually be in delay.h */
  array_t *fo_array, *nodes, *orig_po_array, *fanin_array;
  network_t *netw, *netw1, *network;
  lib_gate_t *gate, *d_latch;
  node_t *temp, *new_buffer, *node, *dup_node, *po, *pi, *control;
  node_t *root_node, *fanin, *fanin_node, *fo, *old_fanin;

  extern void map_invalid();                   /* belongs to map_int.h */
  extern array_t *network_and_node_to_array(); /* belongs to speed_int.h */

  if (use_mapped) {
    /* Get the latch from the library so as to annotate the latches */
    d_latch =
        lib_choose_smallest_latch(lib_get_library(), "f=a;", graph->s_type);
    if (d_latch == NIL(lib_gate_t)) {
      error_append("No D flip-flop present in library\n");
      return NIL(network_t);
    }
  } else {
    d_latch = NIL(lib_gate_t);
  }
  network = network_alloc();
  node_to_id_table = st_init_table(st_ptrcmp, st_ptrhash);

  re_foreach_node(graph, i, re_no) {
    if (re_no->type != RE_PRIMARY_OUTPUT && re_no->type != RE_IGNORE) {
      dup_node = node_dup(re_no->node);
      if (!use_mapped)
        map_invalid(dup_node);
      re_no->node->copy = dup_node;
      (void)st_insert(node_to_id_table, (char *)dup_node, (char *)(re_no->id));

      if (re_no->type == RE_PRIMARY_INPUT) {
        network_add_primary_input(network, dup_node);
      } else {
        /*
         * Next need to set the implementation as the one desired
         * The duplicate node is the key to the retime node in the
         * cache
         */
        network_add_node(network, dup_node);
        if (use_mapped)
          retime_set_gate(dup_node, graph, node_to_id_table);
      }

      /* Retain correspondence of primary o/p nodes */
      re_foreach_fanout(re_no, j, edge) {
        if (re_ignore_edge(edge))
          continue;
        if (edge->sink->type == RE_PRIMARY_OUTPUT) {
          if (graph->control_name != NIL(char) &&
              !strcmp(node_long_name(dup_node), graph->control_name))
            continue;
          po = network_add_primary_output(network, dup_node);
          /* JCM - don't change name of primary inputs!!! */
          if (re_no->type == RE_PRIMARY_INPUT)
            network_swap_names(network, dup_node, po);
          network_change_node_name(
              network, po, util_strsav(node_long_name(edge->sink->node)));
          if (edge->weight > 0) {
            fanin_remove_fanout(po);
            retime_assure_init_values(edge);
            retime_add_latches_to_network(network, dup_node, po, 0,
                                          edge->weight, d_latch,
                                          edge->initial_values);
            fanin_add_fanout(po);
          }
        }
      }
    }
  }

  /*
   * For the +ve weight edges add appropriate latches . Also
   * patch the pointers.
   */
  re_foreach_node(graph, i, re_no) {
    if (re_no->type == RE_INTERNAL) {
      fanin_remove_fanout(re_no->node->copy);
      re_foreach_fanin(re_no, j, edge) {
        if (re_ignore_edge(edge))
          continue;
        if (edge->weight == 0) {
          /*
           * Check to make sure that the fanin ptrs are fine,
           * Required since the graph can contain cycles
           */
          dup_node = re_no->node->copy;
          k = edge->sink_fanin_id;
          old_fanin = node_get_fanin(dup_node, k);
          fanin = edge->source->node->copy;
          if (old_fanin != fanin) {
            dup_node->fanin[k] = fanin;
          }
        } else {
          retime_add_latches(network, edge, d_latch);
        }
      }
      fanin_add_fanout(re_no->node->copy);
    }
  }
  st_free_table(node_to_id_table);

  if (use_mapped) {
    /*
     * Remove extra gates that might be left behind from old DFF's
     */
    foreach_node(network, gen, node) {
      if (node->type == INTERNAL) {
        if (node_function(node) == NODE_BUF) {
          gate = lib_gate_of(node);
          if (lib_gate_type(gate) != COMBINATIONAL) { /* D-FF */
            fanin_node = node_get_fanin(node, 0);
            if (node_num_fanout(node) == 1) {
              fo = node_get_fanout(node, 0);
              if (fo->type != PRIMARY_OUTPUT ||
                  network_is_real_po(network, fo)) {
                /* node is a leftover from previous mapping */
                (void)node_patch_fanin(fo, node, fanin_node);
                network_delete_node_gen(network, gen);
              }
            } else {
              fo_array = array_alloc(node_t *, 0);
              foreach_fanout(node, gen2, fo) {
                array_insert_last(node_t *, fo_array, fo);
              }
              for (i = array_n(fo_array); i-- > 0;) {
                fo = array_fetch(node_t *, fo_array, i);
                (void)node_patch_fanin(fo, node, fanin_node);
              }
              network_delete_node_gen(network, gen);
              array_free(fo_array);
            }
          }
        } else {
          gate = lib_gate_of(node);
          if (lib_gate_type(gate) != COMBINATIONAL) {
            fo = node_get_fanout(node, 0);
            if (network_latch_end(fo) == NIL(node_t)) {

              /* This is combinational logic left over from retiming
                         latches with combinational logic but with no
             feedback.  This needs to be remapped. */

              (void)fprintf(
                  sisout,
                  "WARNING: complex latch %s at %s had to be remapped\n",
                  lib_gate_name(gate), node_name(node));
              /*
               * Decompose the logic gate and replace it by mapped ones
               */
              netw1 = network_create_from_node(node);
              netw = map_network(netw1, lib_get_library(), 0.0, 1, 0);
              if (netw == NIL(network_t)) {
                error_append("Unable to remap complex latch\n");
                return NIL(network_t);
              }
              network_free(netw1);
              gate_table = st_init_table(st_ptrcmp, st_ptrhash);
              nodes = network_and_node_to_array(netw, node, gate_table);
              root_node = array_fetch(node_t *, nodes, 1);
              for (i = array_n(nodes) - 1; i > 1; i--) {
                temp = array_fetch(node_t *, nodes, i);
                if (node_function(temp) != NODE_PI) {
                  network_add_node(network, temp);
                  /*
                   * Also need to annotate the node with corresp gate
                   */
                  (void)st_lookup(gate_table, (char *)temp, (char **)&gate);
                  buf_add_implementation(temp, gate);
                } else {
                  node_free(temp);
                }
              }
              (void)st_lookup(gate_table, (char *)root_node, (char **)&gate);
              node_replace(node, root_node);
              buf_add_implementation(node, gate);
              array_free(nodes);
              network_free(netw);
              st_free_table(gate_table);
            }
          }
        }
      }
    }
  }
  if (use_mapped) {
    foreach_latch(network, gen, latch) {
      po = latch_get_input(latch);
      pi = latch_get_output(latch);
      fanin_node = node_get_fanin(po, 0);
      if (node_num_fanout(fanin_node) > 1) {
        (void)fprintf(sisout,
                      "WARNING: complex latch %s at %s had to be remapped\n",
                      lib_gate_name(latch_get_gate(latch)), io_name(pi, 0));
        /*
         * Decompose the logic gate and replace it by mapped ones
         */
        netw1 = network_create_from_node(fanin_node);
        netw = map_network(netw1, lib_get_library(), 0.0, 1, 0);
        if (netw == NIL(network_t)) {
          error_append("Unable to remap complex latch\n");
          return NIL(network_t);
        }
        network_free(netw1);
        gate_table = st_init_table(st_ptrcmp, st_ptrhash);
        nodes = network_and_node_to_array(netw, fanin_node, gate_table);
        root_node = array_fetch(node_t *, nodes, 1);
        for (i = array_n(nodes) - 1; i > 1; i--) {
          temp = array_fetch(node_t *, nodes, i);
          if (node_function(temp) != NODE_PI) {
            network_add_node(network, temp);
            /* Also need to annotate the node with corresp gate */
            (void)st_lookup(gate_table, (char *)temp, (char **)&gate);
            buf_add_implementation(temp, gate);
          } else {
            node_free(temp);
          }
        }
        (void)st_lookup(gate_table, (char *)root_node, (char **)&gate);
        node_replace(fanin_node, root_node);
        buf_add_implementation(fanin_node, gate);
        array_free(nodes);
        network_free(netw);
        st_free_table(gate_table);
        /*
         * Set the latch type for the latch to be a simple DFF
         */
        new_buffer = node_literal(fanin_node, 1);
        network_add_node(network, new_buffer);
        node_patch_fanin(po, fanin_node, new_buffer);
        retime_lib_set_gate(new_buffer, d_latch, 0);
        latch_set_gate(latch, d_latch);
      }
    }
  }

  /*
   * Just for ease of the user we will keep the primary outputs in the same
   * order as the original network. Also copy over any delay data that may
   * be stored with the PO nodes ....
   */
  orig_po_array = array_alloc(node_t *, 0);
  fanin_array = array_alloc(node_t *, 0);
  re_foreach_primary_output(graph, i, re_no) {
    name = re_no->node->name;
    po = network_find_node(network, name);
    if (po != NIL(node_t)) {
      array_insert_last(node_t *, fanin_array, node_get_fanin(po, 0));
      array_insert_last(node_t *, orig_po_array, re_no->node);
      network_delete_node(network, po);
    } else {
      (void)fprintf(sisout, "E: PO %s not found in new network\n", name);
    }
  }
  for (i = 0; i < array_n(fanin_array); i++) {
    fanin_node = array_fetch(node_t *, fanin_array, i);
    po = network_add_primary_output(network, fanin_node);
    temp = array_fetch(node_t *, orig_po_array, i);
    network_change_node_name(network, po, util_strsav(node_long_name(temp)));
    delay_dup(temp, po);
  }
  array_free(orig_po_array);
  array_free(fanin_array);

  if (!network_check(network)) {
    (void)fprintf(siserr, "%s", error_string());
  }
  if (use_mapped && (!lib_network_is_mapped(network))) {
    error_append("Network is not mapped after conversion\n");
  }

  /* Now annotate each latch with the appropriate control signal */
  control = NIL(node_t);
  if (graph->control_name != NIL(char)) {
    control = network_find_node(network, graph->control_name);
    if (control == NIL(node_t)) {
      (void)fprintf(siserr, "Cannot recover control signal for latches\n");
    }
    control = network_add_fake_primary_output(network, control);
  }
  foreach_latch(network, gen, latch) {
    latch_set_control(latch, control);
    latch_set_type(latch, graph->s_type);
  }

  return (network);
}

/*
 * Assures that the initial values are present on the edge.
 * If the initial values are presnt, no problem. Else the
 * latches should be present on the edge. Get the values
 * from the latches. Failing this assume don't care values.
 */
static void retime_assure_init_values(re_edge *edge)
{
  int k;

  if (edge->weight == 0)
    return;

  if (edge->initial_values == NIL(int)) {
    edge->initial_values = ALLOC(int, edge->weight);
    edge->num_val_alloc = edge->weight;
    if (edge->latches != NIL(latch_t *)) {
      for (k = edge->weight; k-- > 0;) {
        edge->initial_values[k] = latch_get_initial_value(edge->latches[k]);
      }
    } else {
      for (k = edge->weight; k-- > 0;) {
        edge->initial_values[k] = 2;
      }
    }
  }
}
/*
 * Adds the appropriate latches --
 * Note that the source and sink fields hold the entries to
 * nodes in the old network. The copy field of these nodes
 * has the info in the current network.
 */
static void retime_add_latches(network_t *network, re_edge *edge, lib_gate_t *d_latch)
{
  latch_t *latch;

  if (edge->weight <= 0)
    return;

  retime_assure_init_values(edge);
  if (d_latch != NIL(lib_gate_t) &&
      is_feedback_latch(lib_gate_of(edge->source->node)) && edge->weight == 1 &&
      edge->source == edge->sink) {
    /*
     * The latch has been added when the gate was set at the source
     * Make sure that the initial value is correctly put in
     */
    latch = latch_from_node(node_get_fanout(edge->source->node->copy, 0));
    assert(latch != NIL(latch_t));
    latch_set_initial_value(latch, edge->initial_values[0]);
    latch_set_current_value(latch, edge->initial_values[0]);
    return;
  }

  retime_add_latches_to_network(network, edge->source->node->copy,
                                edge->sink->node->copy, edge->sink_fanin_id,
                                edge->weight, d_latch, edge->initial_values);
}

/*
 * Add "weight" latches between "from" and the "index" fanin of "to"
 */
static void retime_add_latches_to_network(network_t *network, node_t *from_node, node_t *to, int index, int weight, lib_gate_t *d_latch, int *init_val){
  int i, num_latches;
  latch_t *latch;
  node_t *l_in, *l_out, *from, *node;
  /*
   * see if there are already some latches that can be shared
   */

  num_latches = 0;
  from = from_node;
  /*
   * Find the number of latches yet to be added
   */
  retime_share_latches(&from, weight, d_latch, init_val, &num_latches);

  if (num_latches > 0) {
    node = from;

    /*
     * For all but last latch add a pair of po and pi nodes and connect
     * them via the latch entry. Also put in the correct initial value
     */
    for (i = 0; i < num_latches - 1; i++) {
      if (d_latch != NIL(lib_gate_t)) {
        l_in = node_literal(node, 1);
        network_add_node(network, l_in);
        retime_lib_set_gate(l_in, d_latch, 0); /* Annotate the gate */
        node = l_in;
      }
      l_in = network_add_fake_primary_output(network, node);
      l_out = node_alloc();
      network_add_primary_input(network, l_out);
      network_create_latch(network, &latch, l_in, l_out);
      latch_set_initial_value(latch, init_val[weight - num_latches + i]);
      latch_set_current_value(latch, init_val[weight - num_latches + i]);
      latch_set_gate(latch, d_latch);
      node = l_out;
    }
    /*
     * Add the last latch and connect to the "index" fanin of "to"
     */
    if (d_latch != NIL(lib_gate_t)) {
      l_in = node_literal(node, 1);
      network_add_node(network, l_in);
      retime_lib_set_gate(l_in, d_latch, 0); /* Annotate the latch */
      node = l_in;
    }
    l_in = network_add_fake_primary_output(network, node);
    node = node_alloc();
    network_add_primary_input(network, node);
    network_create_latch(network, &latch, l_in, node);
    latch_set_initial_value(latch, init_val[weight - 1]);
    latch_set_current_value(latch, init_val[weight - 1]);
    latch_set_gate(latch, d_latch);
    to->fanin[index] = node;
  } else {
    to->fanin[index] = from;
  }
}

/*
 * Check to see if we can share latches already present. Make sure
 * that the latches present have the same initial value as ours
 */
static void retime_share_latches(node_t **node, int req, lib_gate_t *d_latch, int *init_val, int *deficit) {
  int i;
  node_t *np, *l_out, *l_in;

  for (np = *node, *deficit = req, i = 0;
       ((l_out = retime_get_latch_input(np, d_latch, init_val[i])) !=
        NIL(node_t)) &&
       (*deficit > 0);
       i++) {
    l_in = network_latch_end(l_out);

    np = *node = l_in;
    (*deficit)--;
  }
}

/*
 * Check if latch with the same initial value is present or not
 */
static node_t *retime_get_latch_input(node_t *node, lib_gate_t *d_latch, int value)
{
  lsGen gen;
  node_t *np, *l_out;

  foreach_fanout(node, gen, np) {
    if (d_latch != NIL(lib_gate_t)) {
      /* Traverse a possible buffer -- added nodes are already mapped */
      if (np->type == INTERNAL && lib_gate_of(np) == d_latch) {
        assert(node_num_fanout(np) == 1);
        np = node_get_fanout(np, 0);
        return np;
      }
    }
    if ((l_out = network_latch_end(np)) != NIL(node_t)) {
      /* Make sure the new latch has the desired initial value */
      if (latch_get_initial_value(latch_from_node(l_out)) == value) {
        (void)lsFinish(gen);
        return np;
      }
    }
  }
  return NIL(node_t);
}

/*
 * Set the gate implementation of node.. node is part of a network
 */
static void retime_set_gate(node_t *node, re_graph *graph, st_table *node_to_id_table)
{
  re_edge *edge;
  re_node *re_no;
  int j, k, index;
  lib_gate_t *gate;
  node_t *orig_node;

  assert(st_lookup_int(node_to_id_table, (char *)node, &index));
  re_no = re_get_node(graph, index);
  orig_node = re_no->node;

  gate = lib_gate_of(orig_node);
  k = -1;
  if (is_feedback_latch(gate)) {
    /* Look for the index of the feedback arc for the complex latch */
    re_foreach_fanout(re_no, j, edge) {
      if (re_ignore_edge(edge))
        continue;
      if (edge->weight == 1 && edge->sink == re_no) {
        k = edge->sink_fanin_id;
      }
    }
    assert(k != -1);
  }
  retime_lib_set_gate(node, gate, k);
}

void retime_lib_set_gate(node_t *node, lib_gate_t *gate, int index)
{
  int i, n;
  char **formals;
  latch_t *latch;
  network_t *network;
  node_t *po_node, *pi_node, *np, **actuals;

  /* HERE */
  network = node_network(node);
  if (is_feedback_latch(gate)) {
    /*
     * Special casing for the Complex flip flops
     * Add the PI/PO and the latch construct right here
     */
    pi_node = node_alloc();
    network_add_primary_input(network, pi_node);
    po_node = network_add_primary_output(network, node);

    n = node_num_fanin(node);
    actuals = ALLOC(node_t *, n);
    foreach_fanin(node, i, np) {
      if (i != index)
        actuals[i] = np;
      else
        actuals[i] = pi_node;
    }
    formals = ALLOC(char *, n);
    for (i = 0; i < n; i++) {
      formals[i] = lib_gate_pin_name(gate, i, 1);
    }
    if (!lib_set_gate(node, gate, formals, actuals, n)) {
      (void)fprintf(siserr, "%s", error_string());
    }
    network_create_latch(network, &latch, po_node, pi_node);
    latch_set_gate(latch, gate);
  } else {
    /*
     * Either a DFF or a combinational gate in the old network
     * For now add only the internal gate corresp to the old_gate
     */

    n = node_num_fanin(node);
    actuals = ALLOC(node_t *, n);
    foreach_fanin(node, i, np) { actuals[i] = np; }
    formals = ALLOC(char *, n);
    for (i = 0; i < n; i++) {
      formals[i] = lib_gate_pin_name(gate, i, 1);
    }
    if (!lib_set_gate(node, gate, formals, actuals, n)) {
      (void)fprintf(siserr, "%s", error_string());
    }
  }
  FREE(actuals);
  FREE(formals);
}

/*
 * Generates an array of latches between "to" and "from" modes. The
 * number of such latches is "num" and the latches are arranged in
 * the array as they appear in going from "from" to the "to" node
 */
static latch_t **re_gen_latches(to, index, num, l_table, d_latch) node_t *to;
int index, num;
st_table *l_table; /* reference table of latches */
lib_gate_t *d_latch;
{
  int i;
  char *dummy;
  latch_t **latches;
  node_t *temp, *po, *pi, *end, *actual_to;

  if (num == 0)
    return NIL(latch_t *);

  actual_to = node_get_fanin(to, index);
  end = network_latch_end(actual_to);

  assert(actual_to->type == PRIMARY_INPUT && end != NIL(node_t));

  latches = ALLOC(latch_t *, num);
  for (i = num, pi = actual_to; i-- > 0;) {
    latches[i] = latch_from_node(pi);
    /*
     * This latch has been traversed --
     * delete it from the set of latches  that we need to traverse
     */
    (void)st_delete(l_table, (char **)(latches + i), &dummy);

    po = network_latch_end(pi);
    if (d_latch != NIL(lib_gate_t)) {
      /* A buffer exists -- traverse it as well */
      temp = node_get_fanin(po, 0);
      pi = node_get_fanin(temp, 0);
    } else {
      pi = node_get_fanin(po, 0);
    }
  }
  return latches;
}

#undef is_feedback_latch

#endif /* SIS */
