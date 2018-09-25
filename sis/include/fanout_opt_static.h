#ifndef FANOUT_OPT_STATIC_H
#define FANOUT_OPT_STATIC_H
/* file @(#)fanout_opt_static.h	1.4 */
/* last modified on 7/1/91 at 22:41:14 */

typedef struct fanout_alg_descr {
    int  alg_id;        /* ID for identifying properties. */
    char *name;        /* Name of algorithm.		*/
    void (*init_fn)(); /* Initialization function.	*/
    int on_flag;       /* Some sort of flag.		*/
} fanout_alg_descr;

typedef struct fanout_prop_descr {
    int  alg_id;          /* ID of algorithm.		*/
    char *name;          /* name of property.		*/
    int  value;           /* Value of property.		*/
    void (*update_fn)(); /* Function to update property.	*/
} fanout_prop_descr;

static fanout_alg_descr fanout_algorithms[] = {

        {1, "noalg",          noalg_init,          1},
        {2, "lt_trees",       lt_trees_init,       1},
        {3, "two_level",      two_level_init,      1},
        {4, "fanout_dump",    fanout_dump_init,    0},
        {5, "mixed_lt_trees", mixed_lt_trees_init, 1},
        {6, "bottom_up",      bottom_up_init,      1},
        {7, "balanced",       balanced_init,       1},
        {8, "top_down",       top_down_init,       0},
        {0}};

static fanout_prop_descr fanout_properties[] = {

        {1, "min_size",       1,  fanout_opt_update_size_threshold},
        {1, "peephole",       1,  fanout_opt_update_peephole_flag},

        {2, "min_size",       1,  fanout_opt_update_size_threshold},
        {2, "peephole",       1,  fanout_opt_update_peephole_flag},
        {2, "max_gaps",       5,  lt_trees_set_max_n_gaps},

        {3, "min_size",       2,  fanout_opt_update_size_threshold},
        {3, "peephole",       1,  fanout_opt_update_peephole_flag},

        {4, "min_size",       2,  fanout_opt_update_size_threshold},
        {4, "peephole",       0,  fanout_opt_update_peephole_flag},
        {4, "dump_threshold", 20, fanout_dump_set_dump_threshold},

        {5, "min_size",       1,  fanout_opt_update_size_threshold},
        {5, "peephole",       1,  fanout_opt_update_peephole_flag},
        {5, "max_gaps",       3,  mixed_lt_trees_set_max_n_gaps},
        {5, "max_x",          10, mixed_lt_trees_set_max_x_index},
        {5, "max_y",          10, mixed_lt_trees_set_max_y_index},

        {6, "min_size",       2,  fanout_opt_update_size_threshold},
        {6, "peephole",       1,  fanout_opt_update_peephole_flag},

        {7, "min_size",       2,  fanout_opt_update_size_threshold},
        {7, "peephole",       1,  fanout_opt_update_peephole_flag},

        {8, "min_size",       2,  fanout_opt_update_size_threshold},
        {8, "peephole",       1,  fanout_opt_update_peephole_flag},
        {8, "mode",           6,  top_down_set_mode},
        {8, "debug",          0,  top_down_set_debug},
        {0}};

static delay_time_t get_reloaded_node_arrival_time();

static delay_time_t get_resized_node_arrival_time();

static delay_time_t recompute_map_arrival();

static fanout_cost_t fanout_opt_add_cost();

static int compare_gate_links();

static int extract_fanout_problem();

static int fanout_map_optimal();

static node_t *keep_external_source_only();

static st_table *extract_links();

static void compute_required_times();

static void do_fanout_opt();

static void do_global_area_recover();

static void fanout_opt_end();

static void fanout_opt_init();

static void fanout_single_source_optimal();

static void free_gate_info();

static void init_gate_info();

static void inputs_first_rec();

static void outputs_first_rec();

static void recompute_arrival_times();

static void replace_node_gate();

static void resize_node();

static void virtual_net_for_all_nodes_inputs_first();

static void virtual_net_for_all_nodes_outputs_first();

#endif