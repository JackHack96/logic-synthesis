
/* file %M% release %I% */
/* last modified: %G% at %U% */
#ifdef SIS
#include "sis.h"

static int get_node_order();
static st_table *extract_order_info();
static void order_table();
static int extract_trailing_y();
static void replace_character();

 /* puts the order in the table: node (key) -> index (value) */

void get_manual_order(order, options)
st_table *order;
verif_options_t *options;
{
  int n_elts;
  st_table *name_table;

  assert(options->use_manual_order);
  assert(options->order_network);
  name_table = extract_order_info(options->order_network);
  n_elts = st_count(order);
  order_table(order, name_table);
  assert(n_elts == st_count(order));
  st_free_table(name_table);
}

 /* take the network, extract the PI names in that order */
 /* and put them in a hash table, with the order as value */
static st_table *extract_order_info(network)
network_t *network;
{
  int count = 0;
  node_t *pi;
  lsGen gen;
  st_table *name_table = st_init_table(strcmp, st_strhash);
  
  foreach_primary_input(network, gen, pi) {
    st_insert(name_table, pi->name, (char *) count);
    count++;
  }
  return name_table;
}

static void order_table(node_table, name_table)
st_table *node_table;
st_table *name_table;
{
  int i;
  int index;
  int count;
  node_t *node;
  st_generator *gen;
  node_t **nodes;
  
  count = 0;
  nodes = ALLOC(node_t *, st_count(node_table));
  st_foreach_item(node_table, gen, (char **) &node, NIL(char *)) {
    nodes[count++] = node;
  }
  for (i = 0; i < count; i++) {
    index = get_node_order(nodes[i]->name, name_table);
    st_insert(node_table, (char *) nodes[i], (char *) index);
  }
  FREE(nodes);
}


 /* take the name; remove the ":y%d if something like that at the end */
 /* lookup the matching thing in the table and return the number */
static int get_node_order(name, name_table)
char *name;
st_table *name_table;
{
  int rank;
  char *new_name;
  
  if (! extract_trailing_y(name, &new_name)) {
    new_name = util_strsav(name);
  }
  replace_character(new_name, ':', '_');
  if (! st_lookup_int(name_table, new_name, &rank)) {
    fprintf(miserr, "can't find ordering information in file\n");
    rank = 0;
  }
  FREE(new_name);
  return rank;
}

 /* returns 0 if nothing changed; 1 otherwise */
 /* takes a name, and strips anything of the form :y%d at the end */

static int extract_trailing_y(name, new_name)
char *name;
char **new_name;
{
  char *result;
  char *last = (char *) rindex(name, ':');

  *new_name = 0;
  if (last == 0) return 0;
  if (last[1] != 'y') return 0;
  result = (char *) malloc(last - name + 1);
  strncpy(result, name, last - name);
  result[last - name] = '\0';
  *new_name = result;
  return 1;
}

static void replace_character(name, old, new)
char *name;
char old;
char new;
{
  char *p;

  for (p = name; *p != '\0'; p++) {
    if (*p == old) *p = new;
  }
}
#endif /* SIS */
