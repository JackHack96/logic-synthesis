#ifndef NODE_INT_H
#define NODE_INT_H

#include "node.h"

/*
 *  daemon's are in a linked list by daemon type
 */

typedef struct daemon_struct daemon_t;

struct daemon_struct {
    void (*func)();

    daemon_t *next;
};

#define node_has_function(f) (f->F != 0)

void network_rehash_names();

void node_complement();

void node_invalid();

void make_common_base_by_name();

void make_common_base();

void node_discard_all_daemons();

void set_espresso_flags();

node_t *node_sort_for_printing();

int fancy_lex_order();

void node_remove_dup_fanin();

#endif