/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/gate_link.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:25 $
 *
 */
/* file @(#)gate_link.h	1.3 */
/* last modified on 6/27/91 at 15:13:33 */
#ifndef GATE_LINK_H
#define GATE_LINK_H

typedef struct gate_link_struct gate_link_t;
struct gate_link_struct {
  node_t *node;			/* root of the gate */
  int pin;			/* pin number */
  double load;			/* load on this pin */
  double slack;			/* used by bin_delay.c for approx. slack computations; should disappear */
  delay_time_t required;	/* required time for this signal at this input pin */
};

extern void gate_link_init(/* node_t *node */);
extern double gate_link_compute_load(/* node_t *node */);
extern delay_time_t gate_link_compute_min_required(/* node_t *node */);
extern int gate_link_n_elts(/* node_t *node */);
extern int gate_link_is_empty(/* node_t *node */);
extern int gate_link_first(/* node_t *node, gate_link_t *link */);
extern int gate_link_next(/* node_t *node, gate_link_t *link */);
extern void gate_link_put(/* node_t *node, gate_link_t *link */);
extern int gate_link_get(/* node_t *node, gate_link_t *link */);
extern void gate_link_remove(/* node_t *node, gate_link_t *link */);
extern void gate_link_delete_all(/* node_t *node */);
extern void gate_link_free(/* node_t *node */);
extern void fanout_print_node(/* node_t *node */);

#endif
