/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/node/nodeindex.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:47 $
 *
 */
#ifndef NODEINDEX_H
#define NODEINDEX_H


/*
 *  a quick way to associate cubes (i.e., sm_row *) to small integers
 *  and back.  Used to build the kernel_cube matrix
 */

typedef struct nodeindex_struct nodeindex_t;
struct nodeindex_struct {
    st_table *node_to_int;
    array_t *int_to_node;
};


EXTERN struct nodeindex_struct *nodeindex_alloc ARGS((void));
EXTERN void nodeindex_free ARGS((struct nodeindex_struct *));
EXTERN int nodeindex_insert ARGS((struct nodeindex_struct *, node_t *));
EXTERN int nodeindex_indexof ARGS((struct nodeindex_struct *, node_t *));
EXTERN node_t *nodeindex_nodeof ARGS((struct nodeindex_struct *, int));

#endif
