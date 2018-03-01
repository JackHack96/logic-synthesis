/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/latch/latch.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:29 $
 *
 */
typedef enum latch_synch_enum latch_synch_t;
enum latch_synch_enum {
    ACTIVE_HIGH, ACTIVE_LOW, RISING_EDGE, FALLING_EDGE, COMBINATIONAL,
    ASYNCH, UNKNOWN
};

typedef struct latch_struct latch_t;
struct latch_struct {
    node_t *latch_input;	/* must be a PRIMARY_OUTPUT */
    node_t *latch_output;	/* must be a PRIMARY_INPUT */
    int initial_value;		/* initial or reset state */
    int current_value;		/* current state */
    latch_synch_t synch_type;	/* type of latch */
    struct lib_gate_struct *gate;		/* Reference to the library implementation */
    node_t *control;		/* Pointer to the controlling gate */
    char *undef1;		/* undefined 1, for the programer's use */
};

EXTERN latch_t *latch_alloc ARGS((void));
EXTERN void latch_free ARGS((latch_t *));

#define latch_set_input(l, n) \
    (void) ((l)->latch_input = n)

#define latch_get_input(l) \
    (l)->latch_input

#define latch_set_output(l, n) \
    (void) ((l)->latch_output = n)

#define latch_get_output(l) \
    (l)->latch_output

#define latch_set_initial_value(l, i) \
    (void) ((l)->initial_value = i)

#define latch_get_initial_value(l) \
    (l)->initial_value

#define latch_set_current_value(l, i) \
    (void) ((l)->current_value = i)

#define latch_get_current_value(l) \
    (l)->current_value

#define latch_set_type(l, t) \
    (void) ((l)->synch_type = t)

#define latch_get_type(l) \
    (l)->synch_type

#define latch_set_gate(l, g) \
    (void) ((l)->gate = g)

#define latch_get_gate(l) \
    (l)->gate

EXTERN void latch_set_control ARGS((latch_t *, node_t *));

#define latch_get_control(l) \
    (l)->control

EXTERN latch_t *latch_from_node ARGS((node_t *));

EXTERN void network_create_latch ARGS((network_t *, latch_t **,node_t *,node_t *));
EXTERN void network_delete_latch ARGS((network_t *,latch_t *));
EXTERN void network_delete_latch_gen ARGS((network_t *,lsGen));
EXTERN int latch_equal ARGS((latch_t *, latch_t *));
