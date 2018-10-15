
/* file @(#)library.h	1.2 */
/* last modified on 5/1/91 at 15:51:12 */
#ifndef LIBRARY_H
#define LIBRARY_H

#include "list.h"
#include "network.h"
#include "node.h"
#include "delay.h"
#include "latch.h"

typedef struct library_struct library_t;
struct library_struct {
    lsList classes;  /* list of lib_class_t class descriptions */
    lsList gates;    /* list of lib_gate_t gate descriptions */
    lsList patterns; /* list of prim_t patterns */
    int    nand_flag;
    int    add_inverter;
};

typedef struct lib_class_struct lib_class_t;
struct lib_class_struct {
    network_t   *network;      /* the logic function for the class */
    lib_class_t *dual_class; /* pointer to class for complement of fct */
    lsList      gates;            /* list of gates in this class */
    library_t   *library;      /* pointer back to library */
    char        *name;              /* reserved for future use */
};

/* sequential support */
typedef struct latch_time_struct latch_time_t;
struct latch_time_struct {
    double setup;
    double hold;
};

typedef struct lib_gate_struct lib_gate_t;
struct lib_gate_struct {
    network_t   *network;   /* exactly equal to gate->class->network */
    char        *name;           /* the gate name */
    char        **delay_info;    /* the input pin delays */
    double      area;          /* the gate area */
    int         symmetric;        /* it is more or less symmetric WRT input pins */
    lib_class_t *class_p; /* pointer back to the class */

    /* sequential support */
    int          type;      /* type of gate -- lib_gate_type returns latch_synch_t */
    int          latch_pin; /* index for the latch output pin (-1 if none) */
    latch_time_t **latch_time_info; /* setup/hold times */
    delay_pin_t  *clock_delay;       /* delay from clock to output */
    char         *control_name;             /* name of the clock pin */
};

/* normal library functions */
library_t *lib_get_library(void);

lsGen lib_gen_classes(library_t *);

lsGen lib_gen_gates(lib_class_t *);

lib_class_t *lib_get_class(network_t *, library_t *);

char *lib_class_name(lib_class_t *);

network_t *lib_class_network(lib_class_t *);

lib_class_t *lib_class_dual(lib_class_t *);

lib_gate_t *lib_get_gate(library_t *, char *);

char *lib_gate_name(lib_gate_t *);

char *lib_gate_pin_name(lib_gate_t *, int, int);

double lib_gate_area(lib_gate_t *);

lib_class_t *lib_gate_class(lib_gate_t *);

int lib_gate_num_in(lib_gate_t *);

int lib_gate_num_out(lib_gate_t *);

/* sequential support */
#define lib_gate_type(g)                                                       \
  (((g) == NIL(lib_gate_t)) ? UNKNOWN : (latch_synch_t)(g)->type)
#define lib_gate_latch_pin(g) (((g) == NIL(lib_gate_t)) ? -1 : (g)->latch_pin)
#define lib_gate_latch_time(g)                                                 \
  (((g) == NIL(lib_gate_t)) ? NIL(latch_time_t *) : (g)->latch_time_info)
#define lib_gate_clock_delay(g)                                                \
  (((g) == NIL(lib_gate_t)) ? NIL(delay_pin_t) : (g)->clock_delay)

lib_gate_t *lib_choose_smallest_latch(library_t *library, char *string, latch_synch_t latch_type);

lib_class_t *lib_get_class_by_type(network_t *, library_t *,
                                          latch_synch_t);

/* for mapped nodes/networks */
lib_gate_t *lib_gate_of(node_t *);

int lib_network_is_mapped(network_t *);

int lib_set_gate(node_t *, lib_gate_t *, char **, node_t **, int);

/* obsolete */
char *lib_get_gate_name(node_t *);

char *lib_get_pin_name(node_t *, int);

char *lib_get_out_pin_name(node_t *, int);

char *lib_get_pin_delay(node_t *, int);

double lib_get_gate_area(node_t *);

#endif
