/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/extract/com_ex.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:22 $
 *
 */
#include "sis.h"
#include "extract_int.h"
#include "heap.h"
#include "fast_extract_int.h"

static int com_fast_extract();
static st_table *find_required_time();
static void remove_unwanted_nodes();

int ONE_PASS    = 0;
int FX_DELETE   = 0;
int LENGTH1     = 5;
int LENGTH2     = 5;
int OBJECT_SIZE = 50000;
int DONT_USE_WEIGHT_ZERO = 1;

static int
com_cube_extract(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    lsGen gen;
    sm_matrix *cube_literal_matrix;
    int thres, use_best_subcube, c;
    node_t *node;

    use_best_subcube = 0;
    use_complement = 0;        /* global */
    cube_extract_debug = 0;    /* global */
    thres = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "bcdft:v:")) != EOF) {
    switch(c) {
    case 'b':
        use_best_subcube = 1;
        break;
    case 'c':
        use_complement = 1;
        break;
    case 'd':
        cube_extract_debug = 1;
        break;
    case 'f':
        use_best_subcube = 2;
        break;
    case 't':
        thres = atoi(util_optarg);
        break;
    case 'v':
        cube_extract_debug = atoi(util_optarg);
        break;
    default:
        goto usage;
    }
    }
    if (argc - util_optind != 0) goto usage;
        
    ex_setup_globals(*network, 0);

    /* create the cube-literal matrix */
    cube_literal_matrix = sm_alloc();
    foreach_node(*network, gen, node) {
    switch(node_function(node)) {
    case NODE_PI: 
    case NODE_PO:
    case NODE_0: 
    case NODE_1:
    case NODE_INV:
    case NODE_BUF:
        break;

    case NODE_AND:
    case NODE_OR:
    case NODE_COMPLEX:
        ex_node_to_sm(node, cube_literal_matrix);
        break;
    default:
	;
    }
    }

    /* extract cubes */
    (void) sparse_cube_extract(cube_literal_matrix, thres, use_best_subcube);

    free_value_cells(cube_literal_matrix);
    sm_free(cube_literal_matrix);
    ex_free_globals(0);
    return 0;

usage:
    (void) fprintf(siserr, 
    "usage: gcx [-bcdf] [-t thresh]\n");
    (void) fprintf(siserr, 
    "    -b\t\tuses best subcube (rather than pingpong)\n");
    (void) fprintf(siserr, 
    "    -c\t\tuses complement during division\n");
    (void) fprintf(siserr, 
    "    -d\t\tenables debug\n");
    (void) fprintf(siserr, 
    "    -f\t\tuses factored literals for value\n");
    (void) fprintf(siserr, 
    "    -t thres\tset threshold value for extraction\n");
    return 1;
}

static int
com_kernel_extract(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    sm_matrix *func;
    node_t *node;
    int use_best_subkernel, use_all_kernels, use_overlap, one_pass;
    int need_duplicate, value, thres, c, index;
    lsGen gen;

    one_pass = 0;
    use_all_kernels = 0;
    use_best_subkernel = 0;
    use_complement = 0;        /* global */
    kernel_extract_debug = 0;    /* global */
    use_overlap = 0;
    need_duplicate = 0;
    thres = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "1abcdfot:v:")) != EOF) {
    switch(c) {
    case '1':
        one_pass = 1;
        break;
    case 'a':
        use_all_kernels = 1;
        break;
    case 'b':
        use_best_subkernel = 1;
        break;
    case 'c':
        use_complement = 1;
        break;
    case 'd':
        kernel_extract_debug = 1;
        break;
    case 'f':
        use_best_subkernel = 2;
        break;
    case 'o':
        use_overlap = 1;
        need_duplicate = 1;
        break;
    case 't':
        thres = atoi(util_optarg);
        break;
    case 'v':
        kernel_extract_debug = atoi(util_optarg);
        break;
    default:
        goto usage;
    }
    }

    if (argc - util_optind != 0) goto usage;
        
    do {
    ex_setup_globals(*network, need_duplicate);

    kernel_extract_init();
    foreach_node(*network, gen, node) {
        switch(node_function(node)) {
        case NODE_PI:
        case NODE_PO:
        case NODE_0:
        case NODE_1:
        case NODE_INV:
        case NODE_BUF:
        break;

        case NODE_AND:
        case NODE_OR:
        case NODE_COMPLEX:
        func = sm_alloc();
        ex_node_to_sm(node, func);
        index = nodeindex_indexof(global_node_index, node);
        kernel_extract(func, index, use_all_kernels);
        free_value_cells(func);
        sm_free(func);
        break;
	default:
	;
        }
    }

    if (use_overlap) {
        value = overlapping_kernel_extract(thres, use_best_subkernel);
    } else {
        value = sparse_kernel_extract(thres, use_best_subkernel);
    }
    kernel_extract_free();

    ex_free_globals(need_duplicate);
    } while (! one_pass && value > 0);

    return 0;

usage:
    (void) fprintf(siserr, 
    "usage: gkx [-1abcdfo] [-t thresh]\n");
    (void) fprintf(siserr, 
    "    -1\t\tone pass over network\n");
    (void) fprintf(siserr, 
    "    -a\t\tuse all kernels (rather than level 0)\n");
    (void) fprintf(siserr, 
    "    -b\t\tuse best subkernel (rather than pingpong)\n");
    (void) fprintf(siserr, 
    "    -c\t\tuse complement during division\n");
    (void) fprintf(siserr, 
    "    -d\t\tenables debug\n");
    (void) fprintf(siserr, 
    "    -f\t\tuse factored literals for value\n");
    (void) fprintf(siserr, 
    "    -o\t\tallow overlapping factors\n");
    (void) fprintf(siserr, 
    "    -t n\tset threshold value for extraction\n");
    return 1;
}

/* ARGSUSED */
static int
print_kernel(kernel, co_kernel, state)
node_t *kernel;
node_t *co_kernel;
char *state;
{
    (void) fprintf(sisout, "(");
    node_print_rhs(sisout, co_kernel);
    (void) fprintf(sisout, ") * (");
    node_print_rhs(sisout, kernel);
    (void) fprintf(sisout, ")\n");
    node_free(kernel);
    node_free(co_kernel);
    return 1;
}


static int
com_print_kernel(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    node_t *node;
    array_t *node_vec;
    int c, i, subkernel, subkernel_level;

    subkernel = 0;
    subkernel_level = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "as")) != EOF) {
    switch(c) {
    case 'a':
        subkernel_level = 1;
        break;
    case 's':
        subkernel = 1;
        break;
    default:
        goto usage;
    }
    }

    node_vec = com_get_nodes(*network, argc-util_optind+1, argv+util_optind-1);

    for(i = 0; i < array_n(node_vec); i++) {
    node = array_fetch(node_t *, node_vec, i);
    if (node->type == PRIMARY_INPUT) continue;
    if (node->type == PRIMARY_OUTPUT) continue;
    if (subkernel) {
        (void) fprintf(sisout, "Subkernels of %s\n", node_name(node));
        ex_subkernel_gen(node, print_kernel, subkernel_level, NIL(char));
    } else {
        (void) fprintf(sisout, "Kernels of %s\n", node_name(node));
        ex_kernel_gen(node, print_kernel, NIL(char));
    }
    }
    array_free(node_vec);
    return 0;

usage:
    (void) fprintf(siserr, "usage: print_kernel [-as] n1 n2 ...\n");
    (void) fprintf(siserr, 
    "    -a\t\tgenerate kernels of all levels\n");
    (void) fprintf(siserr, 
    "    -s\t\tgenerate subkernels rather than kernels\n");
    return 1;
}

static int
com_gdiv(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    node_t *node, *divisor;
    array_t *node_vec;
    int i;

    node_vec = com_get_nodes(*network, argc, argv);
    if (array_n(node_vec) < 1) goto usage;

    for(i = 0; i < array_n(node_vec); i++) {
    node = array_fetch(node_t *, node_vec, i);
    if (node->type == PRIMARY_INPUT) continue;
    if (node->type == PRIMARY_OUTPUT) continue;
    if (strcmp(argv[0], "_gdiv") == 0) {
        (void) fprintf(sisout, "good divisor of %s is: ", node_name(node));
        divisor = ex_find_divisor(node, 0, 1);
    } else {
        (void) fprintf(sisout, "quick divisor of %s is: ", node_name(node));
        divisor = ex_find_divisor_quick(node);
    }
    if (divisor == 0) {
        (void) fprintf(sisout, " (no multiple-cube divisors)\n");
    } else {
        node_print_rhs(sisout, divisor);
        (void) fprintf(sisout, "\n");
        node_free(divisor);
    }
    }
    array_free(node_vec);
    return 0;

usage:
    (void) fprintf(siserr, "usage: %s n1 n2 ...\n", argv[0]);
    return 1;
}

init_extract()
{
    com_add_command("gkx", com_kernel_extract, 1);
    com_add_command("gcx", com_cube_extract, 1);
    com_add_command("print_kernel", com_print_kernel, 1);
    com_add_command("_gdiv", com_gdiv, 0);
    com_add_command("_qdiv", com_gdiv, 0);
    com_add_command("fx",com_fast_extract , 1);
}

end_extract()
{
    sm_cleanup();
}



static int com_fast_extract(network,argc,argv)
network_t **network;
int argc;
char **argv;
{
    sm_matrix *cube_literal_matrix;
    ddset_t *ddivisor_set;
    node_t *node;
    array_t *node_set;
    int i, c, row, index, objnum, cubenum, loop, iterate;
    int PRESERVE_LEVEL = 0;
    lsGen gen;
    sm_row *prow;
    st_table *level_table, *required_table;
    int max_level, po_level;
    char *dummy;
    node_t *po;
    extern st_table *find_node_level();

    /* Global variables should never have been used!  Must initialize
       them for extract. */
    ONE_PASS    = 0;
    FX_DELETE   = 0;
    LENGTH1     = 5;
    LENGTH2     = 5;
    OBJECT_SIZE = 50000;
    DONT_USE_WEIGHT_ZERO = 1;
    util_getopt_reset();
    while ((c = util_getopt(argc,argv,"b:f:s:lo:z")) != EOF) {
        switch(c) {
        case 'o' :
            ONE_PASS = 1;
            break;
        case 'l' :
            PRESERVE_LEVEL = 1;
            break;
        case 'b' :
            ONE_PASS = FX_DELETE = 1;
            OBJECT_SIZE = atoi(util_optarg);
            break;
        case 'f' :
            if (FX_DELETE == 0) break;
            LENGTH1 = atoi(util_optarg);
            break;
        case 's' :
            if (FX_DELETE == 0) break;
            LENGTH2 = atoi(util_optarg);
            break;
       case 'z' :
            DONT_USE_WEIGHT_ZERO = 0;
            break;
        default :
            goto usage;
        }
    }
    
    if (argc - util_optind != 0) goto usage;

    loop = 0;
    iterate = 1;
    do {
        row = 0; iterate--;
        if (PRESERVE_LEVEL){
            level_table= find_node_level(*network);
            max_level = 0;
            foreach_primary_output (*network, gen, po) {
                assert(st_lookup(level_table, (char *) po, &dummy));
                po_level = (int) dummy;
                if (max_level < po_level)
                    max_level = po_level;
            }
            required_table = find_required_time(*network, max_level);
        }

        cube_literal_matrix = sm_alloc();
        ddivisor_set = ddset_alloc_init();

        /* global setting the node index in the network */
        ex_setup_globals(*network, 0);

        objnum = 0;
        node_set = array_alloc(node_t *, 0);
        foreach_node(*network, gen, node) {
            switch(node_function(node)) {
            case NODE_PI:
            case NODE_PO:
            case NODE_0:
            case NODE_1:
            case NODE_INV:
            case NODE_BUF:
                break;
            case NODE_AND:
            case NODE_OR:
            case NODE_COMPLEX:
                (void) array_insert_last(node_t *, node_set, node);
                cubenum = node_num_cube(node);
                objnum += ((cubenum - 1) * cubenum) / 2;
                break;
 	    default:
		;
            }
        }

        FX_DELETE = (objnum >= OBJECT_SIZE) ? 1 : 0;
        if (FX_DELETE == 1) {
            ONE_PASS = 1;
            if (loop == 0) {
                loop = 1; iterate = 5;
            }
        } else {
            if (iterate > 0) ONE_PASS = 0;
        }
        for (i = 0; i < array_n(node_set); i++) {
            node = array_fetch(node_t *, node_set, i);
            fx_node_to_sm(node, cube_literal_matrix);
            index = nodeindex_indexof(global_node_index, node);
            extract_ddivisor(cube_literal_matrix,index,row,ddivisor_set);
            row = cube_literal_matrix->nrows;
        }
        (void) array_free(node_set);

        /* greeedy concurrent algorithm for finding best double cube divisor or
         * single cube divisor.
         */
        fast_extract(cube_literal_matrix,ddivisor_set);
        if (PRESERVE_LEVEL){
            remove_unwanted_nodes(*network, level_table, required_table);
            st_free_table(level_table);
            st_free_table(required_table);
        }
        network_sweep(*network);

        ddset_free(ddivisor_set);
        sm_foreach_row(cube_literal_matrix, prow) {
            (void) sm_cube_free((sm_cube_t *) prow->user_word);
        }
        (void) sm_free(cube_literal_matrix);
        ex_free_globals(0);
        if (iterate > 0) {
            (void) eliminate(*network, 0, 1000);
            (void) eliminate(*network, 5, 1000);
        }
    } while ((loop == 1) && (iterate > 0));

    return 0;

usage:
    (void) fprintf(siserr, "usage: fx [-o] [-b limit] [-l] [-z]\n");
    (void) fprintf(siserr, "     -o\t\t fast 0-level 2-cube kernel extraction\n");
    (void) fprintf(siserr, "     \t\t default is complete kernel extraction\n");
    (void) fprintf(siserr, "     -b\t\t upper bound for the number of divisors, default is 50000 \n");
    (void) fprintf(siserr, 
				 "     -l\t\t preserves the level of each node, used for timing optimization\n");
    (void) fprintf(siserr, "     -z\t\t extract zero-weight factors also\n");
    return 1;
}

static st_table *find_required_time(network, max_level)
network_t *network;
int max_level;
{
    st_table *node_required_table;
    array_t *nodevec;
    lsGen gen;
    int i;
    node_t *np, *fanout;
    int level, tmp;
    char *dummy;
                     
    node_required_table = st_init_table(st_ptrcmp, st_ptrhash);
    nodevec = network_dfs_from_input(network);
    for(i = 0; i < array_n(nodevec); i++) {
        np = array_fetch(node_t *, nodevec, i);
        if (node_function(np) == NODE_PI)
            continue;
        level = max_level;
        if (node_function(np) == NODE_PO){
            st_insert(node_required_table, (char *) np, (char *) level);
            continue;
        }
        foreach_fanout(np, gen, fanout){
            assert(st_lookup(node_required_table, (char *) fanout, &dummy));
            tmp= (int) dummy;
            if (level > tmp)
                level = tmp;
        }
        level--;
        assert(level >= 0);
        st_insert(node_required_table, (char *) np, (char *) level);
    }
    array_free(nodevec);
    return(node_required_table);
}

/* After extraction remove all the new nodes which cause the level some output go
 * beyond the max_level orginally computed for the network. We have already assigned
 * an allowed level to each node in the network. The new level can not be greater
 * than the allowed level.
 */
static void remove_unwanted_nodes(network, level_table, required_table)
network_t *network;
st_table  *level_table;
st_table *required_table;
{
    array_t *nodevec;
    node_t *np, *fanin;
    int tmp;
    char *dummy;
	int current_level, allowed_level;
	int i, l;
	int numin;
                     
    nodevec = network_dfs(network);
    for(i = 0; i < array_n(nodevec); i++) {
        np = array_fetch(node_t *, nodevec, i);
        if (node_function(np) == NODE_PI)
            continue;
        current_level = 0;
        foreach_fanin(np, l, fanin){
            if (node_function(fanin) == NODE_PI){
				tmp = 0;
            }else{
                assert(st_lookup(level_table, (char *) fanin, &dummy));
                tmp= (int) dummy;
            }
            if (current_level < tmp)
                current_level = tmp;
        }
        current_level++;
        if(st_lookup(required_table, (char *) np, &dummy)){
            allowed_level = (int) dummy;
            for (;current_level > allowed_level;){
/*collapse nodes in np whose level is above what is allowed */
			    numin = node_num_fanin(np);
				for(l=0; l< numin; l++){
				    fanin= node_get_fanin(np, l);
                    if (node_function(fanin) == NODE_PI)
                        continue;
                    assert(st_lookup(level_table, (char *) fanin, &dummy));
                    tmp= (int) dummy;
					if (tmp > (allowed_level - 1)){
						node_collapse(np, fanin);
                        assert(!st_lookup(required_table, (char *) fanin, &dummy));
			            numin = node_num_fanin(np);
					}
                }
/* update current level again.*/
				current_level = 0;
                foreach_fanin(np, l, fanin){
                    if (node_function(fanin) == NODE_PI){
		        		tmp = 0;
                    }else{
                        assert(st_lookup(level_table, (char *) fanin, &dummy));
                        tmp= (int) dummy;
                    }
                    if (current_level < tmp)
                      current_level = tmp;
                }
				current_level++;
            }
        }
        st_insert(level_table, (char *) np, (char *) current_level);
    }
    array_free(nodevec);
}
