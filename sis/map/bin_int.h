
/* file @(#)bin_int.h	1.3 */
/* last modified on 5/14/91 at 23:07:37 */
#ifndef BIN_INT_H
#define BIN_INT_H

 /* historically, for binned load values; now we use PWL's */

#define BIN_SLOT		bin
#define BIN(node)		((bin_t *) (node)->BIN_SLOT)

#include "pwl.h"

typedef struct {
  delay_time_t arrival;
  node_t *input;
  node_t *leaf;
  int fanout_index;
  int pin_polarity;
} pin_info_t;

typedef enum { 
  WIRE_BUCKET,
  GATE_BUCKET,
  PI_BUCKET,
  CONSTANT_BUCKET
} delay_bucket_type_t;

typedef struct delay_bucket_struct delay_bucket_t;
struct delay_bucket_struct {
  delay_bucket_type_t type;
  lib_gate_t *gate;
  int ninputs;
  node_t **save_binding;
  pin_info_t *pin_info;
  delay_pwl_t pwl;
  delay_bucket_t *next;		/* used in chain of buckets to free them all */
};

typedef struct {
  lib_gate_t *gate;
  int ninputs;
  node_t **save_binding;
  double area;
} area_bucket_t;

 /* fanout_bucket_t contains the information related to buffers in a fanout tree that are in direct */
 /* contact with leaves. The load at those buffers is the total load the buffer is expected to drive. */

typedef struct fanout_bucket_struct fanout_bucket_t;
struct fanout_bucket_struct {
  double load;				/* total load expected on that output terminal */
  delay_pwl_t pwl;			/* delay info associated with that output terminal */
  fanout_bucket_t *next;		/*  for allocation/deallocation*/
};

typedef struct {
  double load;				/* the load of an entry is the expected load at the corresponding leaf */
  fanout_bucket_t *bucket;		/* point to entry in ../fanout_buckets */
} fanout_leaf_t;

#include "fanout_int.h"

typedef struct {
  int n_fanouts;
  fanout_bucket_t *buckets;		/* pointer to list of buckets allocated for that instance */
  fanout_leaf_t *leaves[POLAR_MAX];	/* [POLARITY(2)][n_fanouts] */
  double loads[POLAR_MAX];		/* the load at these sources (est.) */
  st_table *index_table;		/* takes a fanout of this node, returns its fanout_index */
  int best_is_inverter;			/* flag set if choice of gate at multi fanout point is an inverter */
  int polarity;				/* polarity of this multiple fanout node (POLAR_X iff nfanin==0) */
} multi_fanout_t;

typedef struct {
  pwl_t *pwl;
  area_bucket_t area; 
  multi_fanout_t *fanout;
  int visited;
} bin_t;

 /* from bin_delay.c */

extern delay_bucket_t *bin_delay_compute_gate_bucket(/* lib_gate_t *gate; int ninputs; pin_info_t *pin_info; */);
extern delay_pwl_t bin_delay_compute_gate_pwl(/* lib_gate_t *gate; int ninputs; pin_info_t *pin_info; */);
extern void bin_delay_update_node_pwl(/* node_t *node, delay_bucket_t *bucket */);
extern delay_time_t bin_delay_compute_pwl_delay(/* pwl_t *pwl; double load; */);
extern delay_pwl_t bin_delay_select_active_pwl_delay(/* pwl_t *pwl; double load; */);
extern void bin_delay_update_node_pwl(/* node_t *node; delay_bucket_t *bucket; */);
extern delay_pwl_t bin_delay_get_pi_delay_pwl(/* node_t *node; */);
extern delay_time_t bin_delay_compute_pwl_delay(/* pwl_t *pwl, double load */);
extern delay_time_t bin_delay_compute_delay_pwl_delay(/* delay_pwl_t pwl, double load */);


 /* from fanout_est.c */

extern void fanout_est_get_prim_pin_fanout(/* prim_t *prim; int pin; pin_info_t *pin_info; */);
extern void fanout_est_compute_fanout_info(/* node_t *node; */);
extern fanout_bucket_t *fanout_est_fanout_bucket_alloc(/* multi_fanout_t *fanout */);
extern void fanout_est_free_fanout_info(/* node_t *node; */);
extern delay_time_t fanout_est_get_pin_arrival_time(/* pin_info_t *pin_info; double pin_load; */);


#endif /* BIN_INT_H */
