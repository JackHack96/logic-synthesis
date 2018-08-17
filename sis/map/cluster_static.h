
/* file @(#)cluster_static.h	1.5 */
/* last modified on 7/22/91 at 12:36:23 */
static clust_node_t *clust_node_alloc();

static cluster_t *cluster_alloc();

static void cluster_free();

static double duplication_ratio();

static int best_labelling();

static int compute_tfi_weight_for_given_label();

static int decide_label_node();

static int get_max_fanin_label();

static int label_nodes();

static int lazy_binary_search();

static int sorted_node_cmp();

static network_t *cluster_under_best_ratio();

static network_t *cluster_under_depth_constraint();

static network_t *cluster_under_size_as_depth_constraint();

static network_t *cluster_under_size_constraint();

static st_table *clust_node_table_alloc();

static void clust_node_free();

static void clust_node_table_free();

static void clusters_free();

static void collapse_cluster();

static void collapse_clusters();

static void compute_tfi_for_given_label_rec();

static void form_clusters();

static void gather_cluster_statistics();

static void print_cluster();

static void print_clusters();

static void put_tfi_in_cluster_rec();

static void reinit_cluster_info();

static void relabel_nodes();
