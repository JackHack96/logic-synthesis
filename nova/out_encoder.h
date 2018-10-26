
/*#define isdigit(ch) ((ch >= '0') && (ch <= '9'))
#define isalpha(ch) (((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')))*/

#define MAXNODE    220
#define MAXDIM     8+1
#define MAXPOW2DIM 256

#define Z_X '?'
#define NOT_DONE 0
#define DONE 1
#define EOS '\0'
#define FAIL '\0'

typedef struct g_type {
	int num;
        int name;
	struct g_type *next;
} t_graph,*pgraph;

typedef struct dag_t {
	int depth;
	int incard;
    int outcard;
	int out_done;
} t_dag;

typedef struct sol_type {
	int dim;
	char node[MAXNODE] [MAXDIM];
} t_solution;
