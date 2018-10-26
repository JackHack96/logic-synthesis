#ifndef SIMP_INT_H
#define SIMP_INT_H

#include "simplify.h"
#include "array.h"
#include "bdd.h"

typedef struct sim_flag_struct {
    sim_method_t method;
    sim_accept_t accept;
    sim_dctype_t dctype;
} sim_flag_t;

#define SIM_SLOT simplify
#define SIM_FLAG(node) ((sim_flag_t *)((node)->SIM_SLOT))

typedef struct cspf_struct {
    node_t    *node;
    int       level;
    int       order;
    array_t   *list;
    bdd_t     *bdd;
    var_set_t *set;
} cspf_type_t;

typedef struct odc_struct {
    bdd_t   *f;
    bdd_t   *var;
    array_t *vodc;
    int     order;
    int     level;
    int     value;
    int     po;
} odc_type_t;

typedef struct double_node_struct {
    node_t *pos;
    node_t *neg;
} double_node_t;

#define OBS cspf
#define CSPF(node) ((cspf_type_t *)(node)->OBS)
#define ODC(node) ((odc_type_t *)(node)->OBS)

/* constants for filtering */
#define F_SET 1
#define DC_SET 2
#define SIZE_BOUND 3
#define DIST_BOUND 2
#define sm_ncols(M) M->ncols

/* simp_dc.c */
int  simp_fanin_level;
int  simp_fanin_fanout_level;
bool simp_debug;
bool simp_trace;
bool hsimp_debug;

/* simp_dc.c */
node_t *simp_all_dc();

node_t *simp_tfanin_dc();

node_t *simp_fanout_dc();

node_t *simp_inout_dc();

node_t *simp_sub_fanin_dc();

node_t *simp_sub_x();

node_t *simp_level_dc();

/* filter.c */
node_t *simp_dc_filter();

/* simp_daemon.c */
void simp_alloc();

void simp_free();

void simp_invalid();

void simp_dup();

/* filter_util.c */
void fdc_sm_bp_1();

void fdc_sm_bp_2();

void fdc_sm_bp_4();

void fdc_sm_bp_6();

void fdc_sm_bp_7();

array_t *sm_col_count_init();

sm_col *sm_get_long_col();

/* simp_sm.c */
sm_matrix *simp_node_to_sm();

node_t *simp_sm_to_node();

void simp_sm_print();

/* simp_order.c */
array_t *simp_order();

/* compute_dc.c */
void simplify_with_odc();

void simplify_without_odc();

bdd_t *cspf_bdd_dc();

int odc_value();

void find_odc_level();

void cspf_alloc();

void cspf_free();

void odc_alloc();

void odc_free();

int level_node_cmp1();

int level_node_cmp2();

int level_node_cmp3();

/* simp_util.c */
void copy_dcnetwork();

node_t *find_node_exdc();

void free_dcnetwork_copy();

array_t *order_nodes_elim();

int num_cube_cmp();

int fsize_cmp();

/* simp_image.c */
node_t *simp_bull_cofactor();

int set_size_sort();

/* simp.c */
void simplify_cspf_node();

/* com_simp.c */
st_table *find_node_level();

#endif