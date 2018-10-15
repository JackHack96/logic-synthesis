#define ALLOCSIZE 500
#define LIMIT     5000

/* each dichotomy is a contatenation of two sets - one for the lhs and
 *  one for the rhs */
typedef struct dic_family {
    int  dcapacity;	/* total number of dichotomies */
    int  dcount;	/* number of dichotomies */
    int  dic_size;	/* size of a dic in ints */
    int  dset_elem;	/* number of elements in each set of the dic */
    int  dset_size;	/* size of a set in ints - dic size is twice this */
    pset dic;           /* pointer to first dichotomy */
} dic_family_t;

typedef struct cnf {
    int var1;           /* first variable */
    int var2;           /* second variable */
} cnf_t;

#define lhs_dic(dic) (dic)
#define rhs_dic(dic) ((dic)+LOOPCOPY(dic)+1)

#define GETDIC(family, index)   ((family)->dic + (family)->dic_size*index)

#define MERGE1 1
#define MERGE2 2
extern pset		dic_new ();
extern dic_family_t 	*dic_family_alloc ();
extern dic_family_t     *gen_uniq();
extern dic_family_t     *gen_eqn();
extern sm_matrix        *dic_to_sm();
extern dic_family_t     *reduce_seeds();
