
/* file cluster.c release 1.8 */
/* last modified: 7/22/91 at 12:36:15 */
/*
 * $Log: cluster.c,v $
 * Revision 1.1.1.1  2004/02/07 10:14:24  pchong
 * imported
 *
 * Revision 1.7  1993/08/05  16:14:19  sis
 * Added default case to switch statement so gcc version will work.
 *
 * Revision 1.6  1993/07/14  18:10:07  sis
 * -f option added to reduce_depth (Herve Touati)
 *
 * Revision 1.6  1993/06/22  03:49:35  touati
 * Add option -f to reduce_depth: poor man's version of flowmap for TLU FPGAs.
 *
 * Revision 1.5  22/.0/.1  .0:.4:.4  sis
 *  Updates for the Alpha port.
 *
 * Revision 1.4  1992/05/06  18:55:51  sis
 * SIS release 1.1
 *
 * Revision 1.3  1992/04/17  21:51:27  sis
 * *** empty log message ***
 *
 * Revision 1.2  1992/03/10  19:15:36  sis
 * Changed network_sweep to network_csweep so the latches
 * are not removed during the sweep.
 *
 * Revision 1.2  1992/03/10  19:15:36  sis
 * Changed network_sweep to network_csweep so the latches
 * are not removed during the sweep.
 *
 * Revision 1.2  1992/03/02  23:58:59  cmoon
 * changed network_sweep to network_csweep
 *
 * Revision 1.1  92/01/08  17:40:32  sis
 * Initial revision
 *
 * Revision 1.1  91/07/02  10:49:29  touati
 * Initial revision
 *
 */
#include "cluster.h"
#include "map_defs.h"
#include "sis.h"

/* this file implements Lawler's clustering algorithm */
/* for a given limit in size of clusters, find a minimum depth */
/* clustering. All nodes are supposed to be of size 1. */
/* When all nodes are of the same size, clustering guarantees */
/* that each cluster has only one external output (root of cluster) */
/* then we collapse each cluster into its root node */

#define MAX_LEVEL INFINITY

/* a cluster node is a ROOT_NODE iff it fan outs to a node with a larger label
 */

typedef enum { NORMAL_NODE, ROOT_NODE } clust_node_type_t;

typedef struct {
  int number;
  st_table *nodes; /* tells the nodes that it contains */
  node_t *root;    /* the root of the cluster */
  int label;       /* the label, common to the nodes of the cluster */
} cluster_t;

typedef struct {
  int label;
  int max_label; /* needed by relabelling algorithm */
  int weight;
  clust_node_type_t type;
  array_t
      *clusters; /* list of clusters to which this node belongs (cluster_t *) */
} clust_node_t;

#include "cluster_static.h"

static clust_options_t global_settings;

/* EXTERNAL INTERFACE */

network_t *cluster_under_constraint(network, options) network_t *network;
clust_options_t *options;
{
  network_t *new_network = NIL(network_t);
  global_settings = *options;

  switch (options->type) {
  case SIZE_CONSTRAINT:
    new_network = cluster_under_size_constraint(network, options);
    break;
  case DEPTH_CONSTRAINT:
    new_network = cluster_under_depth_constraint(network, options);
    break;
  case SIZE_AS_DEPTH_CONSTRAINT:
    new_network = cluster_under_size_as_depth_constraint(network, options);
    break;
  case CLUSTER_STATISTICS:
    gather_cluster_statistics(network, options);
    break;
  case BEST_RATIO_CONSTRAINT:
    new_network = cluster_under_best_ratio(network, options);
    break;
  case FANIN_CONSTRAINT:
    new_network = cluster_under_size_constraint(network, options);
    break;
  default:;
  }
  return new_network;
}

/* INTERNAL INTERFACE */

/* Lawler's algorithm: given a max cluster size, returns the best labelling */
/* form and collapse the clusters */
/* if flag "relabel" is set, try to relabel nodes to control duplication of
 * logic */
/* without affecting delay */

static network_t *cluster_under_size_constraint(network,
                                                options) network_t *network;
clust_options_t *options;
{
  int max_label;
  array_t *nodevec;
  array_t *clusters; /* array of cluster_t * */
  st_table *table;

  nodevec = network_dfs(network);

  table = clust_node_table_alloc();
  if (options->verbose >= 2)
    (void)fprintf(misout, "------ labelling nodes-------\n");
  max_label =
      label_nodes(table, nodevec, options->cluster_size, options->verbose);
  if (options->verbose >= 1)
    (void)fprintf(misout, "*** # levels = %d\n", max_label + 1);

  if (options->relabel) {
    relabel_nodes(table, nodevec, max_label, options->verbose);
  }

  if (options->verbose >= 2)
    (void)fprintf(misout, "------ forming clusters-------\n");
  clusters = array_alloc(cluster_t *, 0);
  form_clusters(table, nodevec, clusters);
  clust_node_table_free(table);

  if (options->verbose >= 2) {
    (void)fprintf(misout, "------ printing clusters-------\n");
    print_clusters(clusters);
  }

  if (options->verbose >= 2)
    (void)fprintf(misout, "------ collapsing clusters-------\n");
  collapse_clusters(clusters, nodevec);

  /* remove intermediate nodes not roots of clusters */
  network_csweep(network);

  array_free(nodevec);
  clusters_free(clusters);

  return network;
}

/* built above Lawler's algorithm: finds the minimum cluster size for which */
/* the label can attain "depth"; select that cluster size and call Lawler's
 * algorithm */

static network_t *cluster_under_depth_constraint(network,
                                                 options) network_t *network;
clust_options_t *options;
{
  array_t *nodevec;
  int max_label;
  int min_cluster_size;
  int max_cluster_size;
  clust_options_t new_options;

  nodevec = network_dfs(network);

  min_cluster_size = 1;
  max_cluster_size = network_num_internal(network);
  max_label = options->depth - 1;

  new_options = *options;
  new_options.type = SIZE_CONSTRAINT;
  new_options.cluster_size = lazy_binary_search(
      min_cluster_size, max_cluster_size, max_label, nodevec, best_labelling);

  if (options->verbose >= 1)
    (void)fprintf(misout, "*** max cluster size = %d\n",
                  new_options.cluster_size);
  array_free(nodevec);

  return cluster_under_size_constraint(network, &new_options);
}

/* built above the previous one. Find the minimum size giving the same */
/* depth as the size passed as argument and use it for Lawler's algorithm */

static network_t *
    cluster_under_size_as_depth_constraint(network, options) network_t *network;
clust_options_t *options;
{
  array_t *nodevec;
  int max_label;
  int min_cluster_size;
  int max_cluster_size;
  clust_options_t new_options;

  nodevec = network_dfs(network);

  min_cluster_size = 1;
  max_cluster_size = options->cluster_size;
  max_label = best_labelling(nodevec, options->cluster_size);

  new_options = *options;
  new_options.type = SIZE_CONSTRAINT;
  new_options.cluster_size = lazy_binary_search(
      min_cluster_size, max_cluster_size, max_label, nodevec, best_labelling);

  if (options->verbose >= 1)
    (void)fprintf(misout, "*** max cluster size = %d\n",
                  new_options.cluster_size);
  array_free(nodevec);

  return cluster_under_size_constraint(network, &new_options);
}

static void gather_cluster_statistics(network, options) network_t *network;
clust_options_t *options;
{
  int i;
  array_t *nodevec;
  int min_cluster_size;
  int max_cluster_size;
  int max_label;
  st_table *table;
  double dup_ratio, red_dup_ratio;
  array_t *clusters;

  nodevec = network_dfs(network);

  min_cluster_size = 1;
  max_cluster_size = network_num_internal(network);

  for (i = min_cluster_size; i <= max_cluster_size; i++) {
    table = clust_node_table_alloc();
    max_label = label_nodes(table, nodevec, i, options->verbose);
    clusters = array_alloc(cluster_t *, 0);
    form_clusters(table, nodevec, clusters);
    dup_ratio = duplication_ratio(network, clusters);
    clusters_free(clusters);

    reinit_cluster_info(table);
    relabel_nodes(table, nodevec, max_label, options->verbose);
    clusters = array_alloc(cluster_t *, 0);
    form_clusters(table, nodevec, clusters);
    red_dup_ratio = duplication_ratio(network, clusters);
    clust_node_table_free(table);
    clusters_free(clusters);

    (void)fprintf(misout, "cluster_size = %4d ", i);
    (void)fprintf(misout, "# levels = %4d ", max_label + 1);
    (void)fprintf(misout, "dup_ratio = %2.2f ", dup_ratio);
    (void)fprintf(misout, "red_dup_ratio = %2.2f\n", red_dup_ratio);

    if (max_label == 0)
      break;
  }

  array_free(nodevec);
}

/*
 * Heuristics to choose a good cluster size to cluster with.
 * try labelling and clustering (but no collapsing, no modification of network)
 * for all cluster sizes from 2 to the first cluster size leading to a
 * duplication ratio larger than options->dup_ratio. Record the cluster_size
 * corresponding to the smallest duplication ratio among those leading to the
 * smallest value for max_label. If none, simply take cluster_size = 2. Then do
 * the clustering for that size.
 */

static network_t *cluster_under_best_ratio(network, options) network_t *network;
clust_options_t *options;
{
  int i;
  array_t *nodevec;
  int min_cluster_size;
  int max_cluster_size;
  int max_label;
  st_table *table;
  double dup_ratio;
  array_t *clusters;
  int best_cluster_size;
  int best_level;
  double best_dup_ratio;
  clust_options_t new_options;

  nodevec = network_dfs(network);

  min_cluster_size = 2;
  max_cluster_size = network_num_internal(network);

  best_level = INFINITY;
  best_dup_ratio = INFINITY;
  best_cluster_size = -1;

  /* find the best size */

  for (i = min_cluster_size; i <= max_cluster_size; i++) {
    table = clust_node_table_alloc();
    max_label = label_nodes(table, nodevec, i, options->verbose);
    relabel_nodes(table, nodevec, max_label, options->verbose);
    clusters = array_alloc(cluster_t *, 0);
    form_clusters(table, nodevec, clusters);
    dup_ratio = duplication_ratio(network, clusters);
    clust_node_table_free(table);
    clusters_free(clusters);

    if (dup_ratio >= options->dup_ratio)
      break;
    if (max_label == 0)
      break;
    if (max_label < best_level ||
        (max_label == best_level && dup_ratio < best_dup_ratio)) {
      best_level = max_label;
      best_dup_ratio = dup_ratio;
      best_cluster_size = i;
    }
  }

  if (best_cluster_size == -1)
    best_cluster_size = 2;
  array_free(nodevec);

  new_options = *options;
  new_options.type = SIZE_CONSTRAINT;
  new_options.cluster_size = best_cluster_size;
  if (options->verbose >= 1)
    (void)fprintf(misout, "*** cluster size = %d\n", new_options.cluster_size);

  return cluster_under_size_constraint(network, &new_options);
}

/* LABELLING */

/* visits all internal nodes in topological order from inputs to outputs */
/* allocate a clust_node_t for each node, and put the map "node --> clust_node"
 * in 'table'. */

/* ARGSUSED */
static int label_nodes(table, nodevec, cluster_size, verbose) st_table *table;
array_t *nodevec;
int cluster_size;
int verbose;
{
  int i;
  int label;
  node_t *node;
  int max_label = -1;
  int nsize = array_n(nodevec);
  clust_node_t *node_info;

  for (i = 0; i < nsize; i++) {
    node = array_fetch(node_t *, nodevec, i);
    if (node->type != INTERNAL)
      continue;
    node_info = clust_node_alloc(table, node);
    label = decide_label_node(table, node, node_info, cluster_size);
    if (label > max_label)
      max_label = label;
  }
  return max_label;
}

/* see comments on typedef clust_node_t */

static clust_node_t *clust_node_alloc(table, node) st_table *table;
node_t *node;
{
  clust_node_t *node_info;

  node_info = ALLOC(clust_node_t, 1);
  node_info->label = -1;
  node_info->weight = 1;
  node_info->type = NORMAL_NODE;
  node_info->clusters = array_alloc(cluster_t *, 0);
  st_insert(table, (char *)node, (char *)node_info);
  return node_info;
}

/*
 * The procedure looks at the node and its fanins and decides what
 * label should be given to the node. It puts the label in the node.
 * Returns a label (int)
 */

static int decide_label_node(table, node, node_info,
                             cluster_size) st_table *table;
node_t *node;
clust_node_t *node_info;
int cluster_size;
{
  int max_fanin_label;
  int total_weight;

  /*
   * extract the largest label of any immediate fanin of node
   */

  max_fanin_label = get_max_fanin_label(table, node);

  /*
   * max_fanin_label == -1 iff node is constant node or node with all fanins
   * PI's
   */

  if (max_fanin_label == -1) {
    node_info->label = 0;
    return 0;
  }

  /*
   * sums the weight of all nodes in tfi of node with label 'max_fanin_label'
   * the only difficulty is to make sure not to count the same node twice
   * total_weight includes the weight of 'node'; need to set node_info->label
   * to max_label so that total_weight is computed properly.
   */

  node_info->label = max_fanin_label;
  total_weight =
      compute_tfi_weight_for_given_label(table, node, max_fanin_label);

  if (total_weight > cluster_size) {
    node_info->label = max_fanin_label + 1;
  }

  return node_info->label;
}

/*
 * returns -1 iff fanins of node are all PI or no fanin at all (constant node)
 */

static int get_max_fanin_label(table, node) st_table *table;
node_t *node;
{
  int i;
  node_t *fanin;
  clust_node_t *node_info;
  int max_label = -1;

  foreach_fanin(node, i, fanin) {
    if (fanin->type == PRIMARY_INPUT)
      continue;
    assert(st_lookup(table, (char *)fanin, (char **)&node_info));
    max_label = MAX(max_label, node_info->label);
  }
  return max_label;
}

/* Relies on the fact that labels can only increase along a directed path */
/* to reduce the search of the tfi */
/* Usually the weight is the number of nodes in the cluster */
/* however, under the option FANIN_CONSTRAINT */
/* the weight is the number of distinct inputs */
/* 'tfi_fanin' is only used under the option 'FANIN_CONSTRAINT' */

static int compute_tfi_weight_for_given_label(table, node,
                                              label) st_table *table;
node_t *node;
int label;
{
  int weight = 0;
  st_table *tfi_fanin = st_init_table(st_ptrcmp, st_ptrhash);
  st_table *tfi_for_given_label = st_init_table(st_ptrcmp, st_ptrhash);

  compute_tfi_for_given_label_rec(table, node, label, tfi_for_given_label,
                                  tfi_fanin);

  if (global_settings.type == FANIN_CONSTRAINT) {
    weight = st_count(tfi_fanin);
  } else {
    st_generator *gen;
    clust_node_t *node_info;
    st_foreach_item(tfi_for_given_label, gen, (char **)&node,
                    (char **)&node_info) {
      weight += node_info->weight;
    }
  }
  st_free_table(tfi_for_given_label);
  st_free_table(tfi_fanin);
  return weight;
}

/* Only stores in 'visited' the internal nodes belonging to tfi of 'node' */
/* and having 'label' as label. */
/* Stores in 'visited_fanins' the fanins of the set of nodes in 'visited' */
/* that do not belong to 'visited'. */

static void compute_tfi_for_given_label_rec(table, node, label, visited,
                                            visited_fanins) st_table *table;
node_t *node;
int label;
st_table *visited;
st_table *visited_fanins;
{
  int i;
  node_t *fanin;
  clust_node_t *node_info;

  /* already visited */
  if (st_lookup(visited, (char *)node, NIL(char *)))
    return;

  /* special case of PI */
  if (node->type == PRIMARY_INPUT) {
    st_insert(visited_fanins, (char *)node, NIL(char));
    return;
  }

  assert(st_lookup(table, (char *)node, (char **)&node_info));

  /* stop the recursion when a node with smaller label is found */
  if (node_info->label < label) {
    st_insert(visited_fanins, (char *)node, NIL(char));
    return;
  }

  /* the general case */
  st_insert(visited, (char *)node, (char *)node_info);
  foreach_fanin(node, i, fanin) {
    compute_tfi_for_given_label_rec(table, fanin, label, visited,
                                    visited_fanins);
  }
}

/* RELABELLING */

/*
 * try to split clusters to avoid too much logic duplication
 * if not on the critical path.
 * new_max_label is just there to check we have not done something wrong
 *
 * visit all INTERNAL nodes in topological order, from outputs to inputs
 */

static void relabel_nodes(table, nodevec, max_label, verbose) st_table *table;
array_t *nodevec;
int max_label;
int verbose;
{
  int i, j;
  node_t *fanin;
  node_t *node;
  int fanin_label_limit;
  clust_node_t *node_info;
  clust_node_t *fanin_info;
  int new_max_label = -1;
  int n_nodes = array_n(nodevec);

  /*
   * first need to initialize all the max_label fields
   */

  for (i = n_nodes - 1; i >= 0; i--) {
    node = array_fetch(node_t *, nodevec, i);
    if (node->type != INTERNAL)
      continue;
    assert(st_lookup(table, (char *)node, (char **)&node_info));
    node_info->max_label = max_label;
  }

  /*
   * can update the node_info->label field only when
   * node_info->label information has been used
   */

  for (i = n_nodes - 1; i >= 0; i--) {
    node = array_fetch(node_t *, nodevec, i);
    if (node->type != INTERNAL)
      continue;
    assert(st_lookup(table, (char *)node, (char **)&node_info));
    foreach_fanin(node, j, fanin) {
      if (fanin->type != INTERNAL)
        continue;
      assert(st_lookup(table, (char *)fanin, (char **)&fanin_info));
      if (fanin_info->label == node_info->label) {
        fanin_label_limit = node_info->max_label;
      } else {
        fanin_label_limit = node_info->max_label - 1;
      }
      fanin_info->max_label = MIN(fanin_info->max_label, fanin_label_limit);
    }

    if (verbose) { /* debug */
      (void)fprintf(misout, "label of %s = %d", node_long_name(node),
                    node_info->label);
      if (node_info->label < node_info->max_label)
        (void)fprintf(misout, " -> %d\n", node_info->max_label);
      (void)fprintf(misout, "\n");
    }

    node_info->label = node_info->max_label;
    new_max_label =
        MAX(new_max_label, node_info->label); /* for consistency check */
  }
  assert(new_max_label == max_label);
}

/* CLUSTERING */

/*
 * visit internal nodes in topological order, from outputs to inputs
 *
 */

static void form_clusters(table, nodevec, clusters) st_table *table;
array_t *nodevec;
array_t *clusters;
{
  int i;
  lsGen gen;
  node_t *node;
  node_t *fanout;
  int max_fanout_label;
  cluster_t *new_cluster;
  clust_node_t *node_info;
  clust_node_t *fanout_info;
  int n_nodes = array_n(nodevec);

  /*
   * check whether a node has a fanout to another node with a larger label
   * if yes, create a new cluster; if no, don't. Force cluster creation if
   * fanouts to a PO
   */

  for (i = n_nodes - 1; i >= 0; i--) {
    node = array_fetch(node_t *, nodevec, i);
    if (node->type != INTERNAL)
      continue;
    assert(st_lookup(table, (char *)node, (char **)&node_info));
    max_fanout_label = node_info->label;
    foreach_fanout(node, gen, fanout) {
      if (fanout->type == PRIMARY_OUTPUT) {
        max_fanout_label = INFINITY;
        continue;
      }
      assert(st_lookup(table, (char *)fanout, (char **)&fanout_info));
      max_fanout_label = MAX(max_fanout_label, fanout_info->label);
    }
    if (max_fanout_label > node_info->label) {
      new_cluster = cluster_alloc();
      node_info->type = ROOT_NODE;
      new_cluster->root = node;
      new_cluster->label = node_info->label;
      array_insert_last(cluster_t *, clusters, new_cluster);
      put_tfi_in_cluster_rec(table, new_cluster, node, node_info->label);
    }
  }
}

static void put_tfi_in_cluster_rec(table, cluster, node, label) st_table *table;
cluster_t *cluster;
node_t *node;
int label;
{
  int i;
  node_t *fanin;
  clust_node_t *node_info;

  if (node->type == PRIMARY_INPUT)
    return;
  if (st_lookup(cluster->nodes, (char *)node, NIL(char *)))
    return;
  assert(st_lookup(table, (char *)node, (char **)&node_info));
  if (node_info->label < label)
    return;
  st_insert(cluster->nodes, (char *)node, NIL(char));
  array_insert_last(cluster_t *, node_info->clusters, cluster);

  foreach_fanin(node, i, fanin) {
    put_tfi_in_cluster_rec(table, cluster, fanin, label);
  }
}

/* DUPLICATION ESTIMATE */

/* this only counts internal nodes */

static double duplication_ratio(network, clusters) network_t *network;
array_t *clusters;
{
  int i;
  cluster_t *cluster;
  int before_clustering;
  int after_clustering;

  before_clustering = network_num_internal(network);
  after_clustering = 0;
  for (i = 0; i < array_n(clusters); i++) {
    cluster = array_fetch(cluster_t *, clusters, i);
    after_clustering += st_count(cluster->nodes);
  }
  return (double)after_clustering / (double)before_clustering;
}

/* COLLAPSING CLUSTERS */

/*
 * collapsing the clusters.
 * to do the collapsing of each cluster,
 * have to be careful to collapse them in topological order
 * the closer to the PI's, the lower the index in 'nodevec'
 */

static void collapse_clusters(clusters, nodevec) array_t *clusters;
array_t *nodevec;
{
  int i;
  node_t *node;
  cluster_t *cluster;
  st_table *topological_order = st_init_table(st_numcmp, st_numhash);

  for (i = 0; i < array_n(nodevec); i++) {
    node = array_fetch(node_t *, nodevec, i);
    st_insert(topological_order, (char *)node, (char *)i);
  }
  for (i = 0; i < array_n(clusters); i++) {
    cluster = array_fetch(cluster_t *, clusters, i);
    collapse_cluster(cluster, topological_order);
  }
  st_free_table(topological_order);
}

typedef struct {
  int order;
  node_t *node;
} sorted_node_t;

/* should put first those closer to PO */

static int sorted_node_cmp(obj1, obj2) char *obj1;
char *obj2;
{
  sorted_node_t *c1 = (sorted_node_t *)obj1;
  sorted_node_t *c2 = (sorted_node_t *)obj2;

  return -(c1->order - c2->order);
}

/* extract all the nodes in a cluster into an array */
/* sort the array in such a way to put first the nodes closest to the root */
/* for consistency, check that the first node in the array is marked "root" */

/*
int
node_collapse(f, g)
node_t *f, *g;
        Assuming g is an input to f, re-express f without using g.
        Changes f in-place, g is unchanged.

*/

static void collapse_cluster(cluster, topological_order) cluster_t *cluster;
st_table *topological_order;
{
  int i;
  st_generator *gen;
  node_t *net_node;
  int order;
  sorted_node_t new_node;
  sorted_node_t *root;
  sorted_node_t *node;
  int n_nodes = st_count(cluster->nodes);
  array_t *nodes = array_alloc(sorted_node_t, n_nodes);

  assert(n_nodes > 0);
  st_foreach_item(cluster->nodes, gen, (char **)&net_node, NIL(char *)) {
    assert(st_lookup_int(topological_order, (char *)net_node, &order));
    new_node.node = net_node;
    new_node.order = order;
    array_insert_last(sorted_node_t, nodes, new_node);
  }
  array_sort(nodes, sorted_node_cmp);

  root = array_fetch_p(sorted_node_t, nodes, 0);
  assert(cluster->root == root->node);
  for (i = 1; i < n_nodes; i++) {
    node = array_fetch_p(sorted_node_t, nodes, i);
    node_collapse(root->node, node->node);
    /* just a check */
    if (node_get_fanin_index(root->node, node->node) != -1) {
      node_print(sisout, root->node);
      node_print(sisout, node->node);
      fail("incorrect collapsing: collapsed node still appears as fanin");
    }
  }

  array_free(nodes);
}

/* BINARY SEARCH ON LABELLING PROCEDURE */

/*
 * just an interface to allow binary search without concern for memory leaks
 * returns the max label value obtained by Lawler's algorithm for the given
 * cluster_size
 */

static int best_labelling(nodevec, cluster_size) array_t *nodevec;
int cluster_size;
{
  int max_label;
  st_table *table = clust_node_table_alloc();

  max_label = label_nodes(table, nodevec, cluster_size, 0);
  clust_node_table_free(table);
  return max_label;
}

/*
 * given a function fn guaranteed to be non increasing
 * find the smallest integer x between min and max such that fn(x) >= value
 * the objective is to minimize the number of calls to fn
 */

static int lazy_binary_search(min, max, value, nodevec, fn) int min;
int max;
int value;
array_t *nodevec;
int (*fn)();
{
  int middle;
  int min_value;
  int max_value;
  int middle_value;

  assert(min <= max);
  min_value = (*fn)(nodevec, min);
  if (min_value <= value)
    return min;
  max_value = (*fn)(nodevec, max);
  if (max_value > value)
    return max;
  for (;;) {
    middle = min + (max - min) / 2;
    if (middle == min) {
      assert(max_value <= value);
      return (min_value <= value) ? min : max;
    }
    middle_value = (*fn)(nodevec, middle);
    if (middle_value > value) {
      min = middle;
      min_value = middle_value;
    } else {
      max = middle;
      max_value = middle_value;
    }
  }
}

/* UTILITIES */

/*
 * alloc and free the hash table containing info on nodes: used for labelling
 */

static st_table *clust_node_table_alloc() {
  return st_init_table(st_numcmp, st_numhash);
}

static void clust_node_table_free(table) st_table *table;
{
  node_t *node;
  st_generator *gen;
  clust_node_t *node_info;

  st_foreach_item(table, gen, (char **)&node, (char **)&node_info) {
    clust_node_free(node_info);
  }
  st_free_table(table);
}

static void clust_node_free(node_info) clust_node_t *node_info;
{
  array_free(node_info->clusters);
  FREE(node_info);
}

static void reinit_cluster_info(table) st_table *table;
{
  node_t *node;
  st_generator *gen;
  clust_node_t *node_info;

  st_foreach_item(table, gen, (char **)&node, (char **)&node_info) {
    array_free(node_info->clusters);
    node_info->clusters = array_alloc(cluster_t *, 0);
  }
}

static void clusters_free(clusters) array_t *clusters;
{
  int i;
  cluster_t *cluster;

  for (i = 0; i < array_n(clusters); i++) {
    cluster = array_fetch(cluster_t *, clusters, i);
    cluster_free(cluster);
  }
  array_free(clusters);
}

static cluster_t *cluster_alloc() {
  cluster_t *result = ALLOC(cluster_t, 1);

  result->nodes = st_init_table(st_numcmp, st_numhash);
  result->root = NIL(node_t);
  result->label = -1;
  return result;
}

static void cluster_free(cluster) cluster_t *cluster;
{
  st_free_table(cluster->nodes);
  FREE(cluster);
}

/* FOR DEBUGGING */

static void print_clusters(clusters) array_t *clusters;
{
  int i;
  cluster_t *cluster;

  for (i = 0; i < array_n(clusters); i++) {
    cluster = array_fetch(cluster_t *, clusters, i);
    print_cluster(cluster);
  }
}

static void print_cluster(cluster) cluster_t *cluster;
{
  clust_node_type_t type;
  node_t *node;
  int num_nodes;
  st_generator *gen;
  char *dummy;

  (void)fprintf(misout, "cluster#%d ", cluster->number);
  num_nodes = st_count(cluster->nodes);
  (void)fprintf(misout, "(number of nodes = %d) =>", num_nodes);
  st_foreach_item(cluster->nodes, gen, (char **)&node, &dummy) {
    type = (clust_node_type_t)dummy;
    switch (type) {
    case NORMAL_NODE:
      (void)fprintf(misout, " %s ", node->name);
      break;
    case ROOT_NODE:
      (void)fprintf(misout, " %s:root ", node->name);
      break;
    default:
      (void)fprintf(misout, "\nunexpected clust_node type\n");
      break;
    }
  }
  (void)fprintf(misout, "\n\n");
}
