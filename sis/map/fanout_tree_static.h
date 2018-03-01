/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/fanout_tree_static.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:25 $
 *
 */
/* file @(#)fanout_tree_static.h	1.2 */
/* last modified on 5/1/91 at 15:50:58 */
static array_t *fanout_tree_get_roots();
static array_t *get_forest_in_bottom_up_order();
static delay_time_t check_required_rec();
static delay_time_t recompute_arrival_time();
static double fanout_tree_get_node_load();
static fanout_node_t *get_first_buffer_child();
static int add_edges_rec();
static int already_flat_subtree();
static int can_flatten_subtree();
static int can_merge_node();
static int can_merge_subtrees();
static int continue_flattening();
static int incr_critical_child();
static int peephole_is_better();
static int peephole_optimize();
static node_t *create_buffer();
static void add_children_cost();
static void adjust_node_children();
static void alloc_peephole_rec();
static void build_peephole_tree_rec();
static void build_peephole_trees();
static void check_area();
static void check_polarities();
static void check_polarity_rec();
static void check_required();
static void cleanup_remove_opt_children_entries();
static void cleanup_remove_opt_node();
static void compute_arrival_times_rec();
static void compute_best_new_cost();
static void compute_buffer_node_cost();
static void compute_current_cost();
static void compute_flatten_info();
static void compute_no_opt_cost();
static void do_build_tree();
static void do_build_tree_rec();
static void extract_fanout_leaves_rec();
static void fanout_free_peephole();
static void fanout_tree_alloc_peephole();
static void fanout_tree_compute_arrival_times();
static void fanout_tree_remove_unnecessary_buffers();
static void free_unused_sources();
static void get_forest_in_bottom_up_order_rec();
static void merge_nodes();
static void peephole_init_buffer();
static void peephole_init_sink();
static void peephole_init_source();
static void peephole_optimize_node();
static void print_tabs();
static void print_tree_rec();
static void remove_buffer_process_buffer();
static void remove_buffer_process_sink();
static void remove_buffer_process_source();
static void remove_unnecessary_buffers_rec();
static void select_best_log_entry();
static void update_children_entries();
