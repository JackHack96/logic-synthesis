
#ifndef NODE_H
#define NODE_H

#include "list.h"
#include "espresso.h"

/* ... belongs in network.h, but node and network reference each other ... */
typedef struct network_struct network_t;
typedef struct node_struct    node_t;


typedef enum node_function_enum node_function_t;
enum node_function_enum {
    NODE_PI, NODE_PO, NODE_0, NODE_1, NODE_BUF, NODE_INV,
    NODE_AND, NODE_OR, NODE_COMPLEX, NODE_UNDEFINED
};


typedef enum name_mode_enum name_mode_t;
enum name_mode_enum {
    LONG_NAME_MODE, SHORT_NAME_MODE
};


typedef enum node_type_enum node_type_t;
enum node_type_enum {
    PRIMARY_INPUT, PRIMARY_OUTPUT, INTERNAL, UNASSIGNED
};


typedef enum node_daemon_type_enum {
    DAEMON_ALLOC = 0, DAEMON_FREE = 1, DAEMON_INVALID = 2, DAEMON_DUP = 3
}                           node_daemon_type_t;


typedef enum input_phase_enum input_phase_t;
enum input_phase_enum {
    POS_UNATE, NEG_UNATE, BINATE, PHASE_UNKNOWN
};


typedef struct fanout_struct fanout_t;
struct fanout_struct {
    node_t *fanout;
    int    pin;
};


typedef enum node_sim_enum node_sim_type_t;
enum node_sim_enum {
    NODE_SIM_SCC, NODE_SIM_SIMPCOMP, NODE_SIM_ESPRESSO,
    NODE_SIM_EXACT, NODE_SIM_EXACT_LITS, NODE_SIM_DCSIMP,
    NODE_SIM_NOCOMP, NODE_SIM_SNOCOMP
};


struct node_struct {
    char        *name;            /* name of the output signal */
    char        *short_name;        /* short name for interactive use */
    node_type_t type;        /* type of the node */

    int sis_id;            /* unique id (used to sort fanin) */

    unsigned fanin_changed:1;    /* flag to catch fanin generation errors */
    unsigned fanout_changed:1;    /* flag to catch fanout generation errors */
    unsigned is_dup_free:1;    /* node has no aliasing of its fanin */
    unsigned is_min_base:1;    /* node is minimum base */
    unsigned is_scc_minimal:1;    /* node is scc-minimal */

    int    nin;            /* number of inputs */
    node_t **fanin;

    lsList   fanout;        /* list of 'fanout_t' structures */
    lsHandle *fanin_fanout;    /* handles of our fanin's fanout_t structure */

    pset_family F;        /* on-set */
    pset_family D;        /* dc-set -- currently unused */
    pset_family R;        /* off-set */

    node_t *copy;        /* used by network_dup(), network_append() */

    network_t *network;        /* network this node belongs to */
    lsHandle  net_handle;    /* handle inside of network nodelist */

    char *simulation;        /* reserved for simulation package */
    char *factored;        /* reserved for factoring package */
    char *delay;        /* reserved for delay package */
    char *map;            /* reserved for mapping package */
    char *simplify;        /* reserved for simplify package */
    char *bdd;            /* reserved for bdd package */
    char *pld;            /* reserved for pld package */
    char *ite;            /* reserved for pld package */
    char *buf;            /* reserved for buffer package */
    char *cspf;            /* reserved for cspf (simplify) package */
    char *bin;            /* reserved for binning (mapping) package */
    char *atpg;            /* reserved for atpg package */
    char *undef1;        /* undefined 1 */
};

typedef pset node_cube_t;
typedef int  node_literal_t;

#define foreach_fanin(node, i, p)                    \
    for(i = 0, node->fanin_changed = 0;                    \
    node->fanin_changed ? node_error(4) :                \
        i < (node)->nin && (p = node->fanin[i]); i++)

#define foreach_fanout(node, gen, p)                    \
    for(node->fanout_changed = 0, gen = node_fanout_init_gen(node);    \
    node->fanout_changed ? (node_t *) node_error(5) :        \
        (p = node_fanout_gen(gen, NIL(int))); )

#define foreach_fanout_pin(node, gen, p, pin)                \
    for(node->fanout_changed = 0, gen = node_fanout_init_gen(node);    \
    node->fanout_changed ? (node_t *) node_error(5) :        \
        (p = node_fanout_gen(gen, &pin)); )

#define node_get_cube(f_, i_)                        \
    ((f_)->F ?                                \
    ((i_) >= 0 && (i_) < (f_)->F->count ? GETSET((f_)->F, (i_)) :    \
        (node_cube_t) node_error(0))                \
        : (node_cube_t) node_error(1))

#define node_get_literal(c_, j_)                    \
    ((c_) ? GETINPUT((c_), (j_)) : (node_literal_t) node_error(2))


extern name_mode_t name_mode;

extern node_t *node_alloc(void);

extern void node_free(node_t *);

extern node_t *node_dup(node_t *);

extern void node_register_daemon(enum node_daemon_type_enum, void (*)());

extern network_t *node_network(node_t *);

extern node_t *node_and(node_t *, node_t *);

extern node_t *node_or(node_t *, node_t *);

extern node_t *node_not(node_t *);

extern node_t *node_xor(node_t *, node_t *);

extern node_t *node_xnor(node_t *, node_t *);

extern node_t *node_div(node_t *, node_t *, node_t **);

extern node_t *node_literal(node_t *, int);

extern node_t *node_constant(int);

extern node_t *node_cofactor(node_t *, node_t *);

extern node_t *node_simplify(node_t *, node_t *, enum node_sim_enum);

extern node_t *node_largest_cube_divisor(node_t *);

extern void node_replace(node_t *, node_t *);

extern int node_contains(node_t *, node_t *);

extern int node_equal(node_t *, node_t *);

extern int node_equal_by_name(node_t *, node_t *);

extern char *node_name(node_t *);

extern char *node_long_name(node_t *);

extern int node_num_literal(node_t *);

extern int node_num_cube(node_t *);

extern int *node_literal_count(node_t *);

extern int node_num_fanin(node_t *);

extern int node_num_fanout(node_t *);

extern node_t *node_get_fanin(node_t *, int);

extern node_t *node_get_fanout(node_t *, int);

extern int node_get_fanin_index(node_t *, node_t *);

/* extern int node_get_fanout_index(); */

extern void node_scc(node_t *);

extern void node_d1merge(node_t *);

extern int node_substitute(node_t *, node_t *, int);

extern int node_collapse(node_t *, node_t *);

extern void node_algebraic_cofactor(node_t *, node_t *, node_t **, node_t **, node_t **);

extern int node_invert(node_t *);

extern node_function_t node_function(node_t *);

extern node_type_t node_type(node_t *);

extern input_phase_t node_input_phase(node_t *, node_t *);

extern void node_print(FILE *, node_t *);

extern void node_print_negative(FILE *, node_t *);

extern void node_print_rhs(FILE *, node_t *);

extern void node_lib_process(network_t *);

extern void node_minimum_base(node_t *);

extern void node_replace_internal(node_t *, node_t **, int, struct set_family *);

extern int node_patch_fanin(node_t *, node_t *, node_t *);

extern int node_patch_fanin_index(node_t *, int, node_t *);

extern int node_compare_id(char **, char **);

extern pset_family node_sf_adjust(node_t *, node_t **, int);

extern node_t **nodevec_dup(node_t **, int);

extern node_t *node_create(struct set_family *, node_t **, int);

extern void cautious_define_cube_size(int);

extern void define_cube_size(int);

extern void undefine_cube_size(void);

extern void node_assign_name(node_t *);

extern void node_assign_short_name(node_t *);

extern int node_is_madeup_name(char *, int *);

extern void fanin_remove_fanout(node_t *);

extern void fanin_add_fanout(node_t *);

/* exported for use in macros	*/
extern int node_error(int);

extern node_t *node_fanout_gen(lsList, int *);

extern lsGen node_fanout_init_gen(node_t *);

#endif