/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/simplify/simp_int.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:49 $
 *
 */
typedef struct sim_flag_struct {
    sim_method_t method;
    sim_accept_t accept;
    sim_dctype_t dctype;
} sim_flag_t;

#define SIM_SLOT		 simplify
#define SIM_FLAG(node)		 ((sim_flag_t *) ((node)->SIM_SLOT))

typedef struct cspf_struct{
    node_t  *node;
    int     level;
	int order;
	array_t *list;
	bdd_t *bdd;
	var_set_t *set;
} cspf_type_t;

typedef struct odc_struct{
    bdd_t  *f;
    bdd_t  *var;
    array_t *vodc;
    int    order;  
    int    level;
    int    value;
    int    po;
} odc_type_t;

typedef struct double_node_struct{
    node_t *pos;
	node_t *neg;
} double_node_t;

#define OBS cspf
#define CSPF(node)  ((cspf_type_t *) (node)->OBS)
#define ODC(node)  ((odc_type_t *) (node)->OBS)

/* constants for filtering */
#define F_SET 1
#define DC_SET 2
#define SIZE_BOUND 3
#define DIST_BOUND 2
#define sm_ncols(M) M->ncols

/* simp_dc.c */
extern int    simp_fanin_level;
extern int    simp_fanin_fanout_level;
extern bool   simp_debug;
extern bool   simp_trace;
extern bool   hsimp_debug;

/* simp_dc.c */
extern node_t *simp_all_dc();
extern node_t *simp_tfanin_dc();
extern node_t *simp_fanout_dc();
extern node_t *simp_inout_dc();
extern node_t *simp_sub_fanin_dc();
extern node_t *simp_sub_x();
extern node_t *simp_level_dc();

/* filter.c */
extern node_t *simp_dc_filter();

/* simp_daemon.c */
extern void   simp_alloc();
extern void   simp_free();
extern void   simp_invalid();
extern void   simp_dup();

/* filter_util.c */
extern void	 fdc_sm_bp_1();
extern void	 fdc_sm_bp_2();
extern void	 fdc_sm_bp_4();
extern void	 fdc_sm_bp_6();
extern void	 fdc_sm_bp_7();
extern array_t	 *sm_col_count_init();
extern sm_col	 *sm_get_long_col();

/* simp_sm.c */
extern sm_matrix *simp_node_to_sm();
extern node_t	 *simp_sm_to_node();
extern void 	 simp_sm_print();

/* simp_order.c */
extern array_t   *simp_order();

/* compute_dc.c */
extern void simplify_with_odc();
extern void simplify_without_odc();
extern bdd_t *cspf_bdd_dc();
extern int odc_value();
extern void find_odc_level();
extern void cspf_alloc();
extern void cspf_free();
extern void odc_alloc();
extern void odc_free();
extern int level_node_cmp1();
extern int level_node_cmp2();
extern int level_node_cmp3();

/* simp_util.c */
extern void copy_dcnetwork();
extern node_t *find_node_exdc();
extern void free_dcnetwork_copy();
extern array_t *order_nodes_elim();
extern int num_cube_cmp();
extern int fsize_cmp();

/* simp_image.c */
extern node_t *simp_bull_cofactor();
extern int set_size_sort();

/* simp.c */
extern void simplify_cspf_node();

/* com_simp.c */
extern st_table *find_node_level();
