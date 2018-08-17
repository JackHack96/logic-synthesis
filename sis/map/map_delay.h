
/* file @(#)map_delay.h	1.3 */
/* last modified on 7/1/91 at 22:42:29 */
#ifndef MAP_DELAY_H
#define MAP_DELAY_H

extern void delay_map_compute_required_times();

extern void delay_gate_arrival_times();    /* similar to delay_gate_simulate but faster for several loads */
extern void delay_compute_pin_delays();

extern void map_alloc_delay_info(/* network, bin_global_t *options */);

extern void map_free_delay_info(/* */);

extern delay_time_t pipo_get_pi_arrival(/* node_t *node */);

extern delay_time_t pipo_get_pi_drive(/* node_t *node */);

extern delay_time_t pipo_get_po_required(/* node_t *node */);

extern double pipo_get_po_load(/* node_t *node */);

extern double pipo_get_pi_load_limit(/* node_t *node */);

extern void pipo_set_default_po_required(/* delay_time_t value */);

extern double map_compute_wire_load(/* int n_fanouts */);

extern double map_compute_fanout_load_correction(/* int n_fanouts, bin_global_t *options */);

/* should be put in delay/delay.h */
extern pin_phase_t delay_get_polarity(/* char *pin_delay */);

#endif 
