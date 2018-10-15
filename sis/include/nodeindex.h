
#ifndef NODEINDEX_H
#define NODEINDEX_H

#include "array.h"
#include "st.h"
#include "node.h"

/*
 *  a quick way to associate cubes (i.e., sm_row *) to small integers
 *  and back.  Used to build the kernel_cube matrix
 */

typedef struct nodeindex_struct nodeindex_t;
struct nodeindex_struct {
    st_table *node_to_int;
    array_t  *int_to_node;
};

struct nodeindex_struct *nodeindex_alloc(void);

void nodeindex_free(struct nodeindex_struct *);

int nodeindex_insert(struct nodeindex_struct *, node_t *);

int nodeindex_indexof(struct nodeindex_struct *, node_t *);

node_t *nodeindex_nodeof(struct nodeindex_struct *, int);

#endif
