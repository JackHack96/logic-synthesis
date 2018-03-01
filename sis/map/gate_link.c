/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/gate_link.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:24 $
 *
 */
/* file @(#)gate_link.c	1.5 */
/* last modified on 6/27/91 at 15:12:57 */
/**
**  MODIFICATION HISTORY:
**
**  01 10-June-91 klk	Changed the function fanout_print_node to global so that it can be called from outside
**			for debugging purposes.
**/
#include "sis.h"
#include <math.h>
#include "map_int.h"
#include "map_delay.h"
#include "map_macros.h"
#include "gate_link.h"

static int gate_link_key_cmp();
static int gate_link_key_hash();

 /* implements basic routines to manipulate "gate_links" */
 /* that is, backward pointers from pins to roots of gates */

typedef struct gate_link_key_struct gate_link_key_t;
struct gate_link_key_struct {
  node_t *node;			/* root of the gate */
  int pin;			/* pin number */
};

typedef struct gate_link_value_struct gate_link_value_t;
struct gate_link_value_struct {
  double load;			/* load on this pin */
  double slack;			/* used by bin_delay.c. Might disappear at some point */
  delay_time_t required;	/* slack for this signal at this input pin */
};

static delay_time_t PLUS_INFINITY = {INFINITY, INFINITY};

 /* EXTERNAL INTERFACE */

 /* allocates gate_link (hash table) */

void gate_link_init(node)
node_t *node;
{
  assert(MAP(node)->gate_link == NIL(st_table));
  MAP(node)->gate_link = st_init_table(gate_link_key_cmp, gate_link_key_hash);
  MAP(node)->gen = NIL(st_generator);
}


 /* sums the loads on the pins; takes wire capacitances into account */

double gate_link_compute_load(node)
node_t *node;
{
  double load = 0.0;
  char *key, *value;
  st_generator *gen;
  register st_table *table = MAP(node)->gate_link;
  int n_fanouts = 0;

  st_foreach_item(table, gen, &key, &value) {
    load += ((gate_link_value_t *) value)->load;
    n_fanouts++;
  }
  return load + map_compute_wire_load(n_fanouts);
}

delay_time_t gate_link_compute_min_required(node)
node_t *node;
{
  delay_time_t required;
  gate_link_key_t *key;
  gate_link_value_t *value;
  st_generator *gen;
  register st_table *table = MAP(node)->gate_link;

  required = PLUS_INFINITY;
  st_foreach_item(table, gen, (char **) &key, (char **) &value) {
    SETMIN(required, required, value->required);
  }
  return required;
}


int gate_link_n_elts(node)
node_t *node;
{
  register st_table *table = MAP(node)->gate_link;
  return st_count(table);
}

int gate_link_is_empty(node)
node_t *node;
{
  return (gate_link_n_elts(node) == 0);
}


 /* return 0 if empty */
 /* starts an iterator */
 /* gate_link_first(n, l); do {...} while (gate_link_next(n, l)); */
 /* supposes that storage has been allocated for *link */

int gate_link_first(node, link)
node_t *node;
gate_link_t *link;
{
  register st_table *table = MAP(node)->gate_link;
  MAP(node)->gen = st_init_gen(table);
  return gate_link_next(node, link);
}

int gate_link_next(node, link)
     node_t *node;
     gate_link_t *link;
{
  char *key, *value;
  st_generator *gen = MAP(node)->gen;
  
  if (gen == NIL(st_generator)) return 0;
  if (st_gen(gen, &key, &value) == 0) {
    st_free_gen(gen);
    MAP(node)->gen = NIL(st_generator);
    return 0;
  }
  link->node = ((gate_link_key_t *) key)->node;
  link->pin = ((gate_link_key_t *) key)->pin;
  link->load = ((gate_link_value_t *) value)->load;
  link->slack = ((gate_link_value_t *) value)->slack;
  link->required = ((gate_link_value_t *) value)->required;
  return 1;
}


 /* insert the link into the gate_link table of node */

void gate_link_put(node, link)
node_t *node;
gate_link_t *link;
{
  gate_link_key_t key;
  gate_link_value_t *value;
  register st_table *table = MAP(node)->gate_link;

  key.node = link->node;
  key.pin = link->pin;
  if (st_lookup(table, (char *) &key, (char **) &value)) {
    value->load = link->load;
    value->slack = link->slack;
    value->required = link->required;
  } else {
    gate_link_key_t *new_key = ALLOC(gate_link_key_t, 1);
    gate_link_value_t *new_value = ALLOC(gate_link_value_t, 1);
    new_key->node = link->node;
    new_key->pin = link->pin;
    new_value->load = link->load;
    new_value->slack = link->slack;
    new_value->required = link->required;
    st_insert(table, (char *) new_key, (char *) new_value);
  }
}


 /* link=(key,value); using the key part, fill up the value part */
 /* with what is recorded in the gate_link */
 /* returns 0 if no match */

int gate_link_get(node, link)
node_t *node;
gate_link_t *link;
{
  gate_link_key_t key;
  char *value;
  register st_table *table = MAP(node)->gate_link;

  key.node = link->node;
  key.pin = link->pin;
  if (st_lookup(table, (char *) &key, &value)) {
    link->load = ((gate_link_value_t *) value)->load;
    link->slack = ((gate_link_value_t *) value)->slack;
    link->required = ((gate_link_value_t *) value)->required;
    return 1;
  } else {
    return 0;
  }
}


 /* link=(key,value); remove gate_link entry associated with key if any */

void gate_link_remove(node, link)
node_t *node;
gate_link_t *link;
{
  gate_link_key_t key;
  char *key_ptr;
  char *value_ptr;
  register st_table *table = MAP(node)->gate_link;

  key.node = link->node;
  key.pin = link->pin;
  key_ptr = (char *) &key;
  if (st_delete(table, &key_ptr, &value_ptr)) {
    FREE(key_ptr);
    FREE(value_ptr);
  }
}


 /* free the gate_link table and allocate a new one with no data in it */

void gate_link_delete_all(node)
node_t *node;
{
  gate_link_free(node);
  gate_link_init(node);
}


 /* free the gate_link and all storage associated with it */

void gate_link_free(node)
node_t *node;
{
  char *key, *value;
  st_generator *gen;
  register st_table *table = MAP(node)->gate_link;
  st_foreach_item(table, gen, &key, &value) {
    FREE(key); FREE(value);
  }
  st_free_table(table);
  MAP(node)->gate_link = NIL(st_table);
}


 /* INTERNAL INTERFACE */

static int gate_link_key_cmp(key1, key2)
     char *key1, *key2;
{
  register int i1, i2;
  register gate_link_key_t *k1 = (gate_link_key_t *) key1;
  register gate_link_key_t *k2 = (gate_link_key_t *) key2;

  i1 = (int) k1->node; i2 = (int) k2->node; 
  if (i1 - i2) return i1 - i2;
  i1 = k1->pin; i2 = k2->pin;
  return i1 - i2;
}

static int gate_link_key_hash(key, modulus)
     char *key;
     int modulus;
{
  register gate_link_key_t *k = (gate_link_key_t *) key;
  register unsigned int hash = (unsigned int) k->node;
  hash += k->pin;
  return hash % modulus;
}


 /* for debugging */

#include "map_int.h"
void fanout_print_node(node)
node_t *node;
{
  if (node == NIL(node_t)) return;
  fprintf(sisout, "node: %s(0x%x)\n", node->name, node);
  fprintf(sisout, "l(%2.2f) ", MAP(node)->load);
  fprintf(sisout, "a(%2.2f,%2.2f) ", MAP(node)->map_arrival.rise, MAP(node)->map_arrival.fall);
  fprintf(sisout, "r(%2.2f,%2.2f)\n", MAP(node)->required.rise, MAP(node)->required.fall);
  if (node->type == PRIMARY_INPUT) {
    fprintf(sisout, "PI\n");
  } else if (node->type == PRIMARY_OUTPUT) {
    fprintf(sisout, "PO\n");
  } else if (MAP(node)->gate == NIL(lib_gate_t)) {
    fprintf(sisout, "no gate\n");
  } else {
    int i;
    fprintf(sisout, "gate(%s) ", MAP(node)->gate->name);
    for (i = 0; i < MAP(node)->ninputs; i++)
      fprintf(sisout, "%s(0x%x) ", MAP(node)->save_binding[i]->name, MAP(node)->save_binding[i]);
    fprintf(sisout, "\n");
  }
  if (MAP(node)->gate_link == NIL(st_table)) {
    fprintf(sisout, "no gate link\n");
  } else if (gate_link_is_empty(node)) {
    fprintf(sisout, "empty gate link\n");
  } else {
    gate_link_t link;
    gate_link_first(node, &link);
    do {
      fprintf(sisout, "--> node: %s(0x%x) ", (link.node)->name, link.node);
      fprintf(sisout, "pin: %d ", link.pin);
      fprintf(sisout, "l(%2.2f) ", link.load);
      fprintf(sisout, "r(%2.2f,%2.2f)\n", link.required.rise, link.required.fall);
    } while (gate_link_next(node, &link));
  }
}

 /* for debugging */

void fanout_print_network(network)
network_t *network;
{
  lsGen gen;
  node_t *node;
  
  foreach_node(network, gen, node) {
    fanout_print_node(node);
  }
}
