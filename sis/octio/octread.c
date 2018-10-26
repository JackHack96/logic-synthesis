
#ifdef OCT
#define SEQUENTIAL

/*
 * routine for reading MISII logic networks (mapped and unmapped) from
 * Oct facets
 * Extended to also read mapped sequential circuits (Andrea Casotto, K.J.Singh)
 * NOTE: no facility yet (5/4/91) to read unmapped sequential circuits !!!
 */
#include "../port/copyright.h"
#include "../port/port.h"
#undef assert
#include "sis.h"
#include "st.h"
#undef REALLOC
#undef FREE
#include "errtrap.h"
#include "oct.h"
#include "octio.h"
#include "oh.h"

#ifdef SIS
void read_change_madeup_name(); /* Should be in io.h */
static void oct_read_set_latch();
#endif                                     /* SIS */
node_t *read_find_or_create_node(); /* Should be in io.h */

#define node_oct_name(node) (node)->name

#define STREQ(a, b) (strcmp(a, b) == 0)

#define OCTMISCHECK(fnc)                                                       \
  {                                                                            \
    octStatus octmischeck_status;                                              \
    octmischeck_status = fnc;                                                  \
    if (octmischeck_status < OCT_OK) {                                         \
      errRaise(SIS_PKG_NAME, octmischeck_status, octErrorString());            \
    }                                                                          \
  }

static void uniqNames(container, mask) octObject *container;
octObjectMask mask;
{
  octObject obj;
  octGenerator gen;
  octStatus status;
  char *name, dummyname[1024];
  st_table *name_table;
  int cnt, count;

  name_table = st_init_table(strcmp, st_strhash);
  cnt = 0;

  OH_ASSERT(octInitGenContents(container, mask, &gen));
  while ((status = octGenerate(&gen, &obj)) == OCT_OK) {
    name = ohGetName(&obj);
    if (name == NIL(char)) {
      errRaise(SIS_PKG_NAME, OCT_ERROR, "ohGetName returned NIL");
    }
    if (name[0] == '\0') {
      /* make sure the name is not empty */
      (void)sprintf(dummyname, "%s-%d", ohTypeName(&obj), cnt++);
      OH_ASSERT(ohPutName(&obj, dummyname));
      OH_ASSERT(octModify(&obj));
      OH_ASSERT(ohGetById(&obj, obj.objectId));
      name = ohGetName(&obj);
    }
    if (st_lookup(name_table, name, NIL(char *))) {
      /* loop until we can generate a unique name */
      count = 2;
      do {
        (void)sprintf(dummyname, "%s-{%d}", name, count++);
      } while (st_lookup(name_table, dummyname, NIL(char *)));

      /* change the objects name */
      OH_ASSERT(ohPutName(&obj, dummyname));
      OH_ASSERT(octModify(&obj));
      OH_ASSERT(ohGetById(&obj, obj.objectId));
      name = ohGetName(&obj);
    }

    (void)st_insert(name_table, name, NIL(char));
  }
  OH_ASSERT(status);
  (void)st_free_table(name_table);
  return;
}

static node_t *build_new_node(node, network, instance) node_t *node;
network_t *network;
octObject *instance;
{
  node_t *result, *temp, *oldfanin, *newfanin, *nodeliteral, *and, * or ;
  node_cube_t cube;
  node_literal_t literal;
  int i, j;
  octObject term, net;
  octGenerator gen;
  st_table *table;
  char *netname;
  octStatus status;

  /* build up terminal/net table */
  table = st_init_table(strcmp, st_strhash);
  OH_ASSERT(octInitGenContents(instance, OCT_TERM_MASK, &gen));
  while (octGenerate(&gen, &term) == OCT_OK) {
    status = octGenFirstContainer(&term, OCT_NET_MASK, &net);
    if (status == OCT_OK) {
      (void)st_insert(table, term.contents.term.name, net.contents.net.name);
    }
    if (status < OCT_OK) {
      errRaise(SIS_PKG_NAME, status, "oct error (%s)", octErrorString());
    }
  }

  result = node_constant(0);

  for (i = node_num_cube(node) - 1; i >= 0; i--) {
    cube = node_get_cube(node, i);
    temp = node_constant(1);
    foreach_fanin(node, j, oldfanin) {
      literal = node_get_literal(cube, j);

      if (literal == TWO) {
        continue;
      }

      /* get the net attached to the terminal with the name of the oldfanin */
      if (!st_lookup(table, node_oct_name(oldfanin), &netname)) {
        errRaise(SIS_PKG_NAME, OCT_ERROR,
                 "FATAL: missing connection to terminal %s on %s\n",
                 node_oct_name(oldfanin), ohFormatName(instance));
      }
      newfanin = read_find_or_create_node(network, netname);

      nodeliteral = node_literal(newfanin, ((literal == ZERO) ? 0 : 1));
      and = node_and(temp, nodeliteral);
      node_free(temp); /* garbage collection */
      node_free(nodeliteral);
      temp = and;
    }
    or = node_or(result, temp);
    node_free(result);
    node_free(temp);
    result = or ;
  }

  st_free_table(table);
  return (result);
}

#ifdef SIS
/*

   OCT structure:

   facet->bag(SIS_CLOCKS)|->prop(CYCLETIME)
                         |->bag(CLOCK_EVENTS)|->prop(TIME)
                         |                   |->bag(EVENT)|->prop(DESCRIPTION)
                         |                   |            |->prop(MIN)
                         |                   |            |->prop(MAX)
                         |                   |
                         |                   |->bag(EVENT)|->prop(DESCRIPTION)
                         |                                |->prop(MIN)
                         |                                |->prop(MAX)
                         |
                         |->bag(CLOCK_EVENTS)|->prop(TIME)
                                             |->bag(EVENT)|->prop(DESCRIPTION)
                                             |            |->prop(MIN)
                                             |            |->prop(MAX)
                                             |
                                             |->bag(EVENT)|->prop(DESCRIPTION)
                                                          |->prop(MIN)
                                                          |->prop(MAX)

*/

static void read_oct_add_clock_event(network, eventBag) network_t *network;
octObject *eventBag;
{
  int first = 1;
  clock_edge_t first_edge, edge;
  octObject bag, prop;
  octGenerator gen;
  char *name;

  double nominal;
  if (ohGetByPropName(eventBag, &prop, "TIME") == OCT_OK) {
    nominal = prop.contents.prop.value.real;
  } else {
    nominal = CLOCK_NOT_SET;
  }

  octInitGenContents(eventBag, OCT_BAG_MASK, &gen);
  while (octGenerate(&gen, &bag) == OCT_OK) {
    char *desc;
    double min, max;

    if (ohGetByPropName(&bag, &prop, "DESCRIPTION") == OCT_OK) {
      desc = prop.contents.prop.value.string;
    } else {
      errRaise("oct_read", 1, "No specification of description in clock_event");
    }
    if (ohGetByPropName(&bag, &prop, "MIN") == OCT_OK) {
      min = prop.contents.prop.value.real;
    } else {
      min = CLOCK_NOT_SET;
    }
    if (ohGetByPropName(&bag, &prop, "MAX") == OCT_OK) {
      max = prop.contents.prop.value.real;
    } else {
      max = CLOCK_NOT_SET;
    }

    if (strlen(desc) < 2 || (*desc != 'r' && *desc != 'f'))
      errRaise("oct_read", 1, "Illegal transition description %s", desc);

    name = desc + 2;
    edge.transition = (*desc == 'r') ? RISE_TRANSITION : FALL_TRANSITION;
    edge.clock = clock_get_by_name(network, name);
    if (edge.clock == 0) {
      errRaise("oct_read", 1, "clock %s not found in clock_list", name);
    }

    (void)clock_set_parameter(edge, CLOCK_NOMINAL_POSITION, nominal);
    (void)clock_set_parameter(edge, CLOCK_LOWER_RANGE, min);
    (void)clock_set_parameter(edge, CLOCK_UPPER_RANGE, max);

    /*
    (void)fprintf(sisout, "Time %lf %s %lf %lf\n", nominal, desc, min, max );
    */

    if (first) {
      first = 0;
      first_edge.transition = edge.transition;
      first_edge.clock = edge.clock;
    } else {
      (void)clock_add_dependency(first_edge, edge);
    }
  }
}

/*
 * Looks at a net with NETTYPE "CLCOK" and gets the formal terminal name
 */
static char *read_oct_clock_name(net) octObject *net;
{
  octGenerator gen;
  octObject term;

  OH_ASSERT(octInitGenContents(net, OCT_TERM_MASK, &gen));
  while (octGenerate(&gen, &term) == OCT_OK) {
    if (term.contents.term.instanceId == oct_null_id) {
      return util_strsav(term.contents.term.name);
    }
  }
  return NIL(char);
}

int read_oct_clock(network, facet) network_t *network;
octObject *facet;
{
  octObject sisBag, bag, net, term, prop;
  octGenerator gen, genNet;
  int count = 0;
  int i, nlist;
  node_t *node;
  char *name, *clk_name;
  sis_clock_t *clock;

  octInitGenContents(facet, OCT_NET_MASK, &genNet);
  while (octGenerate(&genNet, &net) == OCT_OK) {
    if (ohGetByPropName(&net, &prop, "NETTYPE") != OCT_OK)
      continue;
    if (!STREQ(prop.contents.prop.value.string, "CLOCK"))
      continue;
    count++;
    name = net.contents.net.name;
    node = read_find_or_create_node(network, name);
    if (node_function(node) != NODE_UNDEFINED) {
      errRaise("oct_read", 1, "clock %s is multiply defined", name);
    }
    clk_name = read_oct_clock_name(&net);
    if (clk_name == NIL(char)) {
      errRaise("oct_read", 1, "Clock %s does not have formal terminal", name);
    }
    clock = clock_create(clk_name);
    if (!clock_add_to_network(network, clock)) {
      errRaise("oct_read", 1, "clock %s is already part of the network",
               clock_name(clock));
    }
    (void)free(clk_name);
  }

  if (count == 0) {
    /*
    (void)fprintf(sisout, "No clocks in the network\n" );
    */
    return 1;
  }

  if (ohGetByBagName(facet, &sisBag, "SIS_CLOCKS") != OCT_OK) {
    /*
    (void)fprintf(sisout, "No SIS_CLOCK bag\n" );
    */
    return 1;
  }

  {
    double cycle_time;
    if (ohGetByPropName(&sisBag, &prop, "CYCLETIME") == OCT_OK) {
      cycle_time = prop.contents.prop.value.real;
    } else {
      cycle_time = CLOCK_NOT_SET;
    }
    clock_set_cycletime(network, cycle_time);
  }

  octInitGenContents(&sisBag, OCT_BAG_MASK, &gen);
  while (octGenerate(&gen, &bag) == OCT_OK) {
    read_oct_add_clock_event(network, &bag); /* This is a CLOCK_EVENTS bag */
  }
  return 1;
}
#endif /* SIS */

struct termInfo {
  char *name;
  network_t *logicFunction;
};

static st_table *MasterTable = NIL(st_table);
#ifdef SIS
static st_table *TypeTable = NIL(st_table);
static st_table *ClockTable = NIL(st_table);
#endif /* SIS */

static void initialize_master_table() {
  MasterTable = st_init_table(strcmp, st_strhash);
#ifdef SIS
  TypeTable = st_init_table(strcmp, st_strhash);
  ClockTable = st_init_table(strcmp, st_strhash);
#endif /* SIS */
  return;
}

static void termFree(terminfo) struct termInfo *terminfo;
{
  (void)free(terminfo->name);
  network_free(terminfo->logicFunction);
  return;
}

static void clean_master_table() {
  st_generator *gen;
  char *key, *value;
  lsList the_list;

  st_foreach_item(MasterTable, gen, &key, &value) {
    the_list = (lsList)value;
    if (the_list != (lsList)0) {
      (void)lsDestroy(the_list, termFree);
    }
  }
  st_free_table(MasterTable);
#ifdef SIS
  st_free_table(TypeTable);
  st_free_table(ClockTable);
#endif /* SIS */
  return;
}

#ifdef SIS

static lsList is_a_logic_cell(instance, is_comb_cellp) octObject *instance;
int *is_comb_cellp;
/*
 * determine if an instance has a logic representation
 *
 *    returns: lsList of the logic function(s), 0 if not a logic cell,
 *    In case the instance is that of a latch,  *is_comb_cellp is set to 0
 */
{
  char *ptr;
  octGenerator gen;
  octObject term, prop, master;
  lsList outputs;
  struct termInfo *termInfo;
  int termTypeExists, is_comb_cell;

  if (st_lookup(MasterTable, instance->contents.instance.master, &ptr)) {
    /* it has been seen before */
    (void)st_lookup_int(TypeTable, instance->contents.instance.master,
                        &is_comb_cell);
    *is_comb_cellp = is_comb_cell;
    return ((lsList)ptr);
  }

  if ((ohOpenMaster(&master, instance, "interface", "r") >= OCT_OK) &&
      (ohGetByPropName(&master, &prop, "CELLTYPE") == OCT_OK)) {
    if (STREQ(prop.contents.prop.value.string, "COMBINATIONAL")) {
      *is_comb_cellp = TRUE;
    } else if (STREQ(prop.contents.prop.value.string, "MEMORY")) {
      *is_comb_cellp = FALSE;
    } else {
      (void)st_insert(MasterTable, instance->contents.instance.master,
                      NIL(char));
      (void)st_insert(TypeTable, instance->contents.instance.master, (char *)1);
      return ((lsList)0);
    }
  } else {
    (void)st_insert(MasterTable, instance->contents.instance.master, NIL(char));
    (void)st_insert(TypeTable, instance->contents.instance.master, (char *)1);
    return ((lsList)0);
  }

  if (octInitGenContents(&master, OCT_TERM_MASK, &gen) < OCT_OK) {
    errRaise(SIS_PKG_NAME, OCT_ERROR, octErrorString());
  }

  outputs = lsCreate();

  while (octGenerate(&gen, &term) == OCT_OK) {
    termTypeExists = (ohGetByPropName(&term, &prop, "TERMTYPE") == OCT_OK);
    /* if no TERMTYPE, defaults to SIGNAL */
    if (termTypeExists && !STREQ(prop.contents.prop.value.string, "SIGNAL")) {
      continue;
    }
    if ((ohGetByPropName(&term, &prop, "DIRECTION") < OCT_OK) ||
        !STREQ(prop.contents.prop.value.string, "OUTPUT")) {
      continue;
    }
    termInfo = ALLOC(struct termInfo, 1);
    termInfo->name = util_strsav(term.contents.term.name);
    if (ohGetByPropName(&term, &prop, "LOGICFUNCTION") < OCT_OK) {
      termInfo->logicFunction = NIL(network_t);
    } else {
      if ((termInfo->logicFunction = read_eqn_string(
               prop.contents.prop.value.string)) == NIL(network_t)) {
        errRaise(SIS_PKG_NAME, OCT_ERROR, "bad equation %s (%s)",
                 prop.contents.prop.value.string, error_string());
      }
    }
    (void)lsNewEnd(outputs, (lsGeneric)termInfo, NIL(lsHandle));
  }

  (void)st_insert(MasterTable, instance->contents.instance.master,
                  (char *)outputs);
  (void)st_insert(TypeTable, instance->contents.instance.master,
                  (char *)(*is_comb_cellp));

  if (octCloseFacet(&master) < OCT_OK) {
    errRaise(SIS_PKG_NAME, OCT_ERROR, octErrorString());
  }
  return (outputs);
}

#else  /* for misII the routine has a different structure */

static lsList is_a_logic_cell(instance) octObject *instance;
/*
 * determine if an instance has a logic representation
 *
 *    returns: lsList of the logic function(s), 0 if not a logic cell,
 */
{
  char *ptr;
  octGenerator gen;
  octObject term, prop, master;
  lsList outputs;
  struct termInfo *termInfo;
  int termTypeExists;

  if (st_lookup(MasterTable, instance->contents.instance.master, &ptr)) {
    /* it has been seen before */
    return ((lsList)ptr);
  }

  if ((ohOpenMaster(&master, instance, "interface", "r") < OCT_OK) ||
      (ohGetByPropName(&master, &prop, "CELLTYPE") < OCT_OK) ||
      !STREQ(prop.contents.prop.value.string, "COMBINATIONAL")) {

    (void)st_insert(MasterTable, instance->contents.instance.master, NIL(char));
    return ((lsList)0);
  }

  if (octInitGenContents(&master, OCT_TERM_MASK, &gen) < OCT_OK) {
    errRaise(SIS_PKG_NAME, OCT_ERROR, octErrorString());
  }

  outputs = lsCreate();

  while (octGenerate(&gen, &term) == OCT_OK) {
    termTypeExists = (ohGetByPropName(&term, &prop, "TERMTYPE") == OCT_OK);
    /* if no TERMTYPE, defaults to SIGNAL */
    if (termTypeExists && !STREQ(prop.contents.prop.value.string, "SIGNAL")) {
      continue;
    }
    if ((ohGetByPropName(&term, &prop, "DIRECTION") < OCT_OK) ||
        !STREQ(prop.contents.prop.value.string, "OUTPUT")) {
      continue;
    }
    termInfo = ALLOC(struct termInfo, 1);
    termInfo->name = util_strsav(term.contents.term.name);
    if (ohGetByPropName(&term, &prop, "LOGICFUNCTION") < OCT_OK) {
      termInfo->logicFunction = NIL(network_t);
    } else {
      if ((termInfo->logicFunction = read_eqn_string(
               prop.contents.prop.value.string)) == NIL(network_t)) {
        errRaise(SIS_PKG_NAME, OCT_ERROR, "bad equation %s (%s)",
                 prop.contents.prop.value.string, error_string());
      }
    }
    (void)lsNewEnd(outputs, (lsGeneric)termInfo, NIL(lsHandle));
  }

  (void)st_insert(MasterTable, instance->contents.instance.master,
                  (char *)outputs);

  if (octCloseFacet(&master) < OCT_OK) {
    errRaise(SIS_PKG_NAME, OCT_ERROR, octErrorString());
  }
  return (outputs);
}
#endif /* SIS */

#ifdef SIS /* Routines to deal with latches are required only for sis */
/*
 * Set the type of the latch, initial_value
 * TODO, also the control signal
 */
static void oct_read_set_latch(network, latch, instance) network_t *network;
latch_t *latch;
octObject *instance;
{
  octGenerator gen;
  octObject master, term, net, prop;
  char *name;
  node_t *control_node, *node;
  int active_high = -1, edge_triggered = -1, clock_used = 0;

  if (ohOpenMaster(&master, instance, "interface", "r") >= OCT_OK) {
    if (ohGetByPropName(&master, &prop, "CELLTYPE") == OCT_OK &&
        STREQ(prop.contents.prop.value.string, "MEMORY")) {
      if (ohGetByPropName(&master, &prop, "SYNCHMODEL") == OCT_OK) {
        if (STREQ(prop.contents.prop.value.string, "TRANSPARENT-LATCH")) {
          edge_triggered = 0;
        } else if (STREQ(prop.contents.prop.value.string,
                         "MASTER-SLAVE-LATCH")) {
          edge_triggered = 1;
        }
      }
    }
    if (edge_triggered >= 0) {
      OH_ASSERT(octInitGenContents(&master, OCT_TERM_MASK, &gen));
      while (octGenerate(&gen, &term) == OCT_OK) {
        if (ohGetByPropName(&term, &prop, "SYNCHTERM") == OCT_OK) {
          if (STREQ(prop.contents.prop.value.string, "CONTROL") ||
              STREQ(prop.contents.prop.value.string, "CONTROLBAR")) {
            if (ohGetByPropName(&term, &prop, "ACTIVE_LEVEL") == OCT_OK) {
              if (STREQ(prop.contents.prop.value.string, "LOW")) {
                active_high = 0;
              } else if (STREQ(prop.contents.prop.value.string, "CONTROLBAR")) {
                active_high = 1;
              }
            }
            /*
             * Also check if this is a clock signal connected to something
             * Generate a net from the terminal on the instance
             */
            if (ohTerminalNet(&term, &net) == OCT_OK) {
              if (clock_used) {
                errRaise(SIS_PKG_NAME, OCT_ERROR,
                         "Multiple clocks used on instance %s",
                         ohFormatName(instance));
              } else {
                clock_used = TRUE;
                name = net.contents.net.name;
                node = read_find_or_create_node(network, name);
                control_node = network_get_control(network, node);
                if (control_node == NIL(node_t)) {
                  node = network_add_fake_primary_output(network, control_node);
                  read_change_madeup_name(network, node);
                  control_node = node;
                }
                latch_set_control(latch, control_node);
              }
            }
          }
        }
      }
    }
  }

  if (edge_triggered >= 0 && active_high >= 0) {
    if (edge_triggered == 0) {
      if (active_high == 0) {
        latch_set_type(latch, FALLING_EDGE);
      } else {
        latch_set_type(latch, RISING_EDGE);
      }
    } else {
      if (active_high == 0) {
        latch_set_type(latch, ACTIVE_LOW);
      } else {
        latch_set_type(latch, ACTIVE_HIGH);
      }
    }
  } else {
    latch_set_type(latch, UNKNOWN);
  }

  if (ohGetByPropName(instance, &prop, "INITIAL_VALUE") == OCT_OK) {
    latch_set_initial_value(latch, prop.contents.prop.value.integer);
  } else {
    latch_set_initial_value(latch, 3); /* UNKNOWN */
  }
}

#endif /* SIS */

network_t *read_oct(facet, mappedp) octObject *facet;
int mappedp;
{

  network_t *network, *newnetwork;
  node_t *output, *fanout, *newnode;
#ifdef SIS
  node_t *temp;
  latch_t *latch;
  int is_comb_cell;
#endif
  octObject instance, term, net, prop, bag;
  octGenerator gen;
  octStatus status;

  lsList outputList;
  lsGen outputGen;
  lsGeneric ptr;
  struct termInfo *termInfo;
  int instanceLogicFunction;

  library_t *library;

  node_t *node;

  /* open the facet */
  if (octOpenFacet(facet) < OCT_OK) {
    errRaise(SIS_PKG_NAME, OCT_ERROR, "can not open the facet %s (%s)",
             ohFormatName(facet), octErrorString());
  }

  /*
   * XXX read the required properties to make sure that it
   * is a good facet
   */

  /* force the net names to be unique */
  uniqNames(facet, OCT_NET_MASK);

  /*
   * make sure formal term names are not the same as net names
   * (really only a problem for primary outputs)
   */

  OH_ASSERT(octInitGenContents(facet, OCT_TERM_MASK, &gen));
  while (octGenerate(&gen, &term) == OCT_OK) {
    OH_ASSERT(octGenFirstContainer(&term, OCT_NET_MASK, &net));
    if (strcmp(term.contents.term.name, net.contents.net.name) == 0) {
      /* build a new name */
      char buffer[1024];
      static int count = 0;
      do {
        (void)sprintf(buffer, "%s-%d", term.contents.term.name, count++);
        net.contents.net.name = buffer;
      } while (octGetByName(facet, &net) == OCT_OK);

      OH_ASSERT(octModify(&net));
    }
  }

  network = network_alloc();

  /* is there a library set? */
  if (mappedp) {
    library = lib_get_library();
  } else {
    library = NIL(library_t);
  }

  if (ohGetByBagName(facet, &bag, "INSTANCES") != OCT_OK) {
    errRaise(SIS_PKG_NAME, OCT_ERROR,
             "Missing INSTANCES bag, facet does not follow policy");
  }

  OH_ASSERT(octInitGenContents(&bag, OCT_INSTANCE_MASK, &gen));

  initialize_master_table();

#ifdef SIS
  /* Read in the clocks */
  read_oct_clock(network, facet);
#endif /* SIS */

  while (octGenerate(&gen, &instance) == OCT_OK) {

    if (library) {
      /* see if this is a mapped gate */
      char *lastslash = strrchr(instance.contents.instance.master, '/');
      if (lastslash) {
        char gateName[1024];
        lib_gate_t *gate;

        (void)sprintf(gateName, "%s:%s", lastslash + 1,
                      instance.contents.instance.view);
        if ((gate = lib_get_gate(library, gateName))) {
          array_t *formal_array, *actual_array;
          char **formal_data;
          node_t **actual_data;
          node_t *outnode;
          int i, size = lib_gate_num_in(gate);

          formal_array = array_alloc(char *, size);
          actual_array = array_alloc(node_t *, size);

#ifdef SIS
          is_comb_cell = (lib_gate_type(gate) == COMBINATIONAL);
#endif /* SIS */
          for (i = 0; i < size; i++) {
            OH_ASSERT(ohGetByTermName(&instance, &term,
                                      lib_gate_pin_name(gate, i, 1)));
            OH_ASSERT(octGenFirstContainer(&term, OCT_NET_MASK, &net));
            node = read_find_or_create_node(network, net.contents.net.name);
            array_insert_last(char *, formal_array, term.contents.term.name);
            array_insert_last(node_t *, actual_array, node);
          }

          for (i = 0; i < lib_gate_num_out(gate); i++) {
            OH_ASSERT(ohGetByTermName(&instance, &term,
                                      lib_gate_pin_name(gate, i, 0)));
            OH_ASSERT(octGenFirstContainer(&term, OCT_NET_MASK, &net));
            outnode = read_find_or_create_node(network, net.contents.net.name);
          }
          formal_data = array_data(char *, formal_array);
          actual_data = array_data(node_t *, actual_array);
          array_free(formal_array);
          array_free(actual_array);

#ifdef SIS
          if (!is_comb_cell) {
            if (node_type(outnode) == PRIMARY_INPUT || outnode->F != 0) {
              errRaise(SIS_PKG_NAME, -1, "gate `%s' redefined",
                       node_name(outnode));
            } else {
              network_change_node_type(network, outnode, PRIMARY_INPUT);
            }
          }
#endif /* SIS */

          if (!lib_set_gate(outnode, gate, formal_data, actual_data, size)) {

            errRaise(SIS_PKG_NAME, -1, "error loading gate `%s'", gateName);
          }

#ifdef SIS
          /*
           * TODO: Should see if the clocks need to be attached
           */
#endif /* SIS */

          free(formal_data);
          free(actual_data);
          continue;
        }
      }
    }

#ifdef SIS
    outputList = is_a_logic_cell(&instance, &is_comb_cell);
#else  /* misII style call */
    outputList = is_a_logic_cell(&instance);
#endif /* SIS */

    if (outputList == (lsList)0) {
      continue;
    }

    outputGen = lsStart(outputList);

    while (lsNext(outputGen, &ptr, NIL(lsHandle)) == LS_OK) {

      termInfo = (struct termInfo *)ptr;

      /* see if there is a LOGICFUNCTION on the instance */
      if (ohGetByTermName(&instance, &term, termInfo->name) < OCT_OK) {
        errRaise(SIS_PKG_NAME, OCT_ERROR,
                 "can not get the actual terminal %s of %s (%s)",
                 termInfo->name, ohFormatName(&instance), octErrorString());
      }

      if (ohGetByPropName(&term, &prop, "LOGICFUNCTION") == OCT_OK) {
        if ((newnetwork = read_eqn_string(prop.contents.prop.value.string)) ==
            NIL(network_t)) {
          errRaise(SIS_PKG_NAME, OCT_ERROR, "bad equation %s (%s)",
                   prop.contents.prop.value.string, error_string());
        }
        instanceLogicFunction = 1;
      } else {
        newnetwork = termInfo->logicFunction;
        instanceLogicFunction = 0;
        if (newnetwork == NIL(network_t)) {
          continue;
        }
      }

      /* get the primary output of the network */
      output = network_get_po(newnetwork, 0);

      status = octGenFirstContainer(&term, OCT_NET_MASK, &net);
      if (status == OCT_GEN_DONE) {
        if (instanceLogicFunction) {
          network_free(newnetwork);
        }
        continue;
      }
      if (status < OCT_OK) {
        errRaise(SIS_PKG_NAME, status, octErrorString());
      }

      fanout = read_find_or_create_node(network, net.contents.net.name);

      /* build the new node */
      /* get the internal node - should be the only fanin the the primary output
       */
      newnode = build_new_node(node_get_fanin(output, 0), network, &instance);

#ifdef SIS
      if (is_comb_cell) {
        /* put in the new function */
        if (node_function(fanout) == NODE_UNDEFINED) {
          node_replace(fanout, newnode);
        } else {
          errRaise(SIS_PKG_NAME, OCT_ERROR,
                   "two outputs driving the same node: %s",
                   node_oct_name(fanout));
        }
      } else {
        /* TODO: Handle exotic latches (with feedback) */
        /*
         * The output of the latch is a PI of the network
         */
        if (node_function(fanout) == NODE_UNDEFINED) {
          network_change_node_type(network, fanout, PRIMARY_INPUT);
        } else {
          errRaise(SIS_PKG_NAME, OCT_ERROR,
                   "latch output driving already driven node: %s",
                   node_oct_name(fanout));
        }
        /*
         * Make sure that the node given does not conflict names yet to come
         * Add a blank to the name. Have to remove this later. Create latch
         */
        temp = node_alloc();
        network_add_node(network, temp);
        node_replace(temp, newnode);
        read_change_madeup_name(network, temp);
        temp = network_add_fake_primary_output(network, temp);
        read_change_madeup_name(network, temp);
        network_create_latch(network, &latch, temp, fanout);

        oct_read_set_latch(network, latch, &instance);
      }
#else  /* for misII only comb cells are present */
      /* put in the new function */
      if (node_function(fanout) == NODE_UNDEFINED) {
        node_replace(fanout, newnode);
      } else {
        errRaise(SIS_PKG_NAME, OCT_ERROR,
                 "two outputs driving the same node: %s",
                 node_oct_name(fanout));
      }
#endif /* SIS */

      if (instanceLogicFunction) {
        /* garbage collection */
        network_free(newnetwork);
      }
    }

    (void)lsFinish(outputGen);
  }

  /* determine PIs and POs */

  OH_ASSERT(octInitGenContents(facet, OCT_TERM_MASK, &gen));

  while (octGenerate(&gen, &term) == OCT_OK) {
    node_function_t funct;

    status = octGenFirstContainer(&term, OCT_NET_MASK, &net);
    if (status == OCT_GEN_DONE) {
      continue;
    }
    if (status < OCT_OK) {
      errRaise(SIS_PKG_NAME, status, "oct error (%s)", octErrorString());
    }

    if ((node = network_find_node(network, net.contents.net.name)) ==
        NIL(node_t)) {
      continue;
    }

    funct = node_function(node);

    if (funct == NODE_UNDEFINED) {
      /* primary input */
      network_change_node_type(network, node, PRIMARY_INPUT);
      network_change_node_name(network, node,
                               util_strsav(term.contents.term.name));
    } else {
      node_t *new;
      char *save;
      /*
       * network_add_primary_output takes the name of 'node' and
       * gives it to 'new'; it then gensyms a name for 'node'
       *
       * this is okay if 'node' is an internal node, but it 'node'
       * is a primary input, this is trouble
       *
       * thus the name of node must be saved and replaced
       */
      save = util_strsav(node_oct_name(node));
      new = network_add_primary_output(network, node);

      /*
       * change the name of the output to the name stored in Oct
       */
      network_change_node_name(network, new,
                               util_strsav(term.contents.term.name));
      network_change_node_name(network, node, save);
    }
  }

  /* garbage collect */
  clean_master_table();

  if (!network_check(network)) {
    errRaise(SIS_PKG_NAME, OCT_ERROR, "network check failed (%s)",
             error_string());
  }

  OH_ASSERT(octCloseFacet(facet));

#ifdef SIS
  /* Correct the names that had a blank added to avoid conflicts */
  network_replace_io_fake_names(network);
#endif
  network_set_name(network, ohFormatName(facet));

  return (network);
}

static jmp_buf sis_oct_error_buf;

static void sis_oct_error_handler(pkgName, code, message) char *pkgName;
int code;
char *message;
{
  (void)fprintf(stderr, "%s: error (%d)\n\t%s\n", pkgName, code, message);
  (void)longjmp(sis_oct_error_buf, -1);
  return;
}

int external_read_oct(network, argc, argv) network_t **network;
int argc;
char **argv;
/*
 * read_oct: read a network from an Oct facet
 *
 */
{
  octObject facet;
  int mappedp = 0;

  if ((argc != 2) && (argc != 3)) {
    (void)fprintf(siserr, "usage: read_oct [-m] cell[:view]\n");
    return (1);
  }

  octBegin();

  if (setjmp(sis_oct_error_buf) == 0) {
    errPushHandler(sis_oct_error_handler);

    (void)ohUnpackDefaults(&facet, "r", ":logic:contents");
    if (ohUnpackFacetName(&facet, argv[argc - 1]) < OCT_OK) {
      errRaise(SIS_PKG_NAME, OCT_ERROR, "usage: read_oct cell[:view]");
    }

    error_init();
    if (argc == 3) {
      mappedp = 1;
    }
    *network = read_oct(&facet, mappedp);
    errPopHandler();
  } else {
    return (1);
  }

  octEnd();

  return (0);
}

#endif /* OCT */
