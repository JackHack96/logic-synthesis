/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/clock/clock.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:18 $
 *
 */
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
enum clock_setting_enum {
    SPECIFICATION = 0,
    WORKING = 1
    };

typedef enum clock_param_enum clock_param_t;
enum clock_param_enum {
    CLOCK_NOMINAL_POSITION, CLOCK_ABSOLUTE_VALUE,
    CLOCK_LOWER_RANGE, CLOCK_UPPER_RANGE
    };

/*
 * Structure to store the parameters of an edge 
 */
typedef struct clock_val clock_val_t ;
struct clock_val {
    double nominal;		/* Nominal value (fraction of cycle-time) */
    double lower_range;		/* Absolute deviation on lower side */
    double upper_range;         /* Absolute deviation on upped side */
    };

/* 
 * Structure to report dependencies of the clock edges 
 */
typedef struct clock_edge clock_edge_t ;
typedef struct clock_struct sis_clock_t; 

struct clock_edge{
    sis_clock_t *clock;
    int transition;		/* RISE_TRANSITION or FALL_TRANSITION */
    };

/*
 * A clock structure 
 */
struct clock_struct {
    char *name;			/* Name of clock signal */
    lsHandle net_handle;	/* Handle inside network clock_defn */
    network_t *network;		/* Pointer to the network */
    lsList dependency[2][2];    /* Dependeny lists of the two edges */
    clock_edge_t edges[2];	/* Clock edges -- for dependency code */
    clock_val_t value[2][2];    /* Clock values ----- [i][j] */
				/* i = RISE_TRANSITION or FALL_TRANSITION */
				/* j = SPECIFICATION or WORKING */
    };


typedef struct network_clock_struct network_clock_t; 
struct network_clock_struct {
    clock_setting_t flag;    /* SPECIFICATION or WORKING */
    double cycle_time[2];    /* Stores the cycle time */
    lsList clock_defn;       /* Linked list of "sis_clock_t" structures */
    };

/*
 * Macro to go through all the clocks 
 */
#define foreach_clock(network, gen, clock)                       \
    for(gen = lsStart(CLOCK(network)->clock_defn);               \
        (lsNext(gen, (lsGeneric *) &clock, LS_NH) == LS_OK)      \
            || ((void) lsFinish(gen), 0); )
/* 
 * List of exported routines
 */
EXTERN int clock_add_to_network ARGS((network_t *, sis_clock_t *));
EXTERN int clock_set_parameter ARGS((clock_edge_t, enum clock_param_enum, double));
EXTERN int clock_add_dependency ARGS((clock_edge_t, clock_edge_t));
EXTERN int clock_delete_from_network ARGS((network_t *, sis_clock_t *));
EXTERN void clock_set_cycletime ARGS((network_t *, double));
EXTERN void clock_free ARGS((sis_clock_t *));
EXTERN void network_clock_dup ARGS((network_t *, network_t *));
EXTERN void network_clock_free ARGS((network_t *));
EXTERN int network_num_clock ARGS((network_t *));
EXTERN int clock_num_dependent_edges ARGS((clock_edge_t));
EXTERN void network_clock_alloc ARGS((network_t *));
EXTERN void clock_remove_dependency ARGS((clock_edge_t, clock_edge_t));
EXTERN void clock_set_current_setting ARGS((network_t *, enum clock_setting_enum));
EXTERN char *clock_name ARGS((sis_clock_t *));
EXTERN lsGen clock_gen_dependency ARGS((clock_edge_t));
EXTERN sis_clock_t *clock_get_by_name ARGS((network_t *, char *));
EXTERN sis_clock_t *clock_create ARGS((char *));
EXTERN double clock_get_parameter ARGS((clock_edge_t, enum clock_param_enum));
EXTERN double clock_get_cycletime ARGS((network_t *));
EXTERN clock_setting_t clock_get_current_setting ARGS((network_t *));

/*
 * The following routines are internal to the clock package --
 * they are placed here to avoid creation of another file clock_int.h
 */
EXTERN int clock_get_current_setting_index ARGS((network_t *));

#endif /* CLOCK_H */
