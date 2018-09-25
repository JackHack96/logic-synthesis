
/* file @(#)map.h	1.4 */
/* last modified on 7/25/91 at 11:41:14 */
#ifndef MAP_H
#define MAP_H

#include "network.h"
#include "library.h"

typedef void (*VoidFn)();

extern network_t *map_network(network_t *, library_t *, double, int, int);

extern void map_add_inverter(network_t *, int);

extern void map_remove_inverter(network_t *, void (*)());

extern void map_network_dup(network_t *);

int init_map();

int end_map();

#endif /* MAP_H */
