
/* file @(#)bin_delay_static.h	1.3 */
/* last modified on 5/2/91 at 17:24:39 */
static delay_bucket_t *compute_constant_gate_bucket();

static delay_bucket_t *compute_wire_bucket();

static delay_bucket_t *delay_bucket_alloc();

static delay_time_t get_arrival_from_pin_info();

static double get_area_from_pin_info();

static int find_best_delay();

static node_t *get_input_from_pin_info();

static pwl_t *gen_constant_pwl();

static pwl_t *gen_infinitely_slow_pwl();

static pwl_t *get_bucket_pwl_max();

static void bin_alloc();

static void bin_free();

static void bin_init_pi();

static int compute_best_match();

static void copy_bucket_to_map();

static void delay_bucket_print();

static void free_delay_bucket_storage();

static void increment_input_loads();

static void init_delay_bucket_storage();

static void preserve_best_area();

static void print_po_estimated_arrival_times();

static void select_best_gate();
