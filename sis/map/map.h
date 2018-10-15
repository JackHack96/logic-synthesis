/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/map.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:25 $
 *
 */
/* file @(#)map.h	1.4 */
/* last modified on 7/25/91 at 11:41:14 */
#ifndef MAP_H
#define MAP_H
typedef void (*VoidFn)();

EXTERN network_t *map_network ARGS((network_t *, library_t *, double, int, int));
EXTERN void map_add_inverter ARGS((network_t *, int));
EXTERN void map_remove_inverter ARGS((network_t *, void (*)()));
EXTERN void map_network_dup ARGS((network_t *));
#endif /* MAP_H */
