
/* file @(#)fanout_delay_static.h	1.2 */
/* last modified on 5/1/91 at 15:50:32 */
static delay_pwl_t delay_get_delay_pwl_buffer();

static delay_pwl_t delay_get_delay_pwl_internal_source();

static delay_pwl_t delay_get_delay_pwl_pi();

static delay_pwl_t delay_get_delay_pwl_pwl_source();

static delay_time_t delay_forward_intrinsic_neg_buffer();

static delay_time_t delay_forward_intrinsic_pos_buffer();

static delay_time_t delay_forward_load_dependent_buffer();

static delay_time_t delay_forward_load_dependent_internal_source();

static delay_time_t delay_forward_load_dependent_pi_source();

static delay_time_t delay_forward_load_dependent_pwl_source();

static delay_time_t delay_intrinsic_fatal_error();

static delay_time_t delay_intrinsic_neg_buffer();

static delay_time_t delay_intrinsic_pos_buffer();

static delay_time_t delay_load_dependent_buffer();

static delay_time_t delay_load_dependent_internal_source();

static delay_time_t delay_load_dependent_pi_source();

static delay_time_t delay_load_dependent_pwl_source();

static int linear_search_best_number_of_inverters();

static void add_buffers();

static void delay_gate_free();

static void delay_gate_initialize();

static void fanout_delay_add_general_source();
