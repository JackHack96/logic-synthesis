typedef struct node_phase_struct node_phase_t;
struct node_phase_struct {
  sm_row *row;
};

typedef struct net_phase_struct net_phase_t;
struct net_phase_struct {
  sm_matrix *matrix;
  array_t *rows;
  double cost;
};

typedef struct row_data_struct row_data_t;
struct row_data_struct {
  int pos_used;     /* number of times the positive phase is used */
  int neg_used;     /* number of times the positive phase is used */
  int inv_save;     /* number of inverters saved when inverting the node */
  bool marked;      /* stamp used by good-phase */
  bool invertible;  /* whether the node is invertible */
  bool inverted;    /* whether the node is inverted */
  bool po;          /* whether the node fans out to a PO */
  node_t *node;     /* points to the node associate with the row. */
  double area;      /* area of the gate */
  double dual_area; /* area of the dual gate */
};

typedef struct element_data_struct element_data_t;
struct element_data_struct {
  int phase; /* 0 - pos. unate, 1 - neg_ungate, 2 - binate */
};

extern net_phase_t *phase_setup();

extern int invert_saving();

extern void phase_node_invert();

extern node_phase_t *phase_get_best();

extern double network_cost();

extern void phase_unmark_all();

extern void phase_mark();

extern net_phase_t *phase_dup();

extern void phase_replace();

extern void phase_free();

extern void phase_record();

extern void phase_check_unset();

extern void phase_check_set();

extern void phase_random_assign();

extern void phase_invert();

extern double phase_value();

extern double cost_comp();

extern void phase_invertible_set();

extern bool phase_trace;
extern bool phase_check;
