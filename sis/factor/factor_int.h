#ifndef FACTOR_INT_H
#define FACTOR_INT_H

#include "node.h"

typedef enum trav_order_enum trav_order;
enum trav_order_enum {
    FACTOR_TRAV_IN_ORDER, FACTOR_TRAV_POST_ORDER
};

typedef enum factor_type_enum factor_type;
enum factor_type_enum {
    FACTOR_0,
    FACTOR_1,
    FACTOR_AND,
    FACTOR_OR,
    FACTOR_INV,
    FACTOR_LEAF,
    FACTOR_UNKNOWN
};

typedef struct ft_struct {
    factor_type      type;
    int              index;
    int              len;
    struct ft_struct *next_level;
    struct ft_struct *same_level;
}                             ft_t;

void factor_recur();

node_t *factor_best_literal();

node_t *factor_quick_kernel();

node_t *factor_best_kernel();

void factor_traverse();

void factor_nt_free();

void value_print();

ft_t *factor_nt_to_ft();

void ft_print();

void set_line_width();

void eliminate();

int value_cmp_inc();

int value_cmp_dec();

#endif