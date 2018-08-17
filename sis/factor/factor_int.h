
typedef enum trav_order_enum trav_order;
enum trav_order_enum {
    FACTOR_TRAV_IN_ORDER, FACTOR_TRAV_POST_ORDER
};

typedef enum factor_type_enum factor_type;
enum factor_type_enum {
    FACTOR_0, FACTOR_1, FACTOR_AND, FACTOR_OR,
    FACTOR_INV, FACTOR_LEAF, FACTOR_UNKNOWN
};


typedef struct ft_struct {
    factor_type      type;
    int              index;
    int              len;
    struct ft_struct *next_level;
    struct ft_struct *same_level;
}                             ft_t;


extern void factor_recur();

extern node_t *factor_best_literal();

extern node_t *factor_quick_kernel();

extern node_t *factor_best_kernel();

extern void factor_traverse();

extern void factor_nt_free();

extern void value_print();

extern ft_t *factor_nt_to_ft();

extern void ft_print();

extern void set_line_width();

extern void eliminate();

extern int value_cmp_inc();

extern int value_cmp_dec();
