
/* file @(#)fanout_est_static.h	1.2 */
/* last modified on 5/1/91 at 15:50:34 */
static double fanout_est_select_non_inverter();
static int fanout_est_get_fanout_index();
static int fanout_est_get_source_polarity();
static int get_fanin_fanout_indices();
static node_t *fanout_est_get_pin_fanout_node();
static void fanout_bucket_free();
static void fanout_est_compute_dummy_fanout_info();
static void fanout_est_create_dummy_fanout_tree();
static void fanout_est_extend_fanout_leaf();
static void fanout_est_extract_remaining_fanout_leaves();
static void fanout_est_get_est_fanout_leaves();
