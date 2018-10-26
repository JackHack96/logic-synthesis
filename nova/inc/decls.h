
/*
 * actual storage for the global variables, see nova.h for descriptions
 * of these variables
 */
int cost_function, num_moves;
CONSTRAINT *inputnet;
CONSTRAINT *statenet;
CONSTRAINT *outputnet;

SYMBLEME *inputs;
SYMBLEME *states;
SYMBLEME *outputs;

CONSTRAINT **levelarray;
CONSTRAINT **cardarray;

HYPERVERTEX *hypervertices;

INPUTTABLE *firstable;
INPUTTABLE *lastable;

int *zeroutput;

int inputnum;
int statenum;
int outputnum;
int inputfield;
int outputfield;

int productnum;
int symblemenum;

int st_codelength;
int inp_codelength;
int out_codelength;

int onehot_products;
int min_inputs;
int min_outputs;
int min_products;
int best_products;
int worst_products;

int first_size;
int best_size;
int rand_trials;
char zero_state[MAXSTRING];
char init_state[MAXSTRING];
char init_code[MAXSTRING];
char file_name[MAXSTRING];
char sh_filename[MAXSTRING];
char temp1[MAXSTRING];
char temp2[MAXSTRING];
char temp3[MAXSTRING];
char temp33[MAXSTRING];
char temp4[MAXSTRING];
char temp5[MAXSTRING];
char temp6[MAXSTRING];
char temp7[MAXSTRING];
char temp81[MAXSTRING];
char temp82[MAXSTRING];
char temp83[MAXSTRING];
char temp9[MAXSTRING];
char temp10[MAXSTRING];
char constraints[MAXSTRING];
char summ[MAXSTRING];
char esp[MAXSTRING];
char dc[MAXSTRING];
char readscript[MAXSTRING];
char blif[MAXSTRING];
char input_names[MAXSTRING];
char output_names[MAXSTRING];

BOOLEAN LIST;
BOOLEAN DEBUG;
BOOLEAN ISYMB;
BOOLEAN OSYMB;
BOOLEAN SHORT;
BOOLEAN PRTALL;
BOOLEAN TYPEFR;
BOOLEAN DCARE;
BOOLEAN POW2CONSTR;
BOOLEAN I_GREEDY;
BOOLEAN I_ANNEAL;
BOOLEAN USER;
BOOLEAN ONEHOT;
BOOLEAN COMPLEMENT;
BOOLEAN ZERO_FL;
BOOLEAN INIT_FLAG;
BOOLEAN RANDOM;
BOOLEAN IBITS_DEF;
BOOLEAN SBITS_DEF;
BOOLEAN ILB;
BOOLEAN OB;
BOOLEAN ANALYSIS;

BOOLEAN I_EXACT;
BOOLEAN NAMES;
BOOLEAN SEL_CAT1;
int graph_depth;
int cart_prod;
int bktup_count;
int bktup_calls;
int cube_work;
int total_work;

CONSTRAINT_E **graph_levels;

IMPLICANT *implicant_list;
int *select_array;

int symbmin_products;

BOOLEAN I_HYBRID;
BOOLEAN IO_HYBRID;
BOOLEAN IO_VARIANT;
BOOLEAN OUT_ONLY;
BOOLEAN OUT_VERIFY;

BOOLEAN OUT_ALL;

int *reached;
OUTSYMBOL *outsymbol_list;
ORDER_RELATION **order_graph;
ORDER_PATH **path_graph;
char **order_array;
int *gain_array;
BOOLEAN L_BOUND;
BOOLEAN SIS;
BOOLEAN VERBOSE;
BOOLEAN PLA_OUTPUT;
