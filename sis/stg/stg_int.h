#ifndef STG_INT_H
#define STG_INT_H

#define MAX_ELENGTH 36

/* We are keeping the definitions of the clocks local to the stg package */
typedef struct stg_clock_structure {
    char   *name;
    double cycle_time;   /* Cycle time */
    double nominal_rise; /* Nominal position */
    double nominal_fall;
    double min_rise; /* max negative skew */
    double min_fall;
    double max_rise; /* max positive skew */
    double max_fall;
} stg_clock_t;

#define stg_get_clock_data(stg)                                                \
  (stg_clock_t *)g_get_g_slot_static((stg), CLOCK_DATA)
#define stg_set_clock_data(stg, i)                                             \
  (void)g_set_g_slot_static((stg), CLOCK_DATA, (gGeneric)i)

typedef struct node_data {
    node_t           *node;
    struct node_data *next, *wnext;
    long             cube;
    char             value[MAX_ELENGTH];
    char             jflag[MAX_ELENGTH];
    int              level;
} ndata;

/*
 * Support for the memory management (use of calloc)
 */
#define SENUM_ALLOC(type, num)                                                 \
  ((type *)calloc((unsigned)(num), (unsigned)sizeof(type)))

int stg_statecmp();

int stg_statehash();

void stg_init_state_hash();

void stg_end_state_hash();

/*
void stg_print_hashed_state();
*/
void stg_translate_hashed_code();

unsigned *stg_get_state_hash();

ndata *nptr();

void setnptr();

/* Old use of the undef1 has been discontinued
#define nptr(node)	((ndata *) (node)->undef1)
#define setnptr(node,n)	((node)->undef1 = (char *) (n))
*/

network_t *copy;
ndata     **stg_pstate, **stg_nstate, **real_po;
int       *stg_estate;
int       nlatch, npi, npo;
int       stg_longs_per_state, stg_bits_per_long;
int       total_no_of_states;
long      total_no_of_edges;
unsigned  *unfinish_head, *hashed_state;
st_table  *slist;
st_table  *state_table;
int       n_varying_nodes;
ndata     **varying_node;

#define SCHEDULED 1
#define ALL_ASSIGNED 2
#define MARKED 4
#define CHANGED 8

void ctable_enum();

unsigned *shashcode();

void enumerate();

void stg_sc_sim();

void level_circuit();

void rearrange_gate_inputs();

void stg_copy_names();

void stg_copy_clock_data();

void stg_set_network_pipo_names();

#endif