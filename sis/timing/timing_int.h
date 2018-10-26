
#include "sis.h"
/* definitions */
#define EPS 1e-3
#define EPS1 1e-5
#define EPS2 1e-2
#define INFTY 10000
#define NOT_SET -2
#define NNODE 1
#define LATCH 2
#define FFF 1
#define FFR 2
#define LSH 3
#define LSL 4

/* Data structures for Optimal Clocking and Clock Verification Routines*/

/* Debug options */
typedef enum debug_enum debug_type_t;
enum debug_enum {
    LGRAPH, CGRAPH, BB, GENERAL, NONE, VERIFY, ALL
};

typedef enum algorithm_enum algorithm_type_t;
enum algorithm_enum {
    OPTIMAL_CLOCK, CLOCK_VERIFY
};

/* LATCH GRAPH structures: common for optimal clocking and clock 
   verification  */     
typedef struct copt_node_struct copt_node_t;
typedef struct cv_node_struct cv_node_t;

typedef struct l_node_struct l_node_t;
struct l_node_struct {
    int pio;
    int num;
    latch_t *latch;
    int latch_type;
    int phase;
    copt_node_t *copt;
    cv_node_t *cv;
};

typedef struct l_edge_struct l_edge_t;
struct l_edge_struct {
    double Dmax, Dmin;
    int K;
    double w;
};

typedef struct sp_matrix_struct sp_matrix_t;
typedef struct l_graph_struct l_graph_t;
struct l_graph_struct {
    array_t *clock_order;
    vertex_t *host;
};


#define GRAPH(g) ((l_graph_t *)g->user_data)
#define NODE(n) ((l_node_t *)n->user_data)
#define EDGE(e) ((l_edge_t *)e->user_data)
#define De(e) (EDGE(e)->Dmax)
#define de(e) (EDGE(e)->Dmin)
#define Ke(e) (EDGE(e)->K)
#define We(e) (EDGE(e)->w)
#define ID(v) (NODE(v)->num)
#define TYPE(v) (NODE(v)->pio)
#define LTYPE(v) (NODE(v)->latch_type)
#define NLAT(v) (NODE(v)->latch)
#define PHASE(v) (NODE(v)->phase)
#define CL(g) (GRAPH(g)->clock_order)
#define HOST(g) (GRAPH(g)->host)

/* END of LATCH GRAPH structures */

/* CONSTRAINT GRAPH structes: used ONLY for OPTIMAL CLOCKING*/
typedef struct c_edge_struct c_edge_t;
struct c_edge_struct{
    int ignore;
    int evaluate_c;
    st_table *c_1;
    double w, fixed_w, duty, *c_2;
};

typedef struct c_node_struct c_node_t;
struct c_node_struct{
    double p;
    int id, m_id;
};
typedef struct phase_struct phase_t;
struct phase_struct{
    vertex_t *rise, *fall;
};

typedef struct c_graph_struct c_graph_t;
struct c_graph_struct {
    phase_t **phase_list;
    int num_phases;
    vertex_t *zero;
};

#define PHASE_LIST(g) (((c_graph_t *)g->user_data)->phase_list)
#define NUM_PHASE(g)  (((c_graph_t *)g->user_data)->num_phases)
#define ZERO_V(g) (((c_graph_t *)g->user_data)->zero)
#define USER(e) ((c_edge_t *)e->user_data)
#define CONS1(e) (USER(e)->c_1)
#define CONS2(e) (USER(e)->c_2)
#define WEIGHT(e) (USER(e)->w)
#define FIXED(e) (USER(e)->fixed_w)
#define DUTY(e) (USER(e)->duty)
#define EVAL(e) (USER(e)->evaluate_c)
#define IGNORE(e) (USER(e)->ignore)
#define P(v) (((c_node_t *)v->user_data)->p)
#define PID(v) (((c_node_t *)v->user_data)->id)
#define MID(v) (((c_node_t *)v->user_data)->m_id)
#define FROM 0
#define TO 1
#define get_vertex(v, flag, g) \
             (flag == FROM ? ((LTYPE(v) == FFR || LTYPE(v) == \
              LSH) ? PHASE_LIST(g)[PHASE(v) - 1]->rise : \
              PHASE_LIST(g)[PHASE(v)-1]->fall) : \
	      ((LTYPE(v) == FFR || LTYPE(v) == \
              LSL) ? PHASE_LIST(g)[PHASE(v) - 1]->rise : \
              PHASE_LIST(g)[PHASE(v)-1]->fall))

#define get_index(v, flag, g) \
             (flag == FROM ? ((LTYPE(v) == FFR || LTYPE(v) == \
              LSH) ? PHASE(v) - 1 : \
	      PHASE(v)-1+NUM_PHASE(g)) : \
	      ((LTYPE(v) == FFR || LTYPE(v) == \
              LSL) ? PHASE(v) - 1 : \
              PHASE(v)-1+NUM_PHASE(g)))

#define get_vertex_from_index(g, i) \
             (i < NUM_PHASE(g) ? (PHASE_LIST(cg)[i])->rise : \
                                 (PHASE_LIST(cg)[(i - NUM_PHASE(g))])->fall)

/* END of CONSTRAINT GRAPH structure */

/* MATRIX STRUCTURES for Solving the Optimal CLocking Problem */
typedef struct matrix_struct matrix_t;
struct matrix_struct {
    double **W_old, **W_new, **beta_old, **beta_new, c, c_L, c_U;
    int num_v;
};

#define W_n(m) (m->W_new)
#define W_o(m) (m->W_old)
#define BETA_n(m) (m->beta_new)
#define BETA_o(m) (m->beta_old)
#define CURRENT_CLOCK(m) (m->c)
#define LOWER_BOUND(m) (m->c_L)
#define UPPER_BOUND(m) (m->c_U)
#define NUM_V(m) (m->num_v)
/* END of MATRIX structure */

/* DELAY structure for sequential delay trace */
typedef struct my_delay_struct my_delay_t;
struct my_delay_struct {
    delay_time_t max, min;
    int dirty;
};
#define UNDEF(n) (n->undef1)
#define MDEL(n) ((my_delay_t *)(n->undef1))
#define maxdr(n) (MDEL(n)->max.rise)
#define maxdf(n) (MDEL(n)->max.fall)
#define mindr(n) (MDEL(n)->min.rise)
#define mindf(n) (MDEL(n)->min.fall)
#define DIRTY(n) (MDEL(n)->dirty)
#define MVER(l) ((vertex_t *)(l->undef1))
/* END of DELAY structures */

/* GRAPH structures for OPTIMAL CLOCK COMPUTATION*/
struct copt_node_struct {
    double w, prevw;
    int r, dirty;
    vertex_t *parent;
};
#define COPT(v) (NODE(v)->copt)
#define Wv(v) (COPT(v)->w)
#define PREV_Wv(v) (COPT(v)->prevw)
#define Rv(v) (COPT(v)->r)
#define PARENT(v) (COPT(v)->parent)
#define COPT_DIRTY(v) (COPT(v)->dirty)

struct cv_node_struct {
    double A, D, a, d;
    int dirty;
};
#define CV(v) (NODE(v)->cv)
#define ARR(v) (CV(v)->A)
#define arr(v) (CV(v)->a)
#define DEP(v) (CV(v)->D)
#define dep(v) (CV(v)->d)
#define CV_DIRTY(v) (CV(v)->dirty)

typedef struct clock_event_struct clock_event_t;
struct clock_event_struct {
    double *rise;
    double *fall;
    int num_phases;
    double *shift;
};
#define NUM_CLOCK(e) (e->num_phases)
#define RISE(e) (e->rise)
#define FALL(e) (e->fall)
#define SHIFT(e) (e->shift)

/* GLOBAL vars */
debug_type_t debug_type;


/* Routine defs */

/* timing_graph.c */
graph_t *tmg_network_to_graph();
int timing_update_K_edge();
int tmg_build_graph();
static void tmg_delay_alloc();
static void tmg_delay_free();
int tmg_all_negative_cycles();

/* timing_util.c */
array_t *tmg_determine_clock_order();
sis_clock_t *tmg_latch_get_clock();
double tmg_get_set_up();
double tmg_get_hold();
double tmg_get_min_sep();
double tmg_get_max_sep();
c_edge_t *tmg_alloc_cedge();
c_graph_t *tmg_alloc_cgraph();
l_graph_t *tmg_alloc_graph();
l_node_t *tmg_alloc_node();
l_edge_t *tmg_alloc_edge();
int tmg_get_gen_algorithm_flag();
double tmg_max_clock_skew();
double tmg_min_clock_skew();

/* timing_comp.c */
int cycle_in_graph();
static int my_pq_cmp();
double tmg_compute_optimal_clock();
graph_t *tmg_construct_clock_graph();
vertex_t *tmg_get_constraint_vertex();
double tmg_guess_clock_bound();
double tmg_clock_lower_bound();
double tmg_solve_constraints();
int tmg_all_negative_cycles();
int tmg_is_feasible();
double tmg_solve_gen_constraints();
array_t *tmg_compute_intervals();
matrix_t *tmg_init_matrix();
int timing_exterior_path_search();
int tmg_update_matrix();

/* timing_seq.c */
delay_time_t  tmg_node_get_delay();
delay_time_t  tmg_map_get_delay();

/* timing_verify.c */
int tmg_check_clocking_scheme();
int trace_recursive_path();
clock_event_t *tmg_get_clock_events();



