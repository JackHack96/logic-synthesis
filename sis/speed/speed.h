/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/speed/speed.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:50 $
 *
 */
#ifndef SPEED_H
#define SPEED_H

EXTERN void	  speed_node_interface ARGS((network_t *, node_t *, double, delay_model_t));
EXTERN void	  speed_loop_interface ARGS((network_t **, double, double, int, delay_model_t, int));
EXTERN array_t    *speed_decomp_interface ARGS((node_t *, double, delay_model_t));

EXTERN int buffer_network ARGS((network_t *, int, int, double, int, int));

/*
 * some of the low level buffering stuff ...
 * The data structures are exported for the mapper to interface with buffering
 */

typedef struct sp_fanout_struct{
    int pin;		       /* The "fanin_index" of the destination pin */
    double load;	       /* The input_cap of the destination */
    node_t *fanout;	       /* The node to which destination belongs to */
    delay_time_t req;	       /* Req time of the destination */
    pin_phase_t phase;	       /* Whether POS or NEG poloarity signal */
    } sp_fanout_t;

typedef struct buffer_alg_input_struct buf_alg_input_t;
struct buffer_alg_input_struct {
    node_t *root;	       /* root node of the buffering problem */
    node_t *node;	       /* node at which buffering is being performed*/
    node_t *inv_node;	       /* node driving inv destinations */
    sp_fanout_t *fanouts;      /* Description of the fanout destinations */
    int num_pos;	       /* Number of positive fanouts */
    int num_neg;	       /* Number of negative destination */
    double max_ip_load;	       /* The max load at "cfi" input of root */
    };


/* Following routines are for the map package */
EXTERN void buf_init_top_down ARGS((network_t *, int, int));
EXTERN void buf_map_interface ARGS((network_t *, buf_alg_input_t *));
EXTERN void buf_add_implementation ARGS((node_t *, lib_gate_t *));
EXTERN void buf_set_prev_drive ARGS((node_t *, delay_time_t));
EXTERN void buf_set_prev_phase ARGS((node_t *, pin_phase_t));
EXTERN void buf_set_required_time_at_input ARGS((node_t *, delay_time_t));
EXTERN double buf_get_auto_route ARGS((void));
EXTERN delay_model_t buf_get_model ARGS((void));
EXTERN delay_time_t buf_get_required_time_at_input ARGS((node_t *));

#endif






