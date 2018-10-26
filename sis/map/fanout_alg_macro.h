#ifndef FANOUT_ALG_MACRO_H
#define FANOUT_ALG_MACRO_H
/* file @(#)fanout_alg_macro.h	1.3 */
/* last modified on 6/28/91 at 17:21:06 */

/* Functions to initialize fanout optimization algorithms. */

void noalg_init();

void lt_trees_init();

void two_level_init();

void fanout_dump_init();

void mixed_lt_trees_init();

void bottom_up_init();

void balanced_init();

void top_down_init();

/* Functions to update fanout optimization properties. */

void fanout_opt_update_size_threshold();

void fanout_opt_update_peephole_flag();

void lt_trees_set_max_n_gaps();

void fanout_dump_set_dump_threshold();

void mixed_lt_trees_set_max_n_gaps();

void mixed_lt_trees_set_max_x_index();

void mixed_lt_trees_set_max_y_index();

void top_down_set_mode();

void top_down_set_debug();

#endif