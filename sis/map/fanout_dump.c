/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/fanout_dump.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:24 $
 *
 */
/* file @(#)fanout_dump.c	1.2 */
/* last modified on 5/1/91 at 15:50:22 */
#include <stdio.h>
#include "sis.h"
#include "map_int.h"
#include "lib_int.h"
#include "map_delay.h"
#include "fanout_int.h"
#include "fanout_delay.h"

static int do_fanout_dump();
static network_t *extract_fanout_network();
static node_t *add_inverter_to_source();
static void add_sinks();
static void add_source_to_network();


 /* uses the same interface as a fanout optimizer */
 /* but actually dumps each fanout problem individually */
 /* into a blif file */
 /* the blif files are generated in sequence */

static char *current_network_name;

void fanout_dump_init(network, alg)
network_t *network;
fanout_alg_t *alg;
{
  current_network_name = network_name(network);
  alg->optimize = do_fanout_dump;
}

static int DUMP_THRESHOLD = 40;

 /* ARGSUSED */
void fanout_dump_set_dump_threshold(alg, property)
fanout_alg_t *alg;
alg_property_t *property;
{
  DUMP_THRESHOLD = property->value;
}


 /* INTERNAL INTERFACE */

 /* ARGSUSED */

static int do_fanout_dump(fanout_info, tree, cost)
opt_array_t *fanout_info;		 /* fanout_info[POLAR_X] lists the positive polarity sinks; POLAR_Y negative */
array_t *tree;				 /* array in which the result tree is stored: format of fanout_tree.c */
fanout_cost_t *cost;			 /* contains information on the result: required times, area */
{
  static uniq_id = 0;
  char *filename, *p;
  network_t *network;
  FILE *fp;

  cost->slack = MINUS_INFINITY;
  cost->area =  INFINITY;
  network = extract_fanout_network(fanout_info);
  if (network) {
    if (current_network_name == NIL(char))
      current_network_name = "debug";
    filename = ALLOC(char, strlen(current_network_name) + 20);
    sprintf(filename, "%s.%d.fanout", current_network_name, uniq_id);
    p = strrchr(filename, '/');
    p = (p != NIL(char)) ? p + 1 : filename;
    if ((fp = fopen(p, "w")) == NULL) {
      fprintf(siserr, "can't open file %s\n", filename);
    } else {
      network_set_name(network, filename);
      write_blif(fp, network, 0, 1);
      (void) fclose(fp);
      uniq_id++;
    }
    FREE(filename);
    network_free(network);
  }
  return 0;
}

static network_t *extract_fanout_network(fanout_info)
opt_array_t *fanout_info;
{
  int source_polarity;
  node_t *source, *inverter;
  network_t *network;

  if (fanout_info[POLAR_X].n_elts + fanout_info[POLAR_Y].n_elts < DUMP_THRESHOLD) return NIL(network_t);
  network = network_alloc();
  add_source_to_network(network, &source, &source_polarity);
  add_sinks(network, source, &fanout_info[source_polarity]);
  if (fanout_info[POLAR_INV(source_polarity)].n_elts > 0) {
    inverter = add_inverter_to_source(network, source);
    add_sinks(network, inverter, &fanout_info[POLAR_INV(source_polarity)]);
  }
  return network;
}

 /* just take the first available source */

static void add_source_to_network(network, source, source_polarity)
network_t *network;
node_t **source;
int *source_polarity;
{
  int i, n_pi;
  lsGen gen;
  delay_time_t arrival;
  delay_time_t drive;
  node_t *node, *new_node, *pi;
  node_t **new_nodes;
  char **formals;
  n_gates_t n_gates;

  n_gates = fanout_delay_get_n_gates();
  node = fanout_delay_get_source_node(n_gates.n_buffers);
  *source_polarity = fanout_delay_get_source_polarity(n_gates.n_buffers);
  if (node->type == PRIMARY_INPUT) {
    *source = new_node = node_alloc();
    map_alloc(new_node);
    new_node->name = util_strsav(node->name);
    network_add_primary_input(network, new_node);
    arrival = pipo_get_pi_arrival(node);
    drive = pipo_get_pi_drive(node);
    delay_set_parameter(new_node, DELAY_DRIVE_RISE, drive.rise);
    delay_set_parameter(new_node, DELAY_DRIVE_FALL, drive.fall);
    delay_set_parameter(new_node, DELAY_ARRIVAL_RISE, arrival.rise);
    delay_set_parameter(new_node, DELAY_ARRIVAL_FALL, arrival.fall);
  } else {
    n_pi = MAP(node)->ninputs;
    new_nodes = ALLOC(node_t *, n_pi);
    for (i = 0; i < n_pi; i++) {
      char buffer[10];
      new_nodes[i] = node_alloc();
      map_alloc(new_nodes[i]);
      sprintf(buffer, "%d", i);
      new_nodes[i]->name = util_strsav(buffer);
      network_add_primary_input(network, new_nodes[i]);
      arrival = MAP(MAP(node)->save_binding[i])->map_arrival;
      drive = pipo_get_pi_drive(NIL(node_t));
      delay_set_parameter(new_nodes[i], DELAY_DRIVE_RISE, drive.rise);
      delay_set_parameter(new_nodes[i], DELAY_DRIVE_FALL, drive.fall);
      delay_set_parameter(new_nodes[i], DELAY_ARRIVAL_RISE, arrival.rise);
      delay_set_parameter(new_nodes[i], DELAY_ARRIVAL_FALL, arrival.fall);
    }
    *source = new_node = node_alloc();
    map_alloc(new_node);
    new_node->name = util_strsav(node->name);
    formals = ALLOC(char *, n_pi);
    i = 0;
    foreach_primary_input(MAP(node)->gate->network, gen, pi) {
      formals[i++] = pi->name;
    }
    assert(lib_set_gate(new_node, MAP(node)->gate, formals, new_nodes, n_pi));
    network_add_node(network, new_node);
    FREE(formals);
    FREE(new_nodes);
  }
}

static void add_sinks(network, source, fanout_info)
network_t *network;
node_t *source;
opt_array_t *fanout_info;
{
  int i;
  node_t *po;
  gate_link_t *link;
  char *buffer;

  for (i = 0; i < fanout_info->n_elts; i++) {
    link = array_fetch(gate_link_t *, fanout_info->required, i);
    po = network_add_primary_output(network, source);
    map_alloc(po);
    buffer = ALLOC(char, strlen(link->node->name) + 10);
    sprintf(buffer, "%s(%d)", link->node->name, link->pin);
    network_change_node_name(network, po, util_strsav(buffer));
    FREE(buffer);
    delay_set_parameter(po, DELAY_REQUIRED_RISE, link->required.rise);
    delay_set_parameter(po, DELAY_REQUIRED_FALL, link->required.fall);
    delay_set_parameter(po, DELAY_OUTPUT_LOAD, link->load);
  }
}

static node_t *add_inverter_to_source(network, source)
network_t *network;
node_t *source;
{
  node_t *inverter;

  inverter = node_literal(source, 0);
  map_alloc(inverter);
  MAP(inverter)->ninputs = 1;
  MAP(inverter)->gate = lib_get_default_inverter();
  MAP(inverter)->save_binding = ALLOC(node_t *, 1);
  MAP(inverter)->save_binding[0] = source;
  inverter->name = util_strsav(".inv.");
  network_add_node(network, inverter);
  return inverter;
}

