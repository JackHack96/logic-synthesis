
#ifdef SIS
#include "retime_int.h"
#include "sis.h"

#define REAL 1
#define INT 2

#define D_MIN(a, b) ((double)a < (double)b ? (double)a : (double)b)

#define SCALE 10000
#define ALLOC_EDGES 20

struct a_graph {
  struct a_node **nodes;
  struct a_edge **edges;
  int num_edges;
  int num_edges_alloc;
  int num_int;
  int num_total;
};

struct a_node {
  struct a_edge *fanin;
  double r;
  double y;
  double x;
};

struct a_edge {
  int from;
  int to;
  double weight_a;
  double weight_b;
  int type;
  struct a_edge *next;
};

static void re_free_lies_graph();
static void re_add_usertime_constraints();
static void re_add_vertex_constraints();
static void re_add_edge_constraint();
static void re_node_to_index();

/*
 * Compute the retiming to meet the specified time "c" by solving the
 * MILP (Mixed Integer Linear Program) formulation of the problem
 */

/* ARGSUSED */
int retime_lies_routine(graph, c, retiming) re_graph *graph;
double c;
int *retiming; /* Array of adequate length to hold the solution */
{
  struct a_graph *milp_graph;
  int i, j, k, count, num_int, num_total;
  double arrival;
  char *key, *value;
  re_node *re_no;
  re_edge *re_ed;
  struct a_edge *edge;
  struct a_node **nodes;
  avl_tree *tree;
  avl_generator *gen;
  st_table *table;
  int to, from, lag;

  milp_graph = ALLOC(struct a_graph, 1);
  count = 0;
  milp_graph->num_edges = 0;
  milp_graph->num_edges_alloc = ALLOC_EDGES;
  num_int = milp_graph->num_int = re_num_internals(graph) + 1;
  num_total = milp_graph->num_total = 2 * num_int;
  milp_graph->nodes = ALLOC(struct a_node *, num_total);
  for (i = 0; i < num_total; i++) {
    milp_graph->nodes[i] = ALLOC(struct a_node, 1);
    (milp_graph->nodes[i])->r = 0.0;
    (milp_graph->nodes[i])->y = 0.0;
    (milp_graph->nodes[i])->fanin = NIL(struct a_edge);
  }
  milp_graph->edges = ALLOC(struct a_edge *, milp_graph->num_edges_alloc);
  for (i = milp_graph->num_edges_alloc; i-- > 0;) {
    milp_graph->edges[i] = ALLOC(struct a_edge, 1);
  }
  /*
   * Insert the constraints corresponding to the retime graph
   * by creating the required edges between the a_node entries
   * Keep the correspondence between the re_no and the index of
   * the node in the milp_graph->nodes
   */
  table = st_init_table(st_ptrcmp, st_ptrhash);
  re_foreach_node(graph, i, re_no) {
    if (re_no->type == RE_INTERNAL) {
      count++;
      (void)st_insert(table, (char *)(re_no), (char *)count);
    }
  }
  assert(count == (num_int - 1));

  /*
   * Vertex constraints for the host vertex
   */
  re_add_vertex_constraints(milp_graph, NIL(re_node), 0, c);
  re_foreach_node(graph, i, re_no) {
    if (re_no->type == RE_PRIMARY_OUTPUT) {
      /*
       * Add the edges between the host vertex and the immediate fanin
       */
      to = 0;
      re_foreach_fanin(re_no, j, re_ed) {
        re_node_to_index(table, re_ed->source, &from);
        re_add_edge_constraint(milp_graph, from, to, re_ed, c);
      }
    } else if (re_no->type == RE_INTERNAL) {
      re_node_to_index(table, re_no, &to);
      /*
       * Add the two edges corresponding to the vertex constraints
       */
      re_add_vertex_constraints(milp_graph, re_no, to, c);

      /*
       * Add the two edges corresponding to the edge constraints
       */
      re_foreach_fanin(re_no, j, re_ed) {
        re_node_to_index(table, re_ed->source, &from);
        re_add_edge_constraint(milp_graph, from, to, re_ed, c);
      }
    }

    /*
     * Now add the constraints that might result from the user specifying
     * arrival and required times at the host vertices
     */
    re_add_usertime_constraints(milp_graph, table, re_no, c);
  }
  nodes = milp_graph->nodes;

  /* Now carry out the MILP solution -- ala the Lieserson paper */
  /* T1-T4 */
  for (i = num_int; i < num_total; i++) {
    for (j = num_int; j < num_total; j++) {
      for (edge = nodes[j]->fanin; edge != NIL(struct a_edge);
           edge = edge->next) {
        (nodes[j])->r =
            D_MIN((nodes[j])->r, (nodes[edge->from])->r + edge->weight_a);
      }
    }
  }
  /* T5-T6 */
  for (j = num_int; j < num_total; j++) {
    for (edge = nodes[j]->fanin; edge != NIL(struct a_edge);
         edge = edge->next) {
      if (nodes[j]->r > nodes[edge->from]->r + edge->weight_a) {
        if (retime_debug > 40) {
          (void)fprintf(sisout,
                        "\tMILP_FAIL1 :%d to %d, %5.2f > %5.2f + %5.2f \n",
                        edge->from, edge->to, nodes[j]->r, nodes[edge->from]->r,
                        edge->weight_a);
        }
        st_free_table(table);
        re_free_lies_graph(milp_graph);
        return 0;
      }
    }
  }
  /* T7-T8 */
  for (i = milp_graph->num_edges; i-- > 0;) {
    edge = (milp_graph->edges)[i];
    edge->weight_b = edge->weight_a + nodes[edge->from]->r - nodes[edge->to]->r;
    if (edge->type == REAL)
      assert(edge->weight_b >= 0);
  }
  /* T10-T22 */
  for (k = 0; k < num_int; k++) {
    tree = avl_init_table(avl_numcmp);
    for (i = 0; i < num_int; i++) {
      for (edge = nodes[i]->fanin; edge != NIL(struct a_edge);
           edge = edge->next) {
        nodes[edge->to]->y = D_MIN(
            nodes[edge->to]->y, floor(nodes[edge->from]->y + edge->weight_b));
      }
    }
    /* T15-T21 */
    for (i = 0; i < num_total; i++) {
      (void)avl_insert(tree, (char *)((int)floor(SCALE * nodes[i]->y)),
                       (char *)i);
    }
    gen = avl_init_gen(tree, AVL_FORWARD);
    while (avl_gen(gen, &key, &value)) {
      i = (int)value;
      /* T19-T21 */
      for (j = num_int; j < num_total; j++) {
        for (edge = nodes[j]->fanin; edge != NIL(struct a_edge);
             edge = edge->next) {
          if (edge->from == i) {
            nodes[edge->to]->y =
                D_MIN(nodes[edge->to]->y, nodes[i]->y + edge->weight_b);
          }
        }
      }
    }
    avl_free_gen(gen);
    avl_free_table(tree, (void (*)())0, (void (*)())0);
  }
  /* T23-T26 */
  for (i = 0; i < num_int; i++) {
    for (edge = nodes[i]->fanin; edge != NIL(struct a_edge);
         edge = edge->next) {
      if (nodes[edge->to]->y > (nodes[edge->from]->y + edge->weight_b)) {
        if (retime_debug > 40) {
          (void)fprintf(sisout,
                        "\tMILP_FAIL2: %d to %d, %5.2f > %5.2f + %5.2f \n",
                        edge->from, edge->to, nodes[edge->to]->y,
                        nodes[edge->from]->y, edge->weight_b);
        }
        st_free_table(table);
        re_free_lies_graph(milp_graph);
        return 0;
      }
    }
  }
  /*
   * Scale everything so that the host has a integral variable == 0
   */
  if (retime_debug > 20)
    (void)fprintf(sisout, "\n\tInteger part(lags) \n\t");
  nodes[0]->x = nodes[0]->y + nodes[0]->r;
  for (i = 1; i < num_int; i++) {
    nodes[i]->x = nodes[i]->y + nodes[i]->r - nodes[0]->x;
    if (retime_debug > 20) {
      (void)fprintf(sisout, "%5d ", (int)(nodes[i]->x));
    }
  }
  if (retime_debug > 20)
    (void)fprintf(sisout, "\n\tReal part(arrival times)\n\t");
  nodes[num_int]->x = nodes[num_int]->y + nodes[num_int]->r - nodes[0]->x;
  for (i = num_int + 1; i < num_total; i++) {
    nodes[i]->x = nodes[i]->y + nodes[i]->r - nodes[0]->x;
    if (retime_debug > 20) {
      arrival = c * (nodes[i]->x - nodes[i - num_int]->x);
      (void)fprintf(sisout, "%5.2f ", arrival);
    }
  }
  if (retime_debug > 20)
    (void)fprintf(sisout, "\n");

  /* Build up the soultion vector */
  if (retime_debug)
    (void)fprintf(sisout, "\tRetiming following nodes\n\t");
  re_foreach_node(graph, i, re_no) {
    retiming[i] = 0;
    if (re_no->type == RE_INTERNAL) {
      re_node_to_index(table, re_no, &from);
      lag = (int)(nodes[from]->x);
      if (retime_debug && lag != 0) {
        (void)fprintf(sisout, "%s by %d  ", re_no->node->name,
                      (int)(nodes[from]->x));
      }
      retiming[i] = lag;
      retime_single_node(re_no, lag);
    }
  }
  if (retime_debug)
    (void)fprintf(sisout, "\n");

  st_free_table(table);
  re_free_lies_graph(milp_graph);
  return 1;
}

static void re_free_lies_graph(milp_graph) struct a_graph *milp_graph;
{
  int i;
  struct a_node **nodes;

  nodes = milp_graph->nodes;
  for (i = milp_graph->num_edges_alloc; i-- > 0;) {
    FREE((milp_graph->edges)[i]);
  }
  for (i = milp_graph->num_total; i-- > 0;) {
    FREE(nodes[i]);
  }
  FREE(milp_graph->nodes);
  FREE(milp_graph->edges);
  FREE(milp_graph);
}

/*
 * from(u) is a fanin of to(v) in the orig re_graph
 */
static void re_add_edge_constraint(graph, from, to, edge,
                                   c) struct a_graph *graph;
int from, to;
re_edge *edge;
double c;
{
  int count;
  double node_delay;
  struct a_edge **edge_array;
  struct a_node *u_no;

  node_delay = (to == 0 ? -c : edge->sink->final_delay);

  count = graph->num_edges;
  re_add_edges_to_graph(graph, 2); /* Need two more edges */

  edge_array = graph->edges;

  /* The r(u)-r(v) <= w(e) : from(u) is a fanin to to(v) in the re_graph*/
  u_no = (graph->nodes)[from];
  edge_array[count]->from = to;
  edge_array[count]->to = from;
  edge_array[count]->weight_a = (double)(edge->weight);
  edge_array[count]->type = INT;
  edge_array[count]->next = u_no->fanin;
  u_no->fanin = edge_array[count];
#ifdef RE_DEBUG
  if (retime_debug > 40) {
    (void)fprintf(sisout, "constraint3: %d to %d  = %5.2f\n",
                  edge_array[count]->from, edge_array[count]->to,
                  edge_array[count]->weight_a);
  }
#endif
  count++;
  /* The R(u)-R(v) <= w(e) - d(v)/c */
  u_no = (graph->nodes)[from + graph->num_int];
  edge_array[count]->from = to + graph->num_int;
  edge_array[count]->to = from + graph->num_int;
  edge_array[count]->weight_a = (double)(edge->weight) - (node_delay) / c;
  edge_array[count]->type = REAL;
  edge_array[count]->next = u_no->fanin;
  u_no->fanin = edge_array[count];
#ifdef RE_DEBUG
  if (retime_debug > 40) {
    (void)fprintf(sisout, "constraint4: %d to %d  = %5.2f\n",
                  edge_array[count]->from, edge_array[count]->to,
                  edge_array[count]->weight_a);
  }
#endif
  count++;

  graph->num_edges = count;
}

static void re_add_vertex_constraints(graph, re_no, to,
                                      c) struct a_graph *graph;
re_node *re_no;
int to;
double c;
{
  int count, num_int;
  double node_delay;
  struct a_edge **edge_array;
  struct a_node *i_no, *r_no;

  node_delay = (to == 0 ? -c : re_no->final_delay);
  num_int = graph->num_int;
  count = graph->num_edges;
  i_no = (graph->nodes)[to];
  r_no = (graph->nodes)[to + num_int];
  re_add_edges_to_graph(graph, 2);

  edge_array = graph->edges;

  /* The r(v)-R(v) <= -d(v)/c ; edge incident on the integer node*/
  edge_array[count]->from = to + num_int;
  edge_array[count]->to = to;
  edge_array[count]->weight_a = -(node_delay / c);
  edge_array[count]->type = INT;
  edge_array[count]->next = i_no->fanin;
  i_no->fanin = edge_array[count];
#ifdef RE_DEBUG
  if (retime_debug > 40) {
    (void)fprintf(sisout, "\tconstraint1: %d to %d  = %5.2f\n",
                  edge_array[count]->from, edge_array[count]->to,
                  edge_array[count]->weight_a);
  }
#endif
  count++;

  /* The R(v)-r(v) <= c; edge is incident on the real node */
  edge_array[count]->to = to + num_int;
  edge_array[count]->from = to;
  edge_array[count]->weight_a = 1; /* Maybe RHS should be 1 */
  edge_array[count]->type = REAL;
  edge_array[count]->next = r_no->fanin;
  r_no->fanin = edge_array[count];
#ifdef RE_DEBUG
  if (retime_debug > 40) {
    (void)fprintf(sisout, "\tconstraint2: %d to %d  = %5.2f\n",
                  edge_array[count]->from, edge_array[count]->to,
                  edge_array[count]->weight_a);
  }
#endif
  count++;

  graph->num_edges = count;
}

static void re_add_usertime_constraints(graph, table, re_no,
                                        c) struct a_graph *graph;
st_table *table; /* Stores the index corresponding to the node */
re_node *re_no;
double c;
{
  re_edge *re_ed;
  double node_delay, user_time;
  struct a_edge **edge_array;
  int i, count, num_int, from, to, index;

  if ((re_no->type == RE_INTERNAL) || (re_no->user_time < RETIME_TEST_NOT_SET))
    return;

  count = graph->num_edges;
  num_int = graph->num_int;
  edge_array = graph->edges;

  if (re_no->type == RE_PRIMARY_INPUT) {
    /* Allocte the edges in the milp graph structure */
    re_add_edges_to_graph(graph, re_num_fanouts(re_no));

    /*
     * Constraints of the form r(h) - R(v) <= w(e) - d(v)/c - t/c
     * where "t" is the time AFTER the edge that the data is available
     * The constraint edge is incident on the integer node r(h) ...
     */
    re_foreach_fanout(re_no, i, re_ed) {
      re_node_to_index(table, re_ed->sink, &index);
      node_delay = re_ed->sink->final_delay;
      user_time = re_no->user_time;
      from = index + num_int;
      to = 0;

      edge_array[count]->from = from;
      edge_array[count]->to = to;
      edge_array[count]->weight_a =
          re_ed->weight - (user_time + node_delay) / c;
      edge_array[count]->type = INT;
      edge_array[count]->next = ((graph->nodes)[to])->fanin;
      ((graph->nodes)[to])->fanin = edge_array[count];
      count++;
    }
  } else {
    /* Allocte the edges in the milp graph structure */
    re_add_edges_to_graph(graph, re_num_fanins(re_no));

    /*
     * Constraint of the form R(v) - r(h) <= t/c + w(e)
     * where "t" is the time AFTER the clock edge that the data is
     * required to be present at the output... The edge is incident on the
     * REAL node
     */
    re_foreach_fanin(re_no, i, re_ed) {
      re_node_to_index(table, re_ed->source, &index);
      /* User time is referenced to the start of the clock */
      user_time = (c + re_no->user_time);
      from = 0;
      to = index + num_int;

      edge_array[count]->from = from;
      edge_array[count]->to = to;
      edge_array[count]->weight_a = re_ed->weight + user_time / c;
      edge_array[count]->type = REAL;
      edge_array[count]->next = ((graph->nodes)[to])->fanin;
      ((graph->nodes)[to])->fanin = edge_array[count];
      count++;
    }
  }
  graph->num_edges = count;
}

re_add_edges_to_graph(graph, num) struct a_graph *graph;
int num;
{
  int i, count, alloc_count;
  struct a_edge **edge_array;

  count = graph->num_edges;
  alloc_count = graph->num_edges_alloc;
  edge_array = graph->edges;

  if (count + num >= graph->num_edges_alloc) {
    alloc_count += ALLOC_EDGES;
    edge_array = REALLOC(struct a_edge *, edge_array, alloc_count);
    for (i = ALLOC_EDGES; i > 0; i--) {
      edge_array[alloc_count - i] = ALLOC(struct a_edge, 1);
    }
  }
  graph->num_edges_alloc = alloc_count;
  graph->edges = edge_array;
}

static void re_node_to_index(table, node, index) st_table *table;
re_node *node;
int *index;
{
  if (node->type == RE_INTERNAL) {
    if (!st_lookup(table, (char *)node, (char **)index)) {
      fail("Hey, howcome node not assigned an index");
    }
  } else {
    *index = 0;
  }
}

#endif /* SIS */
