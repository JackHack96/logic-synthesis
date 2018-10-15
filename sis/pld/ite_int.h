#include "act_bool.h"

/*----------------------- Will's file starts --------------------------------*/

/* commenting the following to make mapping work */
/* #define ite_slot 	ite 
#define ite_get(node)	((ite_vertex *) (node)->ite_slot) */

typedef	struct ite_vertex_defn	{
		struct ite_vertex_defn	*IF,*THEN,*ELSE;
		struct fo_ite_vertex	*fo;
		int 			index;
		int			value; /* 1 for ONE, 0 for ZERO, 2 for literal,*/
						/*3 for nonterminal node*/
		int			id;
		int			mark; /* for printing use temporarily */
		int			print_mark;	/* 0 for have been printed */
		int			index_size;
		node_t			*node;
		node_t			*fanin;
		char 			*name;
		int			multiple_fo;
		int			multiple_fo_for_mapping;
		int			cost;
		double			arrival_time;
		int			pattern_num;
		int			mapped;
		struct	ite_vertex_defn	*parent;
		int			phase;
		}	ite_vertex, *ite_vertex_ptr;

typedef struct	fo_ite_vertex {
		ite_vertex_ptr *parent_ite_ptr;
		struct fo_ite_vertex	*next;
		} fanout_ite;

/*-----------------------------my file starts -------------------------------*/

#define EXPENSIVE 1
#define INEXPENSIVE 0
#define NEW 1
#define OLD 0
#define MAP_WITH_ITER 2
#define MAP_WITH_JUST_DECOMP 3
#define USE_GOOD_DECOMP 0
#define USE_FACTOR 1

typedef struct act_ite_cost_struct_defn{
	node_t *node;           /* pointer to the node of the network */
	int cost;		/* number of basic blocks used to realize the node */
	double arrival_time;	/* arrival_time at the output of the node */
	double required_time;	/* required_time at the output of the node */
        double slack;		/* slack = required_time - arrival_time */
	int is_critical;	/* is 1 if the slack is <= some threshold */
	double area_weight;	/* penalty if node is collapsed */
        double cost_and_arrival_time;
	ite_vertex_ptr  ite;	/* stores the ite for the node */
        ACT_VERTEX_PTR  act;    /* stores the robdd for the node */
        char *will_ite; 
        ACT_MATCH *match;       /* if the cost is 1, stores the match here */
        network_t *network;     /* if MAP_METHOD is MAP_WITH_ITER, this is 
                                   the network associated with the node */
} ACT_ITE_COST_STRUCT;

#define ACT_DEF_ITE_SLOT(node)  node->ite
#define ACT_ITE_SLOT(node)  ((ACT_ITE_COST_STRUCT *)node->ite)
#define ACT_ITE_ite(node)   (ACT_ITE_SLOT(node)->ite)
#define ACT_ITE_act(node)   (ACT_ITE_SLOT(node)->act)
#define ACT_ITE_will_ite(node) (ACT_ITE_SLOT(node)->will_ite)
/* ite_map.c */
extern int act_is_or_used;
extern int ACT_ITE_DEBUG;
extern ite_vertex_ptr PRESENT_ITE;
extern ACT_VERTEX_PTR PRESENT_ACT;

/* #ifndef
#define MAXINT 10000000
#endif
*/  /* Feb 8, 1992 - defined in pld_int.h */
extern int act_map_ite();
extern act_ite_map_network_with_iter();

/* com_ite.c */
extern void act_ite_alloc();
extern void act_ite_free();
extern int ACT_ITE_DEBUG;
extern int ACT_ITE_STATISTICS;
extern int ACT_BDD_NEW;   
extern int USE_FAC_WHEN_UNATE; /* only used when V >= 0, and new mapper */
extern int ACT_ITE_ALPHA; /* for binateness of the variable */
extern int ACT_ITE_GAMMA; /* for binateness of the resulting functions */
extern int UNATE_SELECT;  /* if 1, then apply max column cover for unate functions for m = 1 */

/* ite_urp.c */
extern ite_vertex *ite_literal();
extern ite_vertex *my_shannon_ite();
extern void ite_split_F();
extern st_table *ite_end_table;
extern int ACT_ITE_FIND_KERNEL;

/* act_ite.c */
extern node_t *act_make_node_from_row();
extern ite_vertex *ite_OR_itevec();

/* ite_leaf.c */
extern ite_vertex *ite_check_for_single_literal_cubes();

/* ite_util.c */
/* 
extern node_t *pld_remap_get_node();
*/ /* pld_int.h */
extern node_t *ite_get_node_literal_of_vertex();
extern st_table *ite_my_traverse_ite();

/* ite_break.c */
extern node_t *act_ite_get_function();
extern node_t *act_new_act_get_function();

/* ite_new_urp.c */
extern ite_vertex *ite_for_muxnode();
extern ite_vertex *ite_buffer();
extern ite_vertex *ite_inv();
extern ite_vertex *ite_get_vertex();
extern ite_vertex *ite_new_OR();
extern ite_vertex *ite_new_OR_with_inv();
extern ite_vertex *ite_create_ite_for_orthogonal_cubes();
extern ite_vertex *ite_new_ite_for_unate_cover();
extern ite_vertex *ite_convert_match_to_ite();
extern node_t *node_most_binate_variable();
extern node_t *node_most_binate_variable_new();
extern node_t *ite_get_minimum_cost_variable();
extern ite_vertex *ite_new_literal();

/* act_ite_new.c */
extern ite_vertex *ite_new_ite_for_cubenode();
extern ite_vertex *ite_new_ite_for_single_literal_cubes();
extern ite_vertex *ite_new_ite_and();
extern ite_vertex *ite_new_ite_or();

/* ite_new_map.c */
extern act_ite_map_node_with_iter_imp();

/* ite_new_bdd.c */
extern network_t *act_map_using_new_bdd();

/* ite_factor.c */
extern ite_vertex *act_ite_create_from_factored_form();
extern ite_vertex *act_ite_factored_and();
extern ite_vertex *act_ite_factored_or();
extern ite_vertex *act_ite_get_ite();

/*---------------------------- my file ends ----------------------------*/
	

#define ite_slot        (ACT_ITE_COST_STRUCT *) ite
#define ite_get(node)	((ite_vertex_ptr) (ACT_ITE_SLOT(node)->will_ite))

	
extern void ite_clear_dag();
extern ite_vertex *ite_alloc();
extern void ite_print_dag();
extern void ite_print_out();
extern void ite_assgn_num();
extern void ite_print();

