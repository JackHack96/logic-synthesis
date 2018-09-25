
#ifndef SPEED_H
#define SPEED_H

#include "network.h"
#include "delay.h"
#include "library.h"

extern void speed_node_interface(network_t *, node_t *, double, delay_model_t);

extern void speed_loop_interface(network_t **, double, double, int,
                                 delay_model_t, int);

extern array_t *speed_decomp_interface(node_t *, double, delay_model_t);

extern int buffer_network(network_t *, int, int, double, int, int);

/*
 * some of the low level buffering stuff ...
 * The data structures are exported for the mapper to interface with buffering
 */

typedef struct sp_fanout_struct {
    int          pin;           /* The "fanin_index" of the destination pin */
    double       load;       /* The input_cap of the destination */
    node_t       *fanout;    /* The node to which destination belongs to */
    delay_time_t req;  /* Req time of the destination */
    pin_phase_t  phase; /* Whether POS or NEG poloarity signal */
} sp_fanout_t;

typedef struct buffer_alg_input_struct buf_alg_input_t;
struct buffer_alg_input_struct {
    node_t      *root;         /* root node of the buffering problem */
    node_t      *node;         /* node at which buffering is being performed*/
    node_t      *inv_node;     /* node driving inv destinations */
    sp_fanout_t *fanouts; /* Description of the fanout destinations */
    int         num_pos;          /* Number of positive fanouts */
    int         num_neg;          /* Number of negative destination */
    double      max_ip_load;   /* The max load at "cfi" input of root */
};

/* Following routines are for the map package */
extern void buf_init_top_down(network_t *, int, int);

extern void buf_map_interface(network_t *, buf_alg_input_t *);

extern void buf_add_implementation(node_t *, lib_gate_t *);

extern void buf_set_prev_drive(node_t *, delay_time_t);

extern void buf_set_prev_phase(node_t *, pin_phase_t);

extern void buf_set_required_time_at_input(node_t *, delay_time_t);

extern double buf_get_auto_route(void);

extern delay_model_t buf_get_model(void);

extern delay_time_t buf_get_required_time_at_input(node_t *);

int init_speed();

int end_speed();

#endif
