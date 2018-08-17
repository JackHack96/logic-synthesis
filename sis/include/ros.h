
/******************************************************************************
 * This module has been written by Abdul A. Malik                             *
 * Please forward all feedback, comments, bugs etc to:                        *
 *                                                                            *
 * ABDUL A. MALIK                                                             *
 * malik@ic.berkeley.edu                                                      *
 *                                                                            *
 * Department of Electrical Engineering and Computer Sciences                 *
 * University of California at Berkeley                                       *
 * Berkeley, CA 94704.                                                        *
 *                                                                            *
 * Telephone: (415) 642-5048                                                  *
 ******************************************************************************/

#define LEFT 0
#define RIGHT 1
#define NONUNATE 0
#define UNATE 1
#define cdist0bv(p, q, var) \
((GETINPUT(p, var)) & (GETINPUT(q, var)))
#define ROS_ZEROS 20
#define ROS_MEM_SIZE 1000000

/* Node to store intermediate nodes of unate tree */
typedef struct nc_node_struct {
    struct nc_node_struct *child[2];
    pset                  part_cube[2];
    pset                  *cubelist;
    pset                  cof;
    int                   var;
} nc_node_t, *pnc_node;

/* ros_holder to hold ROS */
typedef struct ros_holder_struct {
    pset_family ros;
    pcube       ros_cube;
    int         count;
} ros_holder_t, *pros_holder;


/* Global variables used by routines in ros */
extern pcube *nc_tmp_cube;    /* Temporary cubes to be used in this file */

extern pcover Tree_cover;    /* F union D used to generate the unate tree */

extern int      Max_level;        /* Controls the depth of the part of unate
			           tree that is stored */
extern int      N_level;            /* Some processing to reduce the size of
				  cofactors is done every N_level levels */
extern pnc_node root;        /* Root node of the unate tree */

extern int Num_act_vars;       /* Number of active variables in the cube being 
			          expanded */

extern pcover ROS;        /* Reduced off set */

extern int ROS_count;        /* Number of ROS generated */

extern long ROS_time;        /* Total time for generating ROSs */

extern pcube ROS_cube;        /* Cube for which ROS was computed */

extern int Max_ROS_size;    /* Maximum size of off set during the entire
				   run */
extern int ROS_zeros;        /* Upper bound on the number of zeros in
				   ROS cube */
extern int ROS_max_store;    /* Number of ROS stored at any time */

extern int ROS_max_size_store;    /* Upped limit on the size of ROS that can
                                   be stored for future use */
extern int N_ROS;        /* Keeps track of number of ROS stored so far */

extern pros_holder ROS_array;    /* ROS_array that stores ROS */

extern int OC_count;        /* Number of overexpanded cubes generated */

extern long OC_time;        /* Total time for generating overexpanded
				   cubes */

/* Functions used in ros */

/* ros.c */ extern void get_ROS1();

/* ros.c */ extern pcover get_ROS();

/* ros.c */ extern void init_ROS();

/* ros.c */ extern bool find_correct_ROS();

/* ros.c */ extern void store_ROS();

/* ros.c */ extern void close_ROS();

/* ros.c */ extern void find_overexpanded_cube_dyn();

/* ros.c */ extern bool find_oc_dyn_special_cases();

/* ros.c */ extern pcover setupBB_dyn();

/* ros.c */ extern bool BB_dyn_special_cases();

/* ros.c */ extern void find_overexpanded_cube();

/* ros.c */ extern bool find_oc_special_cases();

/* ros.c */ extern pcover setupBB();

/* ros.c */ extern bool BB_special_cases();

/* ros.c */ extern void get_OC();

/* ros.c */ extern bool partition_ROS();

/* ros.c */ extern pcover multiply2_sf();

/* ros.c */ extern int nc_cubelist_partition();

/* ros.c */ extern void nc_generate_cof_tree();

/* ros.c */ extern pcover nc_compl_cube();

/* ros.c */ extern void nc_free_unate_tree();

/* ros.c */ extern void nc_bin_cof();

/* ros.c */ extern void nc_cof_contain();

/* ros.c */ extern bool cdist0v();

/* ros.c */ extern void nc_rem_unnec_cubes();

/* ros.c */ extern void nc_or_unnec();

/* ros.c */ extern bool nc_is_nec();

/* ros.c */ extern void nc_sf_multiply();

/* ros.c */ extern void nc_rem_noncov_cubes();

/* ros.c */ extern void nc_compl_special_cases();

/* ros.c */ extern pcube *nc_copy_cubelist();

/* ros.c */ extern void nc_setup_tmp_cubes();

/* ros.c */ extern void nc_free_tmp_cubes();
