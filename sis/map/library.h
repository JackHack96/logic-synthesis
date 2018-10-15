/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/map/library.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:25 $
 *
 */
/* file @(#)library.h	1.2 */
/* last modified on 5/1/91 at 15:51:12 */
#ifndef LIBRARY_H
#define LIBRARY_H


typedef struct library_struct library_t;
struct library_struct {
    lsList classes;		/* list of lib_class_t class descriptions */
    lsList gates;		/* list of lib_gate_t gate descriptions */
    lsList patterns;		/* list of prim_t patterns */
    int nand_flag;
    int add_inverter;
};


typedef struct lib_class_struct lib_class_t;
struct lib_class_struct {
    network_t *network;		/* the logic function for the class */
    lib_class_t *dual_class;	/* pointer to class for complement of fct */
    lsList gates;		/* list of gates in this class */
    library_t *library;		/* pointer back to library */
    char *name;			/* reserved for future use */
};

#ifdef SIS
/* sequential support */
typedef struct latch_time_struct latch_time_t;
struct latch_time_struct {
  double setup;
  double hold;
};
#endif

typedef struct lib_gate_struct lib_gate_t;
struct lib_gate_struct {
  network_t *network; 		/* exactly equal to gate->class->network */
  char *name; 			/* the gate name */
  char **delay_info; 		/* the input pin delays */
  double area; 			/* the gate area */
  int symmetric; 		/* it is more or less symmetric WRT input pins */
  lib_class_t *class_p; 	/* pointer back to the class */

#ifdef SIS
  /* sequential support */
  int type; 			/* type of gate -- lib_gate_type returns latch_synch_t */
  int latch_pin; 		/* index for the latch output pin (-1 if none) */
  latch_time_t **latch_time_info; /* setup/hold times */
  delay_pin_t *clock_delay; 	/* delay from clock to output */
  char *control_name; 		/* name of the clock pin */
#endif
};


/* normal library functions */
EXTERN library_t *lib_get_library ARGS((void));
EXTERN lsGen lib_gen_classes ARGS((library_t *));
EXTERN lsGen lib_gen_gates ARGS((lib_class_t *));
EXTERN lib_class_t *lib_get_class ARGS((network_t *, library_t *));
EXTERN char *lib_class_name ARGS((lib_class_t *));
EXTERN network_t *lib_class_network ARGS((lib_class_t *));
EXTERN lib_class_t *lib_class_dual ARGS((lib_class_t *));
EXTERN lib_gate_t *lib_get_gate ARGS((library_t *, char *));
EXTERN char *lib_gate_name ARGS((lib_gate_t *));
EXTERN char *lib_gate_pin_name ARGS((lib_gate_t *, int, int));
EXTERN double lib_gate_area ARGS((lib_gate_t *));
EXTERN lib_class_t *lib_gate_class ARGS((lib_gate_t *));
EXTERN int lib_gate_num_in ARGS((lib_gate_t *));
EXTERN int lib_gate_num_out ARGS((lib_gate_t *));

#ifdef SIS
/* sequential support */
#define lib_gate_type(g) (((g)==NIL(lib_gate_t))?UNKNOWN:(enum latch_synch_enum)(g)->type)
#define lib_gate_latch_pin(g) (((g)==NIL(lib_gate_t))?-1:(g)->latch_pin)
#define lib_gate_latch_time(g) (((g)==NIL(lib_gate_t))?NIL(latch_time_t*):(g)->latch_time_info)
#define lib_gate_clock_delay(g) (((g)==NIL(lib_gate_t))?NIL(delay_pin_t):(g)->clock_delay)
EXTERN lib_gate_t *lib_choose_smallest_latch ARGS((library_t *, char *, enum latch_synch_enum));
#endif

#ifdef SIS
EXTERN lib_class_t *lib_get_class_by_type ARGS((network_t *, library_t *, latch_synch_t));
#else
EXTERN lib_class_t *lib_get_class_by_type ARGS((network_t *, library_t *));
#endif

/* for mapped nodes/networks */
EXTERN lib_gate_t *lib_gate_of ARGS((node_t *));
EXTERN int lib_network_is_mapped ARGS((network_t *));
EXTERN int lib_set_gate ARGS((node_t *, lib_gate_t *, char **, node_t **, int));

/* obsolete */
EXTERN char *lib_get_gate_name ARGS((node_t *));
EXTERN char *lib_get_pin_name ARGS((node_t *, int));
EXTERN char *lib_get_out_pin_name ARGS((node_t *, int));
EXTERN char *lib_get_pin_delay ARGS((node_t *, int));
EXTERN double lib_get_gate_area ARGS((node_t *));

#endif
