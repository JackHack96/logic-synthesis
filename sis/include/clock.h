
#ifndef CLOCK_H
#define CLOCK_H

/*
 * Data structures
 */

#define CLOCK_SLOT clock
#define CLOCK(network) ((network_clock_t *)((network)->CLOCK_SLOT))

#define RISE_TRANSITION 0
#define FALL_TRANSITION 1

#define CLOCK_NOT_SET -1.0

typedef enum clock_setting_enum clock_setting_t;
enum clock_setting_enum { SPECIFICATION = 0, WORKING = 1 };

typedef enum clock_param_enum clock_param_t;
enum clock_param_enum {
  CLOCK_NOMINAL_POSITION,
  CLOCK_ABSOLUTE_VALUE,
  CLOCK_LOWER_RANGE,
  CLOCK_UPPER_RANGE
};

/*
 * Structure to store the parameters of an edge
 */
typedef struct clock_val clock_val_t;
struct clock_val {
  double nominal;     /* Nominal value (fraction of cycle-time) */
  double lower_range; /* Absolute deviation on lower side */
  double upper_range; /* Absolute deviation on upped side */
};

/*
 * Structure to report dependencies of the clock edges
 */
typedef struct clock_edge clock_edge_t;
typedef struct clock_struct sis_clock_t;

struct clock_edge {
  sis_clock_t *clock;
  int transition; /* RISE_TRANSITION or FALL_TRANSITION */
};

/*
 * A clock structure
 */
struct clock_struct {
  char *name;              /* Name of clock signal */
  lsHandle net_handle;     /* Handle inside network clock_defn */
  network_t *network;      /* Pointer to the network */
  lsList dependency[2][2]; /* Dependeny lists of the two edges */
  clock_edge_t edges[2];   /* Clock edges -- for dependency code */
  clock_val_t value[2][2]; /* Clock values ----- [i][j] */
                           /* i = RISE_TRANSITION or FALL_TRANSITION */
                           /* j = SPECIFICATION or WORKING */
};

typedef struct network_clock_struct network_clock_t;
struct network_clock_struct {
  clock_setting_t flag; /* SPECIFICATION or WORKING */
  double cycle_time[2]; /* Stores the cycle time */
  lsList clock_defn;    /* Linked list of "sis_clock_t" structures */
};

/*
 * Macro to go through all the clocks
 */
#define foreach_clock(network, gen, clock)                                     \
  for (gen = lsStart(CLOCK(network)->clock_defn);                              \
       (lsNext(gen, (lsGeneric *)&clock, LS_NH) == LS_OK) ||                   \
       ((void)lsFinish(gen), 0);)

/*
 * List of exported routines
 */
extern int clock_add_to_network(network_t *, sis_clock_t *);

extern int clock_set_parameter(clock_edge_t, enum clock_param_enum, double);

extern int clock_add_dependency(clock_edge_t, clock_edge_t);

extern int clock_delete_from_network(network_t *, sis_clock_t *);

extern void clock_set_cycletime(network_t *, double);

extern void clock_free(sis_clock_t *);

extern void network_clock_dup(network_t *, network_t *);

extern void network_clock_free(network_t *);

extern int network_num_clock(network_t *);

extern int clock_num_dependent_edges(clock_edge_t);

extern void network_clock_alloc(network_t *);

extern void clock_remove_dependency(clock_edge_t, clock_edge_t);

extern void clock_set_current_setting(network_t *, enum clock_setting_enum);

extern char *clock_name(sis_clock_t *);

extern lsGen clock_gen_dependency(clock_edge_t);

extern sis_clock_t *clock_get_by_name(network_t *, char *);

extern sis_clock_t *clock_create(char *);

extern double clock_get_parameter(clock_edge_t, enum clock_param_enum);

extern double clock_get_cycletime(network_t *);

extern clock_setting_t clock_get_current_setting(network_t *);

/*
 * The following routines are internal to the clock package --
 * they are placed here to avoid creation of another file clock_int.h
 */
extern int clock_get_current_setting_index(network_t *);

#endif /* CLOCK_H */
