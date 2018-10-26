
/*
 *  definitions local to 'gbx' go here
 */
/* A Useful utility */
#define SAFE_ALLOC(var, type, num) \
    assert((var = (type *)ALLOC(type, num)) != NIL(type))

#define max(x,y) (x < y?y:x)
#define min(x,y) (x > y?y:x)

/* find all bypasses in a network */

extern int
com_gbx_print_bypasses();

/*
 * Find bypasses and take them
 */
extern int
com_gbx_bypass();
/*
 * Find the bypasses and take them ALL
 */
extern int
com_gbx_all_bypasses();


extern st_table *node_weight_table;
extern delay_model_t gbx_delay_model;
typedef enum gbx_trace_enum gbx_trace_t;
enum gbx_trace_enum {
    GBX_OLD_TRACE, GBX_NEW_TRACE, GBX_NEWER_TRACE
};

typedef struct bypass_struct bypass_t;
struct bypass_struct {
    node_t *first_node, *last_node;
    double gain;
    double slack;
    double side_delay;
    double control_delay;
    int weight;
    node_t *dupe_at;
    lsList  bypassed_nodes;
    input_phase_t phase;
    double side_slack;
};

typedef struct node_bp_struct node_bp_t;
struct node_bp_struct {
    double *pin_slacks, *pin_weights;   /* slacks, weights on fanin edges */
    input_phase_t *input_phases;        /* phases of inputs */
    double slack;                       /* slack on gate */
    node_t *path_fanin;                 /* fanin of minimum slack */
    double path_slack;                  /* extra slack on most critical S.I */
    char mark;                          /* Marked if there's a bypass through this */
};
extern network_t *gbx_dup_network;
extern st_table *bypass_table;
extern st_table *gbx_node_table;
extern lsList  bypasses;
extern int take_bypass();
extern bypass_t *new_bypass();
extern void free_bypass();
extern void bypass_add_node();
extern void register_bypass();
extern node_bp_t *new_node_bp_record();
extern void gbx_init_node_table();
extern void gbx_clean_node_table();
extern node_t *path_fanout();
extern double retrieve_slack();
extern double weight();
extern lsList find_bypass_nodes();
extern void print_bypass();
extern void trace_bypass();
extern lsList new_find_bypass_nodes();
extern lsList newer_find_bypass_nodes();
extern int gbx_verbose_mode;
extern int start_node_mode;
extern int print_bypass_mode;

