
/*
 *  Definitions local to package 'fast_extract' go here
 */

/* SHORT and UNSIGNED are now defined in extract.h */

/*  Define types of double cube divisors */
#define D112        0
#define D222        1
#define D223        2
#define D224        3
#define Dother    4

/*  Define different status for weights in divisors */
#define OLD        0
#define NEW        1
#define CHANGED    2

/*  Define different status of doube-cube divisor after extracting 
 *  single-cube divisor
 */
#define NONCHECK        0
#define INBASE        1
#define INBETWEEN    2
#define INDIVISOR    3

#define HANDLE(p)    ((p)->handle)
#define WEIGHT(p)    ((p)->weight)
#define DTYPE(p)     ((p)->dtype)
#define STATUS(p)    ((p)->status)

/*
 * The data structure for the ddivisor_t structure resides in extract.h
 * This structure is used in some of the speed_up code (HACK)...
 */

/*  Define the cell data structure of a double-cube divisor */
typedef struct ddivisor_cell_struct ddivisor_cell_t;
struct ddivisor_cell_struct {
    sm_row     *cube1;        /* the pointer of the first cube */
    sm_row     *cube2;        /* the pointer of the second cube */
    lsHandle   handle1;     /* lsHandle to the list in cube1. */
    lsHandle   handle2;     /* lsHandle to the list in cube2  */
    lsHandle   handle;      /* lsHandle to the corresponding list */
    ddivisor_t *div;      /* the address of the corresponding divisor */
    int        sis_id;           /* the id in sis */
    UNSIGNED   phase;       /* the phase of this doubler-cube divisor */
    UNSIGNED   baselength;  /* the baselength */
};

#define DIVISOR(p)   ((p)->div)


/*  Define the data structure of a single-cube divisor. */
typedef struct single_cube_divisor_struct sdivisor_t;
struct single_cube_divisor_struct {
    SHORT col1;             /* the 1st column number of a single-cube divisor */
    SHORT col2;             /* the 2nd column number of a single-cube divisor */
    SHORT coin;             /* the coincidence of a single-cube divisor */
};

#define COIN(p)      ((p)->coin)


/* Define the data structure of double-cube divisor set. */
typedef struct ddivisor_set_struct ddset_t;
struct ddivisor_set_struct {
    lsList    DDset;          /* storing all double-cube divisors. */
    sm_row    *node_cube_set; /* storing the cubes for each node in sis */
    sm_matrix *D112_set;   /* storing D112 type divisors */
    sm_matrix *D222_set;   /* storing D222 type divisors */
    sm_matrix *D223_set;   /* storing D223 type divisors */
    sm_matrix *D224_set;   /* storing D224 type divisors */
    sm_matrix *Dother_set; /* storing other types divisors */
};

/* Define the data structure of single-cube divisor set. */
typedef struct sdivisor_set_struct sdset_t;
struct sdivisor_set_struct {
    heap_t *heap;     /* storing all single-cube divisor */
    int    index;        /* the largest column number considered so far */
    lsList columns;   /* the cols unconsidered */
    lsList col_set;   /* the cols considered */
};

/* Define the data structure of col_cell */
typedef struct col_cell_struct col_cell_t;
struct col_cell_struct {
    SHORT    num;        /* column number */
    int      length;       /* column length */
    lsHandle handle;  /* lsHandle */
};

/* Define the data structure for each cube in sparse matrix. */
typedef struct sm_cube_cell_struct sm_cube_t;
struct sm_cube_cell_struct {
    lsList   div_set;      /* storing the dd_cells affected by this cube */
    lsHandle cubehandle; /* pointing to the corresponding node_cube_set */
    int      sis_id;          /* the id in sis  */
};


/* com_fx.c */
extern int ONE_PASS;
extern int FX_DELETE;
extern int LENGTH1;
extern int LENGTH2;
extern int DONT_USE_WEIGHT_ZERO;

/* ddivisor.c */
extern int extract_ddivisor();

extern ddivisor_t *check_append();

extern void hash_ddset_table();

extern lsList choose_check_set();

static lsList lookup_ddset_other();

static lsList lookup_ddset();

extern int check_exist();

extern ddivisor_t *gen_ddivisor();

extern int decide_dtype();

extern void clear_row_element();

/* memory allocation and free in ddivsior.c */
extern ddivisor_t *ddivisor_alloc();

extern void ddivisor_free();

extern ddivisor_cell_t *ddivisor_cell_alloc();

extern void ddivisor_cell_free();

extern ddset_t *ddset_alloc_init();

extern void ddset_free();

extern sm_cube_t *sm_cube_alloc();

extern void sm_cube_free();

/* fast_extract.c */
extern void fx_node_to_sm();

extern node_t *fx_sm_to_node();

extern int fast_extract();

extern void calc_weight();

/* sdivisor.c */
static sdivisor_t *sdivisor_alloc();

static void sdivisor_free();

extern sdset_t *sdset_alloc();

extern void sdset_free();

static col_cell_t *col_cell_alloc();

static void col_cell_free();

extern sdivisor_t *extract_sdivisor();

extern sdivisor_t *sdivsisor_alloc();

extern void sdivisor_free();
