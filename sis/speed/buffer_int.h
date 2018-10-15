#ifndef BUFFER_INT_H
#define BUFFER_INT_H

#include "library.h"
#include "delay.h"
#include "list.h"
#include "st.h"

/*
 * Data Structures internal to buffering package
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 */
typedef struct buffer_struct {
  int depth;
  lib_gate_t **gate;
  double area;
  double ip_load;
  double max_load;
  pin_phase_t phase;
  delay_time_t block;
  delay_time_t drive;
} sp_buffer_t;

typedef struct sp_node_struct {
  int type;
  union {
    lib_gate_t *gate;
    sp_buffer_t *buffer;
  } impl;                /* The implementation at this node */
  int cfi;               /* The index of the most crit fanin */
  double load;           /* The load that this buffer is driving */
  delay_time_t req_time; /* The req time we can achieve */
  delay_time_t prev_dr;  /* Drive of the preceeding gate */
  pin_phase_t prev_ph;   /* Phase of the preceeding gate */
} sp_node_t;

typedef struct global_buffer_param_struct buf_global_t;
struct global_buffer_param_struct {
  /* Command line options */
  int trace;               /* prints the changes in slack as algo proceeds */
  double thresh;           /* Determins the critical paths */
  double crit_slack;       /* Sets the absolute value for crit slack */
  int single_pass;         /* Buffer only one node in the pass */
  int do_decomp;           /* Decomposition of the root gate */
  int debug;               /* Print debugging info */
  int limit;               /* Only fanouts > limit are buffered */
  int mode;                /* What transformations to allow */
  int only_check_max_load; /* */
  int interactive;         /* == 1 when called from command line */
  /* Options specific to to the problem and library */
  sp_buffer_t **buf_array; /* Structure for the buffers available */
  lsList buffer_list;      /* List of the buffers available */
  int num_buf;             /* Number of non_inverting buffers */
  int num_inv;             /* Number of inverters */
  double auto_route;       /* The auto route factor */
  delay_model_t model;     /* The delay model -- usually MAPPED */
  st_table *glbuftable;    /* To store nodes buffered in a pass */
  double min_req_diff;     /* The min diff to warrant an unbalanced decomp */
};

/*
 * Definitions of constants
 * ^^^^^^^^^^^^^^^^^^^^^^^^^
 */
#define BUFFER_SLOT buf
#define BUFFER(node) ((sp_node_t *)(node->BUFFER_SLOT))

#define NONE 0
#define BUF 1
#define GATE 2

#define REPOWER_MASK ((int)(1 << 0))
#define UNBALANCED_MASK ((int)(1 << 1))
#define BALANCED_MASK ((int)(1 << 2))

#define V_SMALL 0.000001 /* 1.0e-6 */

/*
 * Some macros
 */
#define BUF_MAX_D(a) (MAX(a.rise, a.fall))
#define BUF_MIN_D(a) (MIN(a.rise, a.fall))
#define REQ_EQUAL(t1, t2)                                                      \
  ((ABS(((t1).rise) - ((t2).rise)) < V_SMALL) &&                               \
   (ABS(((t1).fall) - ((t2).fall)) < V_SMALL))

#define REQ_IMPR(t1, t2)                                                       \
  (((((t1).rise - (t2).rise)) > V_SMALL) &&                                    \
   ((((t1).fall - (t2).fall)) > V_SMALL))

#define MIN_DELAY(result, t1, t2)                                              \
  ((result).rise = MIN((t1).rise, (t2).rise),                                  \
   (result).fall = MIN((t1).fall, (t2).fall))
#define sp_drive_adjustment(dr, load, req)                                     \
  ((req)->rise -= (dr).rise * load, (req)->fall -= (dr).fall * load)

#define buf_num_fanout(n) ((n) != NIL(node_t) ? node_num_fanout(n) : 0)

/*
 * Functions internal to the buffering package
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 */
char *sp_buffer_name();

char *sp_name_of_impl();

void speed_dump_buffer_list();

void speed_free_buffer_list();

void sp_buf_array_from_list();

int buf_critical();

int sp_buffer_recur();

int speed_buffer_node();

int sp_satisfy_max_load();

int sp_max_load_violation();

int buf_compare_fanout();

int buf_evaluate_trans2();

int speed_buffer_network();

int buf_failed_slack_test();

int sp_replace_cell_strength();

int speed_buffer_array_of_nodes();

void buffer_free();

void buffer_alloc();

void set_buf_thresh();

void buf_dump_fanout_data();

void buf_set_buffers();

void buf_free_buffers();

int sp_buf_get_crit_fanin();

void sp_set_inverter();

void sp_replace_lib_gate();

void sp_buf_annotate_gate();

void sp_implement_buffer_chain();

lib_gate_t *buf_get_gate_version();

/*
 * DELAY STUFF for the buffering algorithm --- need a better way out
 */

void sp_compute();

void sp_buf_req_time();

void sp_subtract_delay();

void sp_buf_compute_req_time();

void buffer_init_globals();

double sp_get_pin_load();

delay_time_t sp_get_input_drive();

void buffer_delay_get_pi_drive();

#endif //BUFFER_INT_H