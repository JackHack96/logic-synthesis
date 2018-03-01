/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/espresso/espresso.h,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2004/03/13 22:25:21 $
 *
 */
/*
 *  espresso.h -- header file for Espresso-mv
 */

#include "ansi.h"
#include <stdio.h>
#include "sparse.h"
#include "mincov.h"
#include "util.h"

#ifdef IBM_WATC
#define void int
#include "short.h"
#endif

#ifdef IBMPC		/* set default options for IBM/PC */
#define NO_INLINE
#define BPI 16
#endif

/*-----THIS USED TO BE set.h----- */

/*
 *  set.h -- definitions for packed arrays of bits
 *
 *   This header file describes the data structures which comprise a
 *   facility for efficiently implementing packed arrays of bits
 *   (otherwise known as sets, cf. Pascal).
 *
 *   A set is a vector of bits and is implemented here as an array of
 *   unsigned integers.  The low order bits of set[0] give the index of
 *   the last word of set data.  The higher order bits of set[0] are
 *   used to store data associated with the set.  The set data is
 *   contained in elements set[1] ... set[LOOP(set)] as a packed bit
 *   array.
 *
 *   A family of sets is a two-dimensional matrix of bits and is
 *   implemented with the data type "set_family".
 *
 *   BPI == 32 and BPI == 16 have been tested and work.
 */


/* Define host machine characteristics of "unsigned int" */
#ifndef BPI
#define BPI             32              /* # bits per integer */
#endif

#if BPI == 32
#define LOGBPI          5               /* log(BPI)/log(2) */
#else
#define LOGBPI          4               /* log(BPI)/log(2) */
#endif

/* Define the set type */
typedef unsigned int *pset;

/* Define the set family type -- an array of sets */
typedef struct set_family {
    int wsize;                  /* Size of each set in 'ints' */
    int sf_size;                /* User declared set size */
    int capacity;               /* Number of sets allocated */
    int count;                  /* The number of sets in the family */
    int active_count;           /* Number of "active" sets */
    pset data;                  /* Pointer to the set data */
    struct set_family *next;    /* For garbage collection */
} set_family_t, *pset_family;

/* Macros to set and test single elements */
#define WHICH_WORD(element)     (((element) >> LOGBPI) + 1)
#define WHICH_BIT(element)      ((element) & (BPI-1))

/* # of ints needed to allocate a set with "size" elements */
#if BPI == 32
#define SET_SIZE(size)          ((size) <= BPI ? 2 : (WHICH_WORD((size)-1) + 1))
#else
#define SET_SIZE(size)          ((size) <= BPI ? 3 : (WHICH_WORD((size)-1) + 2))
#endif

/*
 *  Three fields are maintained in the first word of the set
 *      LOOP is the index of the last word used for set data
 *      LOOPCOPY is the index of the last word in the set
 *      SIZE is available for general use (e.g., recording # elements in set)
 *      NELEM retrieves the number of elements in the set
 */
#define LOOP(set)               (set[0] & 0x03ff)
#define PUTLOOP(set, i)         (set[0] &= ~0x03ff, set[0] |= (i))
#if BPI == 32
#define LOOPCOPY(set)           LOOP(set)
#define SIZE(set)               (set[0] >> 16)
#define PUTSIZE(set, size)      (set[0] &= 0xffff, set[0] |= ((size) << 16))
#else
#define LOOPCOPY(set)           (LOOP(set) + 1)
#define SIZE(set)               (set[LOOP(set)+1])
#define PUTSIZE(set, size)      ((set[LOOP(set)+1]) = (size))
#endif

#define NELEM(set)		(BPI * LOOP(set))
#define LOOPINIT(size)		((size <= BPI) ? 1 : WHICH_WORD((size)-1))

/*
 *      FLAGS store general information about the set
 */
#define SET(set, flag)          (set[0] |= (flag))
#define RESET(set, flag)        (set[0] &= ~ (flag))
#define TESTP(set, flag)        (set[0] & (flag))

/* Flag definitions are ... */
#define PRIME           0x8000          /* cube is prime */
#define NONESSEN        0x4000          /* cube cannot be essential prime */
#define ACTIVE          0x2000          /* cube is still active */
#define REDUND          0x1000          /* cube is redundant(at this point) */
#define COVERED         0x0800          /* cube has been covered */
#define RELESSEN        0x0400          /* cube is relatively essential */

/* Most efficient way to look at all members of a set family */
#define foreach_set(R, last, p)\
    for(p=R->data,last=p+R->count*R->wsize;p<last;p+=R->wsize)
#define foreach_remaining_set(R, last, pfirst, p)\
    for(p=pfirst+R->wsize,last=R->data+R->count*R->wsize;p<last;p+=R->wsize)
#define foreach_active_set(R, last, p)\
    foreach_set(R,last,p) if (TESTP(p, ACTIVE))

/* Another way that also keeps the index of the current set member in i */
#define foreachi_set(R, i, p)\
    for(p=R->data,i=0;i<R->count;p+=R->wsize,i++)
#define foreachi_active_set(R, i, p)\
    foreachi_set(R,i,p) if (TESTP(p, ACTIVE))

/* Looping over all elements in a set:
 *      foreach_set_element(pset p, int i, unsigned val, int base) {
 *		.
 *		.
 *		.
 *      }
 */
#define foreach_set_element(p, i, val, base) 		\
    for(i = LOOP(p); i > 0; )				\
	for(val = p[i], base = --i << LOGBPI; val != 0; base++, val >>= 1)  \
	    if (val & 1)

/* Return a pointer to a given member of a set family */
#define GETSET(family, index)   ((family)->data + (family)->wsize * (index))

/* Allocate and deallocate sets */
#define set_new(size)	set_clear(ALLOC(unsigned int, SET_SIZE(size)), size)
#define set_full(size)	set_fill(ALLOC(unsigned int, SET_SIZE(size)), size)
#define set_save(r)	set_copy(ALLOC(unsigned int, SET_SIZE(NELEM(r))), r)
#define set_free(r)	FREE(r)

/* Check for set membership, remove set element and insert set element */
#define is_in_set(set, e)       (set[WHICH_WORD(e)] & (1 << WHICH_BIT(e)))
#define set_remove(set, e)      (set[WHICH_WORD(e)] &= ~ (1 << WHICH_BIT(e)))
#define set_insert(set, e)      (set[WHICH_WORD(e)] |= 1 << WHICH_BIT(e))

/* Inline code substitution for those places that REALLY need it on a VAX */
#ifdef NO_INLINE
#define INLINEset_copy(r, a)		(void) set_copy(r,a)
#define INLINEset_clear(r, size)	(void) set_clear(r, size)
#define INLINEset_fill(r, size)		(void) set_fill(r, size)
#define INLINEset_and(r, a, b)		(void) set_and(r, a, b)
#define INLINEset_or(r, a, b)		(void) set_or(r, a, b)
#define INLINEset_diff(r, a, b)		(void) set_diff(r, a, b)
#define INLINEset_ndiff(r, a, b, f)	(void) set_ndiff(r, a, b, f)
#define INLINEset_xor(r, a, b)		(void) set_xor(r, a, b)
#define INLINEset_xnor(r, a, b, f)	(void) set_xnor(r, a, b, f)
#define INLINEset_merge(r, a, b, mask)	(void) set_merge(r, a, b, mask)
#define INLINEsetp_implies(a, b, when_false)	\
    if (! setp_implies(a,b)) when_false
#define INLINEsetp_disjoint(a, b, when_false)	\
    if (! setp_disjoint(a,b)) when_false
#define INLINEsetp_equal(a, b, when_false)	\
    if (! setp_equal(a,b)) when_false

#else

#define INLINEset_copy(r, a)\
    {register int i_=LOOPCOPY(a); do r[i_]=a[i_]; while (--i_>=0);}
#define INLINEset_clear(r, size)\
    {register int i_=LOOPINIT(size); *r=i_; do r[i_] = 0; while (--i_ > 0);}
#define INLINEset_fill(r, size)\
    {register int i_=LOOPINIT(size); *r=i_; \
    r[i_]=((unsigned int)(~0))>>(i_*BPI-size); while(--i_>0) r[i_]=~0;}
#define INLINEset_and(r, a, b)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = a[i_] & b[i_]; while (--i_>0);}
#define INLINEset_or(r, a, b)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = a[i_] | b[i_]; while (--i_>0);}
#define INLINEset_diff(r, a, b)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = a[i_] & ~ b[i_]; while (--i_>0);}
#define INLINEset_ndiff(r, a, b, fullset)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = fullset[i_] & (a[i_] | ~ b[i_]); while (--i_>0);}
#ifdef IBM_WATC
#define INLINEset_xor(r, a, b)		(void) set_xor(r, a, b)
#define INLINEset_xnor(r, a, b, f)	(void) set_xnor(r, a, b, f)
#else
#define INLINEset_xor(r, a, b)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = a[i_] ^ b[i_]; while (--i_>0);}
#define INLINEset_xnor(r, a, b, fullset)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = fullset[i_] & ~ (a[i_] ^ b[i_]); while (--i_>0);}
#endif
#define INLINEset_merge(r, a, b, mask)\
    {register int i_=LOOP(a); PUTLOOP(r,i_);\
    do r[i_] = (a[i_]&mask[i_]) | (b[i_]&~mask[i_]); while (--i_>0);}
#define INLINEsetp_implies(a, b, when_false)\
    {register int i_=LOOP(a); do if (a[i_]&~b[i_]) break; while (--i_>0);\
    if (i_ != 0) when_false;}
#define INLINEsetp_disjoint(a, b, when_false)\
    {register int i_=LOOP(a); do if (a[i_]&b[i_]) break; while (--i_>0);\
    if (i_ != 0) when_false;}
#define INLINEsetp_equal(a, b, when_false)\
    {register int i_=LOOP(a); do if (a[i_]!=b[i_]) break; while (--i_>0);\
    if (i_ != 0) when_false;}

#endif	/* NO_INLINE */

#if BPI == 32
#define count_ones(v)\
    (bit_count[v & 255] + bit_count[(v >> 8) & 255]\
    + bit_count[(v >> 16) & 255] + bit_count[(v >> 24) & 255])
#else
#define count_ones(v)   (bit_count[v & 255] + bit_count[(v >> 8) & 255])
#endif

/* Table for efficient bit counting */
extern int bit_count[256];
/*----- END OF set.h ----- */

/* Define a boolean type */
#define bool	int
#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE 	1
#endif
#define MAYBE	2
#define print_bool(x) ((x) == 0 ? "FALSE" : ((x) == 1 ? "TRUE" : "MAYBE"))

/* Map many cube/cover types/routines into equivalent set types/routines */
#define pcube                   pset
#define new_cube()              set_new(cube.size)
#define free_cube(r)            set_free(r)
#define pcover                  pset_family
#define new_cover(i)            sf_new(i, cube.size)
#define free_cover(r)           sf_free(r)
#define free_cubelist(T)        FREE(T[0]); FREE(T);


/* cost_t describes the cost of a cover */
typedef struct cost_struct {
    int cubes;			/* number of cubes in the cover */
    int in;			/* transistor count, binary-valued variables */
    int out;			/* transistor count, output part */
    int mv;			/* transistor count, multiple-valued vars */
    int total;			/* total number of transistors */
    int primes;			/* number of prime cubes */
} cost_t, *pcost;


/* pair_t describes bit-paired variables */
typedef struct pair_struct {
    int cnt;
    int *var1;
    int *var2;
} pair_t, *ppair;


/* symbolic_list_t describes a single ".symbolic" line */
typedef struct symbolic_list_struct {
    int variable;
    int pos;
    struct symbolic_list_struct *next;
} symbolic_list_t;


/* symbolic_list_t describes a single ".symbolic" line */
typedef struct symbolic_label_struct {
    char *label;
    struct symbolic_label_struct *next;
} symbolic_label_t;


/* symbolic_t describes a linked list of ".symbolic" lines */
typedef struct symbolic_struct {
    symbolic_list_t *symbolic_list;	/* linked list of items */
    int symbolic_list_length;		/* length of symbolic_list list */
    symbolic_label_t *symbolic_label;	/* linked list of new names */
    int symbolic_label_length;		/* length of symbolic_label list */
    struct symbolic_struct *next;
} symbolic_t;


/* PLA_t stores the logical representation of a PLA */
typedef struct {
    pcover F, D, R;		/* on-set, off-set and dc-set */
    char *filename;             /* filename */
    int pla_type;               /* logical PLA format */
    pcube phase;                /* phase to split into on-set and off-set */
    ppair pair;                 /* how to pair variables */
    char **label;		/* labels for the columns */
    symbolic_t *symbolic;	/* allow binary->symbolic mapping */
    symbolic_t *symbolic_output;/* allow symbolic output mapping */
} PLA_t, *pPLA;

#define equal(a,b)      (strcmp(a,b) == 0)

/* This is a hack which I wish I hadn't done, but too painful to change */
#define CUBELISTSIZE(T)         (((pcube *) T[1] - T) - 3)

/* For documentation purposes */
#define IN
#define OUT
#define INOUT

/* The pla_type field describes the input and output format of the PLA */
#define F_type          1
#define D_type          2
#define R_type          4
#define PLEASURE_type   8               /* output format */
#define EQNTOTT_type    16              /* output format algebraic eqns */
#define KISS_type	128		/* output format kiss */
#define CONSTRAINTS_type	256	/* output the constraints (numeric) */
#define SYMBOLIC_CONSTRAINTS_type 512	/* output the constraints (symbolic) */
#define FD_type (F_type | D_type)
#define FR_type (F_type | R_type)
#define DR_type (D_type | R_type)
#define FDR_type (F_type | D_type | R_type)

/* Definitions for the debug variable */
#define COMPL           0x0001
#define ESSEN           0x0002
#define EXPAND          0x0004
#define EXPAND1         0x0008
#define GASP            0x0010
#define IRRED           0x0020
#define REDUCE          0x0040
#define REDUCE1         0x0080
#define SPARSE          0x0100
#define TAUT            0x0200
#define EXACT           0x0400
#define MINCOV          0x0800
#define MINCOV1         0x1000
#define SHARP           0x2000
#define IRRED1		0x4000

#define ESPRESSO_VERSION\
    "UC Berkeley, Espresso Version #2.3, Release date 01/31/88"

/* Define constants used for recording program statistics */
#define TIME_COUNT      16
#define READ_TIME       0
#define COMPL_TIME      1
#define ONSET_TIME	2
#define ESSEN_TIME      3
#define EXPAND_TIME     4
#define IRRED_TIME      5
#define REDUCE_TIME     6
#define GEXPAND_TIME    7
#define GIRRED_TIME     8
#define GREDUCE_TIME    9
#define PRIMES_TIME     10
#define MINCOV_TIME	11
#define MV_REDUCE_TIME  12
#define RAISE_IN_TIME   13
#define VERIFY_TIME     14
#define WRITE_TIME	15


/* For those who like to think about PLAs, macros to get at inputs/outputs */
#define NUMINPUTS       cube.num_binary_vars
#define NUMOUTPUTS      cube.part_size[cube.num_vars - 1]

#define POSITIVE_PHASE(pos)\
    (is_in_set(PLA->phase, cube.first_part[cube.output]+pos) != 0)

#define INLABEL(var)    PLA->label[cube.first_part[var] + 1]
#define OUTLABEL(pos)   PLA->label[cube.first_part[cube.output] + pos]

#define GETINPUT(c, pos)\
    ((c[WHICH_WORD(2*pos)] >> WHICH_BIT(2*pos)) & 3)
#define GETOUTPUT(c, pos)\
    (is_in_set(c, cube.first_part[cube.output] + pos) != 0)

#define PUTINPUT(c, pos, value)\
    c[WHICH_WORD(2*pos)] = (c[WHICH_WORD(2*pos)] & ~(3 << WHICH_BIT(2*pos)))\
		| (value << WHICH_BIT(2*pos))
#define PUTOUTPUT(c, pos, value)\
    c[WHICH_WORD(pos)] = (c[WHICH_WORD(pos)] & ~(1 << WHICH_BIT(pos)))\
		| (value << WHICH_BIT(pos))

#define TWO     3
#define DASH    3
#define ONE     2
#define ZERO    1


#define EXEC(fct, name, S)\
    {long t=ptime();fct;if(trace)print_trace(S,name,ptime()-t);}
#define EXEC_S(fct, name, S)\
    {long t=ptime();fct;if(summary)print_trace(S,name,ptime()-t);}
#define EXECUTE(fct,i,S,cost)\
    {long t=ptime();fct;totals(t,i,S,&(cost));}

/*
 *    Global Variable Declarations
 */

extern unsigned int debug;              /* debug parameter */
extern bool verbose_debug;              /* -v:  whether to print a lot */
extern char *total_name[TIME_COUNT];    /* basic function names */
extern long total_time[TIME_COUNT];     /* time spent in basic fcts */
extern int total_calls[TIME_COUNT];     /* # calls to each fct */

extern bool echo_comments;		/* turned off by -eat option */
extern bool echo_unknown_commands;	/* always true ?? */
extern bool force_irredundant;          /* -nirr command line option */
extern bool skip_make_sparse;
extern bool kiss;                       /* -kiss command line option */
extern bool pos;                        /* -pos command line option */
extern bool print_solution;             /* -x command line option */
extern bool recompute_onset;            /* -onset command line option */
extern bool remove_essential;           /* -ness command line option */
extern bool single_expand;              /* -fast command line option */
extern bool summary;                    /* -s command line option */
extern bool trace;                      /* -t command line option */
extern bool unwrap_onset;               /* -nunwrap command line option */
extern bool use_random_order;		/* -random command line option */
extern bool use_super_gasp;		/* -strong command line option */
extern char *filename;			/* filename PLA was read from */
extern bool debug_exact_minimization;   /* dumps info for -do exact */


/*
 *  pla_types are the input and output types for reading/writing a PLA
 */
struct pla_types_struct {
    char *key;
    int value;
};


/*
 *  The cube structure is a global structure which contains information
 *  on how a set maps into a cube -- i.e., number of parts per variable,
 *  number of variables, etc.  Also, many fields are pre-computed to
 *  speed up various primitive operations.
 */
#define CUBE_TEMP       10

struct cube_struct {
    int size;                   /* set size of a cube */
    int num_vars;               /* number of variables in a cube */
    int num_binary_vars;        /* number of binary variables */
    int *first_part;            /* first element of each variable */
    int *last_part;             /* first element of each variable */
    int *part_size;             /* number of elements in each variable */
    int *first_word;            /* first word for each variable */
    int *last_word;             /* last word for each variable */
    pset binary_mask;           /* Mask to extract binary variables */
    pset mv_mask;               /* mask to get mv parts */
    pset *var_mask;             /* mask to extract a variable */
    pset *temp;                 /* an array of temporary sets */
    pset fullset;               /* a full cube */
    pset emptyset;              /* an empty cube */
    unsigned int inmask;        /* mask to get odd word of binary part */
    int inword;                 /* which word number for above */
    int *sparse;                /* should this variable be sparse? */
    int num_mv_vars;            /* number of multiple-valued variables */
    int output;                 /* which variable is "output" (-1 if none) */
};

struct cdata_struct {
    int *part_zeros;            /* count of zeros for each element */
    int *var_zeros;             /* count of zeros for each variable */
    int *parts_active;          /* number of "active" parts for each var */
    bool *is_unate;             /* indicates given var is unate */
    int vars_active;            /* number of "active" variables */
    int vars_unate;             /* number of unate variables */
    int best;                   /* best "binate" variable */
};


extern struct pla_types_struct pla_types[];
extern struct cube_struct cube, temp_cube_save;
extern struct cdata_struct cdata, temp_cdata_save;

#ifdef lint
#define DISJOINT 0x5555
#else
#if BPI == 32
#define DISJOINT 0x55555555
#else
#define DISJOINT 0x5555
#endif
#endif

/* function declarations */

typedef int (*ESP_PFI)();

/* cofactor.c */	EXTERN int binate_split_select ARGS((pcube *, pcube,
   pcube, int));
/* cofactor.c */	EXTERN pcover cubeunlist ARGS((pcube *));
/* cofactor.c */	EXTERN pcube *cofactor ARGS((pcube *, pcube));
/* cofactor.c */	EXTERN pcube *cube1list ARGS((pcover));
/* cofactor.c */	EXTERN pcube *cube2list ARGS((pcover, pcover));
/* cofactor.c */	EXTERN pcube *cube3list ARGS((pcover, pcover, pcover));
/* cofactor.c */	EXTERN pcube *scofactor ARGS((pcube *, pcube, int));
/* cofactor.c */	EXTERN void massive_count ARGS((pcube *));
/* compl.c */	EXTERN pcover complement ARGS((pcube *));
/* compl.c */	EXTERN pcover simplify ARGS((pcube *));
/* compl.c */	EXTERN void simp_comp ARGS((pcube *, pcover *, pcover *));
/* contain.c */	EXTERN int d1_rm_equal ARGS((pset *, ESP_PFI));
/* contain.c */	EXTERN int rm2_contain ARGS((pset *, pset *));
/* contain.c */	EXTERN int rm2_equal ARGS((pset *, pset *, pset *, ESP_PFI));
/* contain.c */	EXTERN int rm_contain ARGS((pset *));
/* contain.c */	EXTERN int rm_equal ARGS((pset *, ESP_PFI));
/* contain.c */	EXTERN int rm_rev_contain ARGS((pset *));
/* contain.c */	EXTERN pset *sf_list ARGS((pset_family));
/* contain.c */	EXTERN pset *sf_sort ARGS((pset_family, ESP_PFI));
/* contain.c */	EXTERN pset_family d1merge ARGS((pset_family, int));
/* contain.c */	EXTERN pset_family dist_merge ARGS((pset_family, pset));
/* contain.c */	EXTERN pset_family sf_contain ARGS((pset_family));
/* contain.c */	EXTERN pset_family sf_dupl ARGS((pset_family));
/* contain.c */	EXTERN pset_family sf_ind_contain ARGS((pset_family, 
   int *));
/* contain.c */	EXTERN pset_family sf_ind_unlist ARGS((pset *, int, int,
   int *, pset));
/* contain.c */	EXTERN pset_family sf_merge ARGS((pset *, pset *, pset *,
   int, int));
/* contain.c */	EXTERN pset_family sf_rev_contain ARGS((pset_family));
/* contain.c */	EXTERN pset_family sf_union ARGS((pset_family, pset_family));
/* contain.c */	EXTERN pset_family sf_unlist ARGS((pset *, int, int));
/* cubestr.c */	EXTERN void cube_setup ARGS(());
/* cubestr.c */	EXTERN void restore_cube_struct ARGS(());
/* cubestr.c */	EXTERN void save_cube_struct ARGS(());
/* cubestr.c */	EXTERN void setdown_cube ARGS(());
/* cvrin.c */	EXTERN int PLA_labels ARGS((pPLA));
/* cvrin.c */	EXTERN char *get_word ARGS((FILE *, char *));
/* cvrin.c */	EXTERN int label_index ARGS((pPLA, char *, int *, int *));
/* cvrin.c */	EXTERN int read_pla ARGS((FILE *, bool, bool, int, pPLA *));
/* cvrin.c */	EXTERN int read_symbolic ARGS((FILE *, pPLA, char *, symbolic_t **));
/* cvrin.c */	EXTERN pPLA new_PLA ARGS(());
/* cvrin.c */	EXTERN void PLA_summary ARGS((pPLA));
/* cvrin.c */	EXTERN void free_PLA ARGS((pPLA));
/* cvrin.c */	EXTERN void parse_pla ARGS((FILE *, pPLA));
/* cvrin.c */	EXTERN void read_cube ARGS((FILE *, pPLA));
/* cvrin.c */	EXTERN void skip_line ARGS((FILE *, FILE *, bool));
/* cvrm.c */	EXTERN int foreach_output_function ARGS((pPLA, ESP_PFI, ESP_PFI ));
/* cvrm.c */	EXTERN int cubelist_partition ARGS((pcube *, pcube **, pcube **,
   unsigned int));
/* cvrm.c */	EXTERN int so_both_do_espresso ARGS((pPLA, int));
/* cvrm.c */	EXTERN int so_both_do_exact ARGS((pPLA, int));
/* cvrm.c */	EXTERN int so_both_save ARGS((pPLA, int));
/* cvrm.c */	EXTERN int so_do_espresso ARGS((pPLA, int));
/* cvrm.c */	EXTERN int so_do_exact ARGS((pPLA, int));
/* cvrm.c */	EXTERN int so_save ARGS((pPLA, int));
/* cvrm.c */	EXTERN pcover cof_output ARGS((pcover, int));
/* cvrm.c */	EXTERN pcover lex_sort ARGS((pcover));
/* cvrm.c */	EXTERN pcover mini_sort ARGS((pcover, ESP_PFI));
/* cvrm.c */	EXTERN pcover random_order ARGS((pcover));
/* cvrm.c */	EXTERN pcover size_sort ARGS((pcover));
/* cvrm.c */	EXTERN pcover sort_reduce ARGS((pcover));
/* cvrm.c */	EXTERN pcover uncof_output ARGS((pcover, int));
/* cvrm.c */	EXTERN pcover unravel ARGS((pcover, int));
/* cvrm.c */	EXTERN pcover unravel_range ARGS((pcover, int, int));
/* cvrm.c */	EXTERN void so_both_espresso ARGS((pPLA, int));
/* cvrm.c */	EXTERN void so_espresso ARGS((pPLA, int));
/* cvrmisc.c */	EXTERN char *fmt_cost ARGS((pcost));
/* cvrmisc.c */	EXTERN char *print_cost ARGS((pcover));
/* cvrmisc.c */	EXTERN void copy_cost ARGS((pcost, pcost));
/* cvrmisc.c */	EXTERN void cover_cost ARGS((pcover, pcost));
/* cvrmisc.c */	EXTERN void fatal ARGS((char *));
/* cvrmisc.c */	EXTERN void print_trace ARGS((pcover, char *, long));
/* cvrmisc.c */	EXTERN void size_stamp ARGS((pcover, char *));
/* cvrmisc.c */	EXTERN void totals ARGS((long, int, pcover, pcost));
/* cvrout.c */	EXTERN char *fmt_cube ARGS((pcube, char *, char *));
/* cvrout.c */	EXTERN char *fmt_expanded_cube ARGS(());
/* cvrout.c */	EXTERN char *pc1 ARGS((pcube));
/* cvrout.c */	EXTERN char *pc2 ARGS((pcube));
/* cvrout.c */	EXTERN char *pc3 ARGS((pcube));
/* cvrout.c */	EXTERN int makeup_labels ARGS((pPLA));
/* cvrout.c */	EXTERN int kiss_output ARGS((FILE *, pPLA));
/* cvrout.c */	EXTERN int kiss_print_cube ARGS((FILE *, pPLA, pcube, char *));
/* cvrout.c */	EXTERN int output_symbolic_constraints ARGS((FILE *, pPLA, int));
/* cvrout.c */	EXTERN void cprint ARGS((pcover));
/* cvrout.c */	EXTERN void debug1_print ARGS((pcover, char *, int));
/* cvrout.c */	EXTERN void sf_debug_print ARGS((pcube *, char *, int));
/* cvrout.c */	EXTERN void eqn_output ARGS((pPLA));
/* cvrout.c */	EXTERN void fpr_header ARGS((FILE *, pPLA, int));
/* cvrout.c */	EXTERN void fprint_pla ARGS((FILE *, pPLA, int));
/* cvrout.c */	EXTERN void pls_group ARGS((pPLA, FILE *));
/* cvrout.c */	EXTERN void pls_label ARGS((pPLA, FILE *));
/* cvrout.c */	EXTERN void pls_output ARGS((pPLA));
/* cvrout.c */	EXTERN void print_cube ARGS((FILE *, pcube, char *));
/* cvrout.c */	EXTERN void print_expanded_cube ARGS((FILE *, pcube, pcube));
/* cvrout.c */	EXTERN void debug_print ARGS((pcube *, char *, int));
/* equiv.c */	EXTERN int  find_equiv_outputs ARGS((pPLA));
/* equiv.c */	EXTERN int check_equiv ARGS((pcover, pcover));
/* espresso.c */	EXTERN pcover espresso ARGS((pcover, pcover, pcover));
/* essen.c */	EXTERN bool essen_cube ARGS((pcover, pcover, pcube));
/* essen.c */	EXTERN pcover cb_consensus ARGS((pcover, pcube));
/* essen.c */	EXTERN pcover cb_consensus_dist0 ARGS((pcover, pcube, pcube));
/* essen.c */	EXTERN pcover essential ARGS((pcover *, pcover *));
/* exact.c */	EXTERN pcover minimize_exact ARGS((pcover, pcover, pcover,
   int));
/* exact.c */	EXTERN pcover minimize_exact_literals ARGS((pcover, pcover,
   pcover, int));
/* expand.c */	EXTERN bool feasibly_covered ARGS((pcover, pcube, pcube,
   pcube));
/* expand.c */	EXTERN int most_frequent ARGS((pcover, pcube));
/* expand.c */	EXTERN pcover all_primes ARGS((pcover, pcover));
/* expand.c */	EXTERN pcover expand ARGS((pcover, pcover, bool));
/* expand.c */	EXTERN pcover find_all_primes ARGS((pcover, pcube, pcube));
/* expand.c */	EXTERN void elim_lowering ARGS((pcover, pcover, pcube, pcube));
/* expand.c */	EXTERN void essen_parts ARGS((pcover, pcover, pcube, pcube));
/* expand.c */	EXTERN void essen_raising ARGS((pcover, pcube, pcube));
/* expand.c */	EXTERN void expand1 ARGS((pcover, pcover, pcube,
   pcube, pcube, pcube, pcube,
   int *, pcube));
/* expand.c */	EXTERN void mincov ARGS((pcover, pcube, pcube));
/* expand.c */	EXTERN void select_feasible ARGS((pcover, pcover,
   pcube, pcube, pcube, int *));
/* expand.c */	EXTERN void setup_BB_CC ARGS((pcover, pcover));
/* gasp.c */	EXTERN pcover expand_gasp ARGS((pcover, pcover, pcover, pcover));
/* gasp.c */	EXTERN pcover irred_gasp ARGS((pcover, pcover, pcover));
/* gasp.c */	EXTERN pcover last_gasp ARGS((pcover, pcover, pcover,
   cost_t *cost));
/* gasp.c */	EXTERN pcover super_gasp ARGS((pcover, pcover, pcover,
   cost_t *cost));
/* gasp.c */	EXTERN void expand1_gasp ARGS((pcover, pcover, pcover,
   pcover, int, pcover *));
/* hack.c */	EXTERN int find_dc_inputs ARGS((pPLA, symbolic_list_t *, int, int, pcover *, pcover *));
/* hack.c */	EXTERN int find_inputs ARGS((pcover, pPLA, symbolic_list_t *,
   int, int, pcover *, pcover *));
/* hack.c */	EXTERN int form_bitvector ARGS((pset, int, int,
   symbolic_list_t *));
/* hack.c */	EXTERN int map_dcset ARGS((pPLA));
/* hack.c */	EXTERN int map_output_symbolic ARGS((pPLA));
/* hack.c */	EXTERN int map_symbolic ARGS((pPLA));
/* hack.c */	EXTERN pcover map_symbolic_cover ARGS((pcover,
   symbolic_list_t *, int));
/* hack.c */	EXTERN int symbolic_hack_labels ARGS((pPLA, symbolic_t*,
   pset, int, int, int));
/* irred.c */	EXTERN bool cube_is_covered ARGS((pcube *, pcube));
/* irred.c */	EXTERN bool taut_special_cases ARGS((pcube *));
/* irred.c */	EXTERN bool tautology ARGS((pcube *));
/* irred.c */	EXTERN pcover irredundant ARGS((pcover, pcover));
/* irred.c */	EXTERN void mark_irredundant ARGS((pcover, pcover));
/* irred.c */	EXTERN void irred_split_cover ARGS((pcover, pcover,
   pcover *, pcover *, pcover *));
/* irred.c */	EXTERN sm_matrix *irred_derive_table ARGS((pcover, pcover,
   pcover));
/* map.c */	EXTERN pset minterms ARGS((pcover));
/* map.c */	EXTERN void explode ARGS((int, int));
/* map.c */	EXTERN void map ARGS((pcover));
/* opo.c */	EXTERN int output_phase_setup ARGS((pPLA, int));
/* opo.c */	EXTERN pPLA set_phase ARGS((pPLA));
/* opo.c */	EXTERN pcover opo ARGS((pcube, pcover, pcover, pcover,
   int));
/* opo.c */	EXTERN pcube find_phase ARGS((pPLA, int, pcube));
/* opo.c */	EXTERN pset_family find_covers ARGS((pcover, pcover, pset, int));
/* opo.c */	EXTERN pset_family form_cover_table ARGS((pcover, pcover,
   pset, int, int));
/* opo.c */	EXTERN pset_family opo_leaf ARGS((pcover, pset, int, int));
/* opo.c */	EXTERN pset_family opo_recur ARGS((pcover, pcover, pcube, int,
   int, int));
/* opo.c */	EXTERN void opoall ARGS((pPLA, int, int, int));
/* opo.c */	EXTERN void phase_assignment ARGS((pPLA, int));
/* opo.c */	EXTERN void repeated_phase_assignment ARGS((pPLA));
/* pair.c */	EXTERN int generate_all_pairs ARGS((ppair, int, pset, ESP_PFI));
/* pair.c */	EXTERN int **find_pairing_cost ARGS((pPLA, int));
/* pair.c */	EXTERN int find_best_cost ARGS((ppair));
/* pair.c */	EXTERN int greedy_best_cost ARGS((int **, ppair *));
/* pair.c */	EXTERN int minimize_pair ARGS((ppair));
/* pair.c */	EXTERN int pair_free ARGS((ppair));
/* pair.c */	EXTERN int pair_all ARGS((pPLA, int));
/* pair.c */	EXTERN pcover delvar ARGS((pcover, bool *));
/* pair.c */	EXTERN pcover pairvar ARGS((pcover, ppair));
/* pair.c */	EXTERN ppair pair_best_cost ARGS((int **));
/* pair.c */	EXTERN ppair pair_new ARGS((int));
/* pair.c */	EXTERN ppair pair_save ARGS((ppair, int));
/* pair.c */	EXTERN int print_pair ARGS((ppair));
/* pair.c */	EXTERN void find_optimal_pairing ARGS((pPLA, int));
/* pair.c */	EXTERN void set_pair ARGS((pPLA));
/* pair.c */	EXTERN void set_pair1 ARGS((pPLA, bool));
/* primes.c */	EXTERN pcover primes_consensus ARGS((pcube *));
/* reduce.c */	EXTERN bool sccc_special_cases ARGS((pcube *, pcube *));
/* reduce.c */	EXTERN pcover reduce ARGS((pcover, pcover));
/* reduce.c */	EXTERN pcube reduce_cube ARGS((pcube *, pcube));
/* reduce.c */	EXTERN pcube sccc ARGS((pcube *));
/* reduce.c */	EXTERN pcube sccc_cube ARGS((pcube, pcube));
/* reduce.c */	EXTERN pcube sccc_merge ARGS((pcube, pcube, pcube, pcube));
/* set.c */	EXTERN bool set_andp ARGS((pset, pset, pset));
/* set.c */	EXTERN bool set_orp ARGS((pset, pset, pset));
/* set.c */	EXTERN bool setp_disjoint ARGS((pset, pset));
/* set.c */	EXTERN bool setp_empty ARGS((pset));
/* set.c */	EXTERN bool setp_equal ARGS((pset, pset));
/* set.c */	EXTERN bool setp_full ARGS((pset, int));
/* set.c */	EXTERN bool setp_implies ARGS((pset, pset));
/* set.c */	EXTERN char *pbv1 ARGS((pset, int));
/* set.c */	EXTERN char *ps1 ARGS((pset));
/* set.c */	EXTERN int *sf_count ARGS((pset_family));
/* set.c */	EXTERN int *sf_count_restricted ARGS((pset_family, pset));
/* set.c */	EXTERN int bit_index ARGS((unsigned int));
/* set.c */	EXTERN int set_dist ARGS((pset, pset));
/* set.c */	EXTERN int set_ord ARGS((pset));
/* set.c */	EXTERN void set_adjcnt ARGS((pset, int *, int));
/* set.c */	EXTERN pset set_and ARGS((pset, pset, pset));
/* set.c */	EXTERN pset set_clear ARGS((pset, int));
/* set.c */	EXTERN pset set_copy ARGS((pset, pset));
/* set.c */	EXTERN pset set_diff ARGS((pset, pset, pset));
/* set.c */	EXTERN pset set_fill ARGS((pset, int));
/* set.c */	EXTERN pset set_merge ARGS((pset, pset, pset, pset));
/* set.c */	EXTERN pset set_or ARGS((pset, pset, pset));
/* set.c */	EXTERN pset set_xor ARGS((pset, pset, pset));
/* set.c */	EXTERN pset sf_and ARGS((pset_family));
/* set.c */	EXTERN pset sf_or ARGS((pset_family));
/* set.c */	EXTERN pset_family sf_active ARGS((pset_family));
/* set.c */	EXTERN pset_family sf_addcol ARGS((pset_family, int, int));
/* set.c */	EXTERN pset_family sf_addset ARGS((pset_family, pset));
/* set.c */	EXTERN pset_family sf_append ARGS((pset_family, pset_family));
/* set.c */	EXTERN pset_family sf_bm_read ARGS((FILE *));
/* set.c */	EXTERN pset_family sf_compress ARGS((pset_family, pset));
/* set.c */	EXTERN pset_family sf_copy ARGS((pset_family, pset_family));
/* set.c */	EXTERN pset_family sf_copy_col ARGS((pset_family, int,
   pset_family, int));
/* set.c */	EXTERN pset_family sf_delc ARGS((pset_family, int, int));
/* set.c */	EXTERN pset_family sf_delcol ARGS((pset_family, int, int));
/* set.c */	EXTERN pset_family sf_inactive ARGS((pset_family));
/* set.c */	EXTERN pset_family sf_join ARGS((pset_family, pset_family));
/* set.c */	EXTERN pset_family sf_new ARGS((int, int));
/* set.c */	EXTERN pset_family sf_permute ARGS((pset_family, int *, int));
/* set.c */	EXTERN pset_family sf_read ARGS((FILE *));
/* set.c */	EXTERN pset_family sf_save ARGS((pset_family));
/* set.c */	EXTERN pset_family sf_transpose ARGS((pset_family));
/* set.c */	EXTERN void set_write ARGS((FILE *, pset));
/* set.c */	EXTERN void sf_bm_print ARGS((pset_family));
/* set.c */	EXTERN void sf_cleanup ARGS(());
/* set.c */	EXTERN void sf_delset ARGS((pset_family, int));
/* set.c */	EXTERN void sf_free ARGS((pset_family));
/* set.c */	EXTERN void sf_print ARGS((pset_family));
/* set.c */	EXTERN void sf_write ARGS((FILE *, pset_family));
/* setc.c */	EXTERN bool ccommon ARGS((pcube, pcube, pcube));
/* setc.c */	EXTERN bool cdist0 ARGS((pcube, pcube));
/* setc.c */	EXTERN bool full_row ARGS((pcube, pcube));
/* setc.c */	EXTERN int ascend ARGS((pset *, pset *));
/* setc.c */	EXTERN int cactive ARGS((pcube));
/* setc.c */	EXTERN int cdist ARGS((pset, pset));
/* setc.c */	EXTERN int cdist01 ARGS((pset, pset));
/* setc.c */	EXTERN int d1_order ARGS((pset *, pset *));
/* setc.c */	EXTERN int desc1 ARGS((pset, pset));
/* setc.c */	EXTERN int descend ARGS((pset *, pset *));
/* setc.c */	EXTERN int lex_order ARGS((pset *, pset *));
/* setc.c */	EXTERN pset force_lower ARGS((pset, pset, pset));
/* setc.c */	EXTERN void consensus ARGS((pcube, pcube, pcube));
/* sharp.c */	EXTERN pcover cb1_dsharp ARGS((pcover, pcube));
/* sharp.c */	EXTERN pcover cb_dsharp ARGS((pcube, pcover));
/* sharp.c */	EXTERN pcover cb_recur_sharp ARGS((pcube, pcover, int, int, int));
/* sharp.c */	EXTERN pcover cb_sharp ARGS((pcube, pcover));
/* sharp.c */	EXTERN pcover cv_dsharp ARGS((pcover, pcover));
/* sharp.c */	EXTERN pcover cv_intersect ARGS((pcover, pcover));
/* sharp.c */	EXTERN pcover cv_sharp ARGS((pcover, pcover));
/* sharp.c */	EXTERN pcover dsharp ARGS((pcube, pcube));
/* sharp.c */	EXTERN pcover make_disjoint ARGS((pcover));
/* sharp.c */	EXTERN pcover sharp ARGS((pcube, pcube));
/* sminterf.c */pset do_sm_minimum_cover ARGS((pset_family));
/* sparse.c */	EXTERN pcover make_sparse ARGS((pcover, pcover, pcover));
/* sparse.c */	EXTERN pcover mv_reduce ARGS((pcover, pcover));
/* unate.c */	EXTERN pcover find_all_minimal_covers_petrick ARGS(());
/* unate.c */	EXTERN pcover map_cover_to_unate ARGS((pcube *));
/* unate.c */	EXTERN pcover map_unate_to_cover ARGS((pset_family));
/* unate.c */	EXTERN pset_family exact_minimum_cover ARGS((pset_family));
/* unate.c */	EXTERN pset_family unate_compl ARGS((pset_family));
/* unate.c */	EXTERN pset_family unate_complement ARGS((pset_family));
/* unate.c */	EXTERN pset_family unate_intersect ARGS((pset_family, pset_family, bool));
/* verify.c */	EXTERN int PLA_permute ARGS((pPLA, pPLA));
/* verify.c */	EXTERN bool PLA_verify ARGS((pPLA, pPLA));
/* verify.c */	EXTERN bool check_consistency ARGS((pPLA));
/* verify.c */	EXTERN bool verify ARGS((pcover, pcover, pcover));
