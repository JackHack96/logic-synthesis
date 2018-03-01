/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/nova.h,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:09 $
 *
 */
#include "../port/port.h"

#define BOOLEAN   int
#define TRUE       1
#define FALSE      0
#define ONE       '1'
#define ZERO      '0'
#define DASH      '-'
#define ANYSTATE "ANY"
#define DCLABEL   -1

#define MAXLINE   1026
#define MAXSTRING 1026
#define MAXWORK   20000

#define is ==
#define isnt !=
#define max(a,b) (a >b) ? a : b
#define free_mem(ptr) if (ptr != NULL) free(ptr)


extern int cost_function,num_moves;

/* data structure representing the input machine symbolic table */

typedef struct inputtable {

        char *input;
        int ilab;
        char *pstate;
        int plab;
        char *nstate;
        int nlab;
        char *output;
        int olab;
	int acc;
        struct inputtable *next;

} INPUTTABLE;


/* data structure representing a constraint among a group of symblemes ( symbolic elements ) */

/* range of "attribute" in data structure "constraint" */
#define NOTUSED        0  /* constraint not already used */
#define PRUNED         1  /* constraint pruned away */
#define USED           2  /* constraint already used in the encoding */
#define TRIED          3  /* constraint being tried in the encoding */

/* range of "odd_type" in data structure "constraint" */
#define ADMITTED       0  /* not-a-power-of-two constraint kept in the lattice */
#define NOTADMITTED    1  /* otherwise */
#define EXAMINED       2  /* not-a-power-of-two-constraint that failed the test
		             to be kept in the lattice since it needed too many
			     free positions */

/* range of "source_type" in data structure "constraint" */
#define INPUT       0  /* constraint obtained from the input structure */
#define OUTPUT      1  /* constraint obtained from the output structure */
#define MIXED       2  /* constraint obtained from both the input and output 
			  structure */


typedef struct constraint {

        char *relation;  /* relation[i] == '1' iff the i-th symbleme belongs to this constraint */
        char *face;    /* face corresponding to this constraint */
	char *next_states; /* next-states associated to this constraint */
        int weight;    /* number of times this constraint appears in the constraint list */
        int level;        /* depth in the intersection chain */
        int newlevel;     /* temporary depth (implementation convenience) */
        int card;         /* # of symblemes in the constraint */
        int attribute;    /* NOTUSED USED PRUNED TRIED */
        int odd_type;     /* ADMITTED NOTADMITTED EXAMINED */
        int source_type;  /* INPUT OUTPUT MIXED */
        struct constraint *levelnext; /* pointer to the next constraint at the 
 same level */
        struct constraint *cardnext;  /* pointer to the next constraint with the same card */
        struct constraint *next;      /* pointer to the next constraint in the 
 global list */

} CONSTRAINT;


/* data structure linking all constraints involving a certain symbleme */

typedef struct constrlink {

        CONSTRAINT *constraint; 
        struct constrlink *next;

} CONSTRLINK;


/* data structure representing binary codes */

typedef struct code {

        char *value;       /* the code in binary form */
        struct code *next;

} CODE;


/* data structure representing the symbolic elements ( i.e. symbolic inputs, outputs , states , called here collectively "symblemes" ) to be encoded */

/* range of "code_status" in data structure "symbleme" */
#define NOTCODED       0  /* symbleme with no code already assigned */
#define TEMPORARY      1  /* symbleme with a temporary code assigned */
#define FIXED          2  /* symbleme with a definitive code assigned */

typedef struct symbleme {

        CONSTRLINK *constraints; /* list of the constraints involving this 
                                    symbleme */
        CODE *vertices;/* list of codes temporarily assigned to this symbleme */
        char *code;              /* definitive code */
        char *name;              /* name of the symbleme */
        int code_status;         /* status of the code currently assigned to 
                                    this symbleme */ 
	char *best_code;         /* best definitive code obtained up to now */
	char *exact_code;        /* exact code */
        char *exbest_code;       /* best exact code */

} SYMBLEME;

/* data structure representing the vertices of an hypercube */

typedef struct hypervertex {

        char *code;        /* boolean code of this hypervertex */
        BOOLEAN ass_status; /* true iff this code has been assigned in a 
                              definitive way */
        int label;

} HYPERVERTEX;


/* GLOBAL VARIABLES */

extern CONSTRAINT *inputnet;   /* net of the constraints on the input symblemes */
extern CONSTRAINT *statenet;   /* net of the constraints on the state symblemes */
extern CONSTRAINT *outputnet; /* net of the constraints on the output symblemes */

extern SYMBLEME *inputs;   /* array of inputs symblemes descriptors */
extern SYMBLEME *states;   /* array of states symblemes descriptors */
extern SYMBLEME *outputs;  /* array of outputs symblemes descriptors */

extern CONSTRAINT **levelarray; /* array of pointers to the constraints with the same level */
extern CONSTRAINT **cardarray;  /* array of pointers to the constraints with the same cardinality */

extern HYPERVERTEX *hypervertices;     /* array of vertices on the hypercube and related information */

extern INPUTTABLE *firstable;      /* pointer to the beginning of the input table */
extern INPUTTABLE *lastable;       /* pointer to the last item of the input table */


extern int *zeroutput; /* zeroutput[i] = n iff the i-th next-state symbleme appears 
                   n times with an all zeroes proper output                  */
extern int inputnum;           /* number of inputs (if ISYMB) */
extern int statenum;           /* number of states */
extern int outputnum;          /* number of outputs (if OSYMB) */
extern int inputfield;         /* length of the input (when not symbolic) */
extern int outputfield;        /* length of the output (when not symbolic) */

extern int productnum;         /* number of rows of the input table */
extern int symblemenum;        /* number of symblemes */

extern int st_codelength;      /* number of bits used to encode the states */
extern int inp_codelength;     /* number of bits used to encode the inputs */
extern int out_codelength;     /* number of bits used to encode the outputs */

extern int onehot_products;    /* number of product terms of the minimized 
				  onehot cover */
extern int min_inputs;         /* number of inputs of the currently minimized 
				  pla */
extern int min_outputs;        /* number of outputs of the currently minimized 
				  pla */
extern int min_products;       /* number of product terms of the currently
				  minimized pla */
extern int best_products;      /* number of product terms of the minimum size 
                                  pla obtained so far */
extern int worst_products;     /* number of product terms of the maximum size 
                                  pla obtained so far */


extern int first_size;         /* size of the minimized pla after the default
				  encoding */
extern int best_size;          /* minimum size of the minimized pla obtained so
				  far      */
extern int rand_trials;        /* number of required random codings */
extern char zero_state[MAXSTRING];       /* user given state to code to zero */
extern char init_state[MAXSTRING];       /* user given initial state         */
extern char init_code[MAXSTRING];        /* code assigned to the init. state */
extern char file_name[MAXSTRING];         /* name of the input file          */
extern char sh_filename[MAXSTRING];      /* name of the input file           */
extern char temp1[MAXSTRING];            /* name of the temp1 file           */
extern char temp2[MAXSTRING];            /* name of the temp2 file           */
extern char temp3[MAXSTRING];            /* name of the temp3 file           */
extern char temp33[MAXSTRING];           /* name of the temp33 file          */
extern char temp4[MAXSTRING];            /* name of the temp4 file           */
extern char temp5[MAXSTRING];            /* name of the temp5 file           */
extern char temp6[MAXSTRING];            /* name of the temp6 file           */
extern char temp7[MAXSTRING];            /* name of the temp7 file           */
extern char temp81[MAXSTRING];           /* name of the temp81 file          */
extern char temp82[MAXSTRING];           /* name of the temp82 file          */
extern char temp83[MAXSTRING];           /* name of the temp83 file          */
extern char temp9[MAXSTRING];            /* name of the temp9 file           */
extern char temp10[MAXSTRING];           /* name of the temp10 file          */
extern char constraints[MAXSTRING];      /* name of the constraints file     */
extern char summ[MAXSTRING];             /* name of summary file             */
extern char esp[MAXSTRING];              /* name of the esp file             */
extern char readscript[MAXSTRING];       /* name of the readscript file      */
extern char dc[MAXSTRING];               /* name of the dc file              */
extern char blif[MAXSTRING];             /* name of the blif file            */
extern char input_names[MAXSTRING];      /* name of the input terminals      */
extern char output_names[MAXSTRING];     /* name of the output terminals     */

/* boolean global flags           True :                    False :          */ 
extern BOOLEAN LIST;           /* echoes input file         Nothing          */
extern BOOLEAN DEBUG;          /* prints out debug info.    Nothing          */
extern BOOLEAN ISYMB;          /* inputs are symbolic       boolean inputs   */
extern BOOLEAN OSYMB;          /* outputs are symbolic      boolean outputs  */
extern BOOLEAN SHORT;          /* considers inputs only                      */
extern BOOLEAN PRTALL;         /* prints extended information                */
extern BOOLEAN TYPEFR;         /* pla format - see Espresso manual           */
extern BOOLEAN DCARE;          /* input constraints extracted with dcares    */
extern BOOLEAN POW2CONSTR;     /* considers only constraints of cardinality 
                                  power of two                               */
extern BOOLEAN I_GREEDY;       /* vertices-wise bottom-up constraint-clustered
                                  (loglength) heuristic encoding algorithm   */
extern BOOLEAN I_ANNEAL;       /* sim.anneal-based encoding algorithm        */
extern BOOLEAN USER;           /* the fsm is encoded by the user             */
extern BOOLEAN ONEHOT;         /* the fsm is 1-hot encoded                   */
extern BOOLEAN COMPLEMENT;     /* complements in turn each column of the 
                                  otherwise maximizes implicants with empty
				  output */
extern BOOLEAN ZERO_FL;        /* TRUE iff user gives state to code zero */
extern BOOLEAN INIT_FLAG;      /* TRUE iff user gives initial state      */
extern BOOLEAN RANDOM;         /* compares the result with random codings    */
extern BOOLEAN IBITS_DEF;      /* TRUE iff code_length of inputs =
				  log2(minpow2(inputnum)) */

extern BOOLEAN SBITS_DEF;      /* TRUE iff code_length of states =
				  log2(minpow2(statenum)) */
extern BOOLEAN ILB;            /* TRUE iff input terminals are given  */
extern BOOLEAN OB;             /* TRUE iff output terminals are given */
extern BOOLEAN ANALYSIS;       /*  analysis of the current encoding required */
extern BOOLEAN PLA_OUTPUT;     /* if TRUE output two-level minimized fsm */



/* ****************************************************************************
*****************  ADDITION RELATED TO THE EXACT ALGORITHM ********************
***************************************************************************** */

extern BOOLEAN I_EXACT;        /* exact encoding algorithm                   */
extern BOOLEAN NAMES;          /* symbols are assigned names by the user     */
extern BOOLEAN SEL_CAT1;       /* true if last call to next_to_code selected
                                  a constraint of category 1                 */
extern int graph_depth;        /* depth of graph_levels                      */
extern int cart_prod;          /* cardinality of the upper level search tree */
extern int bktup_count;        /* index in the upper level search tree       */
extern int bktup_calls;        /* number of face configurations tried in all */
extern int cube_work;          /* number of codes tried in the current cube  */
extern int total_work;         /* number of codes tried by exact_code in all */

typedef struct constraint_e {

        int weight;
	int card;
	int have_father; /* = 1 (otherwise = 0) iff a father of this constraint
			    at exactly one level above was already found (set
			    in "link_graph") */
        char *relation;  /* relation[i] == '1' iff the i-th symbleme belongs to 
			    this constraint */
	struct constraint_e *next; /* pointer to the next constraint at the same
				      level */
	struct fathers_link *up_ptr; /* pointer to the list of the constraints 
					that include this one */
	struct sons_link *down_ptr; /* pointer to the list of the constraints 
				       that are included by this one */
        struct face *face_ptr; /* pointer to the list of faces assigned to this
				  constraint by "x_code" - the first of the list
				  is the face currently assigned */

} CONSTRAINT_E;

/* links all constraints including a certain one */
typedef struct fathers_link {

        CONSTRAINT_E *constraint_e; /* pointer to an including constraint */
	struct fathers_link *next; /* pointer to the next linker */

} FATHERS_LINK;

/* links all constraints included in a certain one */
typedef struct sons_link {

        CONSTRAINT_E *constraint_e; /* pointer to an included constraint */
	CONSTRAINT_E *codfather_ptr; /* pointer to the father triggering the 
					coding of this son */
	struct sons_link *next; /* pointer to the next linker */

} SONS_LINK;

/* links the (primary) constraints in encoding order */
typedef struct codorder_link {

        CONSTRAINT_E *constraint_e; /* pointer to an included constraint */
	struct codorder_link *right; /* pointer to the right linker */
	struct codorder_link *left; /* pointer to the left linker */

} CODORDER_LINK;

/* data structure representing binary codes */
typedef struct face {

        char *seed_value; /* the seed code used for the constraint pointing 
			     to this face */
        BOOLEAN seed_valid;/* TRUE iff current seed_value is valid */
        char *first_value; /* the first code tried for the constraint pointing 
			      to this face */
        BOOLEAN first_valid;/* TRUE iff current first_value is valid */
        char *cur_value;   /* the current code assigned to the constraint 
			      pointing to this face */
        BOOLEAN code_valid;/* TRUE iff current cur_value is valid */
        int count_index;   /* counting index of cur_value */
        int comb_index;    /* combinations index of cur_value */
	int lexmap_index;  /* lexicographical mapping index of cur_value */
        int dim_index;     /* dimensional index of cur_value */
	int tried_codes; /* numbers of attempts to assign a code to the 
			      constraint pointing to this face */
        int category;      /* type of constraint in backtrack_up */
	int mindim_face;   /* minimum face dimension given the cube dimension */
	int curdim_face;   /* currently assigned dimension of the face */
	int maxdim_face;   /* maximum face dimension given the cube dimension */

} FACE;

extern CONSTRAINT_E **graph_levels; /* array of pointers to the constraints with
                                       the same cardinality */

 
/* ****************************************************************************
*********  ADDITION RELATED TO THE I_HYBRID AND IO_HYBRID ALGORITHMS  *********
**************************************************************************** */

typedef struct implicant {

        char *row;        /* product-term specification */
        int state;        /* state which originated this product-term */
        int attribute;    /* NOTUSED USED PRUNED */
        struct implicant *next; /* pointer to the next implicant in the list */

} IMPLICANT;

extern IMPLICANT *implicant_list; /* list of the implicants found in
                                     symbolic minimization                 */
extern int *select_array;         /* status of state selection in iohybrid */

extern int symbmin_products;      /* upper bound of symbolic minimization  */

extern BOOLEAN I_HYBRID;          /* i_hybrid encoding algorithm            */
extern BOOLEAN IO_HYBRID;         /* io_hybrid encoding algorithm           */
extern BOOLEAN IO_VARIANT;        /* io_variant encoding algorithm          */
extern BOOLEAN OUT_ONLY;          /* output only encoding algorithm         */
extern BOOLEAN OUT_VERIFY;        /* verifies the output covering relations */

/* ************************************************************************* */
/*           DATA STRUCTURES USED FOR THE OUTPUT COVERING EXTENSION          */
/* ************************************************************************* */

extern BOOLEAN OUT_ALL; /* accepts all output covering relations - FALSE by
                           default, when we accept only if the gain of the
                           slice minimization is positive                     */

/* set of the admissable values of the output symbolic function ( next states 
column ) */
typedef struct outsymbol {
    char *element;           /* symbolic name of the next-state */
    int nlab;                /* next-state label (same as given by label.c) */
    int card;                /* cardinality of the set of prod.terms whose next-
				state is the current one */
    int selected;            /* = 1 iff already selected by select_oni() */
    struct outsymbol *next;
} OUTSYMBOL;


/* directed acyclic graph (given by row-lists indexed by an array of pointers) 
   representing the partial order on the set of values of the output symbolic 
   function */
typedef struct order_relation { 
    int index;             /* column index in the order_relation */
    struct order_relation *next;
} ORDER_RELATION;

/* lists of the path-connected pairs of nodes of the partial order graph 
   (the starting vertex is given by the index of the array of pointers) */
typedef struct order_path { 
    int dest;           /* destination node in order_path (integer label) */
    struct order_path *next;
} ORDER_PATH;


extern int *reached;                 /* auxiliary array used in dfs          */
extern OUTSYMBOL *outsymbol_list;    /* list of the next-state symbols       */
extern ORDER_RELATION **order_graph; /* array of pointers to the row-lists of
				        the partial order graph              */
extern ORDER_PATH **path_graph;      /* array of pointers to the row-lists of
				        the graph of the paths               */
extern char **order_array;           /* array of pointers to strings 
                                    representing output covering relations   */
extern int *gain_array;              /* gain_array[i] = j iff covering state i
           with the states of order_array[i] achieves a gain of j implicants */

/* ************************************************************************* */
/*                  DATA STRUCTURES USED FOR THE LOWER BOUND                 */
/* ************************************************************************* */

extern BOOLEAN L_BOUND;           /* the fsm is encoded by the user          */

/* ************************************************************************* */
/*                  DATA STRUCTURES USED FOR SIS                             */
/* ************************************************************************* */

extern BOOLEAN SIS;               /* feature for the SIS version             */
extern BOOLEAN VERBOSE;           /* verbose output statistics               */ 
