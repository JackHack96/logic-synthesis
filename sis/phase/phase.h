
#ifndef PHASE_H
#define PHASE_H

extern void add_inv_network(network_t *);

extern int add_inv_node(network_t *, node_t *);

extern void phase_quick(network_t *);

extern void phase_good(network_t *);

extern void phase_random_greedy(network_t *, int);

extern void phase_trace_set(void);

extern void phase_trace_unset(void);

#endif
