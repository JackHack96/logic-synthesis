/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/simplify/com_simp.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:49 $
 *
 */
#include <setjmp.h>
#include <signal.h>
#include "sis.h"
#include "simp_int.h"

#define BDD_NODE_LIMIT 480000
#define BDD_NODE_LOW  50000

int com_simplify();
int com_simp();
int com_full_simplify();
st_table *find_node_level();
static bool get_cone_levels();
static void full_simplify_usage();
static void simp_node_to_bdd();
bool hsimp_debug;

static jmp_buf timeout_env;
static void timeout_handle()
{
  longjmp(timeout_env, 1);
}

init_simplify()
{
    simp_trace = FALSE;
    simp_debug = FALSE;

    node_register_daemon(DAEMON_FREE, simp_free);
    node_register_daemon(DAEMON_ALLOC, simp_alloc);
    node_register_daemon(DAEMON_DUP, simp_dup);
    node_register_daemon(DAEMON_INVALID, simp_invalid);

    com_add_command("simplify", com_simplify, 1);
    com_add_command("_simp", com_simp, 1);
    com_add_command("full_simplify", com_full_simplify, 0);
}

end_simplify()
{
}

static void full_simplify_usage(command)
char *command;
{
    (void) fprintf(siserr, "usage: %s [-d]", command);
    (void) fprintf(siserr, "[-o ordering] [-m method] [-l] ");
    (void) fprintf(siserr, "[-t time] [-v]\n");
    (void) fprintf(siserr, "    -o 0 \tordering type (uses depths: default) \n");
    (void) fprintf(siserr, "    -o 1 \tordering type (uses levels) \n");
    (void) fprintf(siserr, "    -d   \tNo observability don't care set \n");
    (void) fprintf(siserr, "    -l   \trestricted don't care to preserve the level");
    (void) fprintf(siserr, "(default is a subset of fanin don't cares)\n");
    (void) fprintf(siserr, "    -m snocomp\tSingle pass minimizer (default)\n");
    (void) fprintf(siserr, "    -m nocomp\tMultiple pass minimizer\n");
    (void) fprintf(siserr, "    -m dcsimp\tAnother single pass minimizer\n");
    (void) fprintf(siserr, "    -v \tverbose (for debugging)\n");
/*
    (void) fprintf(siserr, "    -t <num> \ttimeout mechanism \n");
*/
}

static void simp_usage(command)
char *command;
{
    (void) fprintf(siserr, "usage: %s [-d]", command);
    (void) fprintf(siserr, "[-i <num>[:<num>]] [-m method] [-f filter] ");
    (void) fprintf(siserr, "[node-list]\n");
    (void) fprintf(siserr, "    -d\tNo don't care set ");
    (void) fprintf(siserr, "  -l restricted don't care to preserve the level");
    (void) fprintf(siserr, "(default is a subset of the fanin don't cares)\n");
    (void) fprintf(siserr, "    -i <num>[:<num>]\tGenerate fanin don't-care");
    (void) fprintf(siserr, " for nodes specified in cone\n");
    (void) fprintf(siserr, "    -m snocomp\tSingle pass minimizer (default)\n");
    (void) fprintf(siserr, "    -m nocomp\tMultiple pass minimizer\n");
    (void) fprintf(siserr, "    -m dcsimp\tAnother single pass minimizer\n");
    (void) fprintf(siserr, "    -f exact\tExact filter (default)\n");
    (void) fprintf(siserr, "    -f disj_sup\tDisjoint support filter\n");
}

int
com_simplify(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c, i; 
    array_t *nodevec;
    node_t *np;
    sim_accept_t accept;
    sim_method_t method;
    sim_dctype_t dctype;
    sim_filter_t filter;
    st_table *level_table;
    extern int level_node_cmp2();

    dctype = SIM_DCTYPE_SUB_FANIN;
    accept = SIM_ACCEPT_FCT_LITS;
    method = SIM_METHOD_SNOCOMP;
    filter = SIM_FILTER_EXACT;
    util_getopt_reset();
    simp_debug = FALSE;
    simp_trace = FALSE;
    simp_fanin_level = 1;
    simp_fanin_fanout_level = 0;
    while ((c = util_getopt(argc, argv, "m:i:f:tdl")) != EOF) {
    switch (c) {
    case 'l':
        dctype = SIM_DCTYPE_LEVEL;
        break;
    case 'd':
        dctype = SIM_DCTYPE_NONE;
        break;
    case 'm':
        if (strcmp(util_optarg, "nocomp") == 0) {
            method = SIM_METHOD_NOCOMP;
        } else if (strcmp(util_optarg, "snocomp") == 0) {
            method = SIM_METHOD_SNOCOMP;
        } else if (strcmp(util_optarg, "dcsimp") == 0) {
            method = SIM_METHOD_DCSIMP;
        } else {
        simp_usage(argv[0]);
        return 1;
        }
        break;
    case 'f':
        if (strcmp(util_optarg, "exact") == 0) {
        filter = SIM_FILTER_EXACT;
        } else if (strcmp(util_optarg, "disj_sup") == 0) {
        filter = SIM_FILTER_DISJ_SUPPORT;
        } else {
        simp_usage(argv[0]);
        return 1;
        }
        break;
    case 'i':
        dctype = SIM_DCTYPE_FANIN;
        if (!get_cone_levels(util_optarg, &simp_fanin_level,
                 &simp_fanin_fanout_level)) {
        simp_usage(argv[0]);
        return 1;
        }
        break;
    case 't':
        simp_trace = TRUE;
        break;
    default:
        simp_usage(argv[0]);
        return 1;
    }
    }
    if(dctype == SIM_DCTYPE_LEVEL){
       level_table= find_node_level(*network);
    }

    nodevec = com_get_nodes(*network, argc-util_optind+1, argv+util_optind-1);
    nodevec = simp_order(nodevec);
    for(i = 0; i < array_n(nodevec); i++) {
        np = array_fetch(node_t *, nodevec, i); 
        if (np->type == INTERNAL) {
             simplify_node(np, method, dctype, accept, filter, level_table); 
        }
    }
    if(dctype == SIM_DCTYPE_LEVEL){
       st_free_table(level_table);
    }
    array_free(nodevec);
    return 0;
}

int
com_simp(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c, i; 
    array_t *nodevec;
    node_t *np;
    sim_method_t method;
    sim_accept_t accept;
    sim_dctype_t dctype;
    sim_filter_t filter;
    st_table *level_table;
    level_table= NIL (st_table);
    dctype = SIM_DCTYPE_FANIN;
    accept = SIM_ACCEPT_FCT_LITS;
    method = SIM_METHOD_SNOCOMP;
    filter = SIM_FILTER_EXACT;
    util_getopt_reset();
    simp_debug = FALSE;
    simp_trace = FALSE;
    simp_fanin_level = 1;
    simp_fanin_fanout_level = 0;
    while ((c = util_getopt(argc, argv, "a:d:m:i:o:f:Dt")) != EOF) {
    switch (c) {
    case 'a':
        if (strcmp(util_optarg, "fct_lits") == 0) {
        accept = SIM_ACCEPT_FCT_LITS;
        } else if (strcmp(util_optarg, "sop_lits") == 0) {
        accept = SIM_ACCEPT_SOP_LITS;
        } else if (strcmp(util_optarg, "cubes") == 0) {
        accept = SIM_ACCEPT_CUBES;
        } else if (strcmp(util_optarg, "always") == 0) {
        accept = SIM_ACCEPT_ALWAYS;
        } else {
        simp_usage(argv[0]);
        return 1;
        }
        break;
    case 'd':
        if (strcmp(util_optarg, "none") == 0) {
        dctype = SIM_DCTYPE_NONE;
        } else if (strcmp(util_optarg, "fanin") == 0) {
        dctype = SIM_DCTYPE_FANIN;
        } else if (strcmp(util_optarg, "fanout") == 0) {
        dctype = SIM_DCTYPE_FANOUT;
        } else if (strcmp(util_optarg, "inout") == 0) {
        dctype = SIM_DCTYPE_INOUT;
        } else if (strcmp(util_optarg, "support") == 0) {
        dctype = SIM_DCTYPE_SUB_FANIN;
        } else if (strcmp(util_optarg, "all") == 0) {
        dctype = SIM_DCTYPE_ALL;
        } else {
        simp_usage(argv[0]);
        return 1;
        }
        break;
    case 'm':
        if (strcmp(util_optarg, "simpcomp") == 0) {
        method = SIM_METHOD_SIMPCOMP;
        } else if (strcmp(util_optarg, "espresso") == 0) {
        method = SIM_METHOD_ESPRESSO;
        } else if (strcmp(util_optarg, "exact") == 0) {
        method = SIM_METHOD_EXACT;
        } else if (strcmp(util_optarg, "min_lit") == 0) {
        method = SIM_METHOD_EXACT_LITS;
        } else if (strcmp(util_optarg, "dcsimp") == 0) {
        method = SIM_METHOD_DCSIMP;
        } else if (strcmp(util_optarg, "nocomp") == 0) {
        method = SIM_METHOD_NOCOMP;
        } else if (strcmp(util_optarg, "snocomp") == 0) {
        method = SIM_METHOD_SNOCOMP;
        } else {
        simp_usage(argv[0]);
        return 1;
        }
        break;
    case 'f':
        if (strcmp(util_optarg, "exact") == 0) {
        filter = SIM_FILTER_EXACT;
        } else if (strcmp(util_optarg, "disj_sup") == 0) {
        filter = SIM_FILTER_DISJ_SUPPORT;
        } else if (strcmp(util_optarg, "size") == 0) {
        filter = SIM_FILTER_SIZE;
            } else if (strcmp(util_optarg, "fdist") == 0) {
        filter = SIM_FILTER_FDIST;
        } else if (strcmp(util_optarg, "sdist") == 0) {
            filter = SIM_FILTER_SDIST;
        } else if (strcmp(util_optarg, "none") == 0) {
        filter = SIM_FILTER_NONE;
        } else if (strcmp(util_optarg, "all") == 0) {
        filter = SIM_FILTER_ALL;
        } else {
        simp_usage(argv[0]);
        return 1;
        }
        break;
    case 'i':
        simp_fanin_level = atoi(util_optarg);
        switch (dctype) {
        case SIM_DCTYPE_FANOUT:
        dctype = SIM_DCTYPE_INOUT;
        break;
        case SIM_DCTYPE_INOUT:
        case SIM_DCTYPE_ALL:
        break;
        default:
        dctype = SIM_DCTYPE_FANIN;
        }
        break;
    case 'o':
        simp_fanin_fanout_level = atoi(util_optarg);
        switch (dctype) {
        case SIM_DCTYPE_FANOUT:
        dctype = SIM_DCTYPE_INOUT;
        break;
        case SIM_DCTYPE_INOUT:
        case SIM_DCTYPE_ALL:
        break;
        default:
        dctype = SIM_DCTYPE_FANIN;
        }
        break;
    case 't':
        simp_trace = TRUE;
        break;
    case 'D':
        simp_debug = TRUE;
        break;
    default:
        simp_usage(argv[0]);
        return 1;
    }
    }

    nodevec = com_get_nodes(*network, argc-util_optind+1, argv+util_optind-1);
    for(i = 0; i < array_n(nodevec); i++) {
    np = array_fetch(node_t *, nodevec, i); 
    if (np->type == INTERNAL) {
         simplify_node(np, method, dctype, accept, filter, level_table); 
    }
    }
    array_free(nodevec);
    return 0;
}

static bool
get_cone_levels(name, par1, par2)
char *name;
int *par1, *par2;
{
    char *s, *str, *str1 = NIL(char), *str2 = NIL(char);

    /* Check for silly input */
    if (name == NIL(char) || strcmp(name, "") == 0) 
    return FALSE;
    if (strchr(name, ' ') != NIL(char)) 
    return FALSE;
    
    /* Copy the input string (because we will tear it apart) */ 
    str = ALLOC(char, strlen(name)+1);
    s = strcpy(str, name);

    if (s[0] != ':') str1 = s;
    if ((s = strchr(s, ':')) != NIL(char)) {
    *s++ = '\0';
    str2 = s;
    }

    if (str1 != NIL(char))
        *par1 = atoi(str1);
    if (str2 != NIL(char))
        *par2 = atoi(str2);
    FREE(str);
    return TRUE;
}

st_table *find_node_level(network)
network_t *network;
{
   st_table *node_level_table;
   array_t *nodevec;
   int i,j;
   node_t *np, *fanin;
   int level,tmp;
   char *dummy;
    
   node_level_table = st_init_table(st_ptrcmp, st_ptrhash);
   nodevec= network_dfs(network);
   for(i = 0; i < array_n(nodevec); i++) {
      np = array_fetch(node_t *, nodevec, i); 
      if (node_function(np) == NODE_PI)
         continue;
      level = 0;
      foreach_fanin(np, j, fanin){
         if(st_lookup(node_level_table, (char *) fanin, &dummy))
            tmp= (int) dummy;
         else
            tmp= 0;
         if (level < tmp)
            level=tmp;
      }
      level++;
      st_insert(node_level_table, (char *) np, (char *) level);
   }
   array_free(nodevec);
   return(node_level_table);
}


int
com_full_simplify(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    st_table *leaves;
    st_table *node_exdc_table;
    array_t *po_list;
    array_t *newlist;
    array_t *odc_order_list, *network_node_list;
    node_t *pi, *po, *node;
    bdd_t  *bdd, *temp_bdd;
    lsGen gen;
    bdd_manager *manager;
    int x_ordering = 0;
    int array_size;
    extern int level_node_cmp2();
    int c,i,j; 
    sim_accept_t accept;
    sim_method_t method;
    sim_dctype_t dctype;
    sim_filter_t filter;
    st_table *level_table;
    extern st_table  *attach_dcnetwork_to_network();
    int timeout = 0;
    st_generator *tablegen;
    int max, index;
    bdd_stats stats;

    dctype = SIM_DCTYPE_ALL;
    accept = SIM_ACCEPT_FCT_LITS;
    method = SIM_METHOD_SNOCOMP;
    filter = SIM_FILTER_EXACT;
    util_getopt_reset();
    hsimp_debug = FALSE;
    simp_debug = FALSE;
    simp_trace = FALSE;
    simp_fanin_level = 1;
    simp_fanin_fanout_level = 0;
    while ((c = util_getopt(argc, argv, "f:o:m:t:vdl")) != EOF) {
    switch (c) {
    case 'o':
        x_ordering = atoi(util_optarg);
        break;
    case 't':
        timeout= atoi(util_optarg);
        break;
    case 'l':
        filter = SIM_FILTER_LEVEL;
        break;
    case 'f':
        if (strcmp(util_optarg, "level") == 0) {
        filter = SIM_FILTER_LEVEL;
        } else if (strcmp(util_optarg, "all") == 0) {
        filter = SIM_FILTER_ALL;
        } else {
        full_simplify_usage(argv[0]);
        return 1;
        }
        break;
    case 'd':
        dctype = SIM_DCTYPE_FANIN;
        break;
    case 'm':
        if (strcmp(util_optarg, "nocomp") == 0) {
            method = SIM_METHOD_NOCOMP;
        } else if (strcmp(util_optarg, "snocomp") == 0) {
            method = SIM_METHOD_SNOCOMP;
        } else if (strcmp(util_optarg, "dcsimp") == 0) {
            method = SIM_METHOD_DCSIMP;
        } else {
        return 1;
        }
        break;
    case 'v':
        hsimp_debug= TRUE;
        break;
    default:
        full_simplify_usage(argv[0]);
        return 1;
    }
    }
    if (*network == NIL (network_t))
        return 0;

    if (timeout > 0) {
        (void) signal(SIGALRM, timeout_handle);
        (void) alarm((unsigned int) timeout);
        if (setjmp(timeout_env) > 0) {
            fprintf(sisout, "timeout occurred after %d seconds\n", timeout);
            return 0;
        }
    }


    if(filter == SIM_FILTER_LEVEL){
        level_table= find_node_level(*network);
    }
    po_list = array_alloc(node_t *, 0);
    foreach_primary_output (*network, gen, po) {
        array_insert_last(node_t *, po_list, po);
    }
    copy_dcnetwork(*network);

    node_exdc_table = attach_dcnetwork_to_network(*network);
    foreach_primary_output (*network, gen, po) {
        node = find_node_exdc(po, node_exdc_table);
        array_insert_last(node_t *, po_list, node);
    }



    newlist = NIL (array_t);
    leaves = st_init_table(st_ptrcmp, st_ptrhash);
    if (x_ordering){
        network_node_list= network_dfs_from_input(*network);
        array_size= array_n(network_node_list);
        odc_order_list= array_alloc(node_t *, 0);
        for(i= 0; i< array_size; i++){
            node= array_fetch(node_t *, network_node_list, i);
            odc_alloc(node);
            ODC(node)->value= odc_value(node);
            ODC(node)->order= i;
            if (node_function(node) == NODE_PO)
                continue;
            array_insert_last(node_t *, odc_order_list, node);
        }
        (void) find_odc_level(*network);
        array_sort(odc_order_list, level_node_cmp2);
        for(i= 0, j = 0 ; i < array_n(odc_order_list) ; i++){
            node= array_fetch(node_t *, odc_order_list, i);
            if (node_function(node) == NODE_PI){
                (void) st_insert(leaves, (char *) node, (char *) j++);
            }
        }
        for(i= 0 ; i < array_n(network_node_list) ; i++){
            node= array_fetch(node_t *, network_node_list, i);
            odc_free(node);
        }
        array_free(odc_order_list);
        array_free(network_node_list);
    }else{
        foreach_primary_input(*network, gen, pi) {
            (void) st_insert(leaves, (char *) pi, (char *) -1);
        }
        newlist= order_dfs(po_list, leaves, 0);
        array_free(newlist);
    }

/* Order all the PI's that cannot be reached from PO's*/
    max = 0;
    st_foreach_item_int(leaves, tablegen, (char **) &pi, &index) {
        if (index > max) {
            max = index;
        }
    }    
    max++;
    st_foreach_item_int(leaves, tablegen, (char **) &pi, &index) {
        if (index < 0){
            index = max++;
            (void) st_insert(leaves, (char *) pi, (char *) index);
        }
    }

    manager = ntbdd_start_manager(network_num_pi(*network));
    for(i= 0 ; i < array_n(po_list) ; i++){
        node= array_fetch(node_t *, po_list, i);
        if (node_function(node) != NODE_PO)
            node_free(node);
    }
    array_free(po_list);
  
/*allocate space for CSPFs; Get the external don't cares for Primary Outputs*/
    newlist = network_dfs(*network);

    foreach_node(*network, gen,node){
        if (node_function(node) == NODE_PI)
            continue;
        cspf_alloc(node);
    }

    bdd_get_stats(manager, &stats);
    for(i = 0; i < array_n(newlist) ; i++){
        node = array_fetch(node_t *, newlist, i);
        if (node_function(node) == NODE_PI)
            continue;
        if (stats.nodes.total > BDD_NODE_LOW){
        (void) simp_node_to_bdd(node, manager, leaves);
    } else{
            (void) ntbdd_node_to_bdd(node, manager, leaves);
        }
        bdd_get_stats(manager, &stats);
        if (stats.nodes.total > BDD_NODE_LIMIT){
            ntbdd_end_manager(manager);
            fprintf(sisout, "The BDD's for this circuit have more than %d nodes.\n", BDD_NODE_LIMIT);
            fprintf(sisout, "full_simplify is not performed. \n");
            return(0);
        }
        if (node_function(node) == NODE_PO){
            CSPF(node)->node= find_node_exdc(node, node_exdc_table);
            temp_bdd = ntbdd_node_to_bdd(CSPF(node)->node, manager, leaves);
            CSPF(node)->bdd = bdd_dup(temp_bdd);
            continue;
        }
    }
    array_free(newlist);
    if (hsimp_debug) {
        bdd_get_stats(manager, &stats);
        fprintf(sisout, "total BDD nodes=  %d \n", stats.nodes.total);
        fprintf(sisout, "used BDD nodes=  %d \n", stats.nodes.used);
        foreach_primary_output(*network, gen, node){
            fprintf(sisout, "used BDD nodes for outputs =  %d \n", bdd_size(ntbdd_at_node(node)));
        }
    }
    foreach_node(*network, gen,node){
        if (node_function(node) == NODE_PI)
            continue;
        if (node_function(node) == NODE_PO)
            continue;
        if (node_num_fanout(node) == 0)
            continue;
        bdd =  ntbdd_node_to_bdd(node, manager, leaves);
        CSPF(node)->set= bdd_get_support(bdd);
    }

    if (hsimp_debug) {
        fprintf(sisout, "done");
        fflush(sisout);
    }


    if(dctype == SIM_DCTYPE_FANIN) {
        simplify_without_odc(*network,accept,method,dctype,filter,level_table,manager,leaves);
    }else{
        simplify_with_odc(*network,accept,method,dctype,filter,level_table,manager,leaves);
    }
    foreach_node(*network, gen,node){
        ntbdd_free_at_node(node); 
        if (node_function(node)!= NODE_PI){
            bdd_free(CSPF(node)->bdd);
            if (CSPF(node)->list != NIL (array_t))
                array_free(CSPF(node)->list);
            if (CSPF(node)->node != NIL (node_t))
                node_free(CSPF(node)->node);
            if (CSPF(node)->set != NIL (var_set_t))
                var_set_free(CSPF(node)->set);
            cspf_free(node);
        }
    }
                                                                                
    free_dcnetwork_copy(*network);
    
    ntbdd_end_manager(manager);
    st_free_table(leaves);
    if (node_exdc_table != NIL(st_table)) {
        st_free_table(node_exdc_table);
    }
    if(filter == SIM_FILTER_LEVEL){
        st_free_table(level_table);
    }
    if (hsimp_debug) {
        fprintf(sisout, "\n");
    }
    if (timeout > 0) {
        (void) alarm((unsigned int) INFINITY);
    }
    return 0;
}

static void simp_node_to_bdd(node, manager, leaves)
node_t *node;
bdd_manager *manager;
st_table *leaves;
{
    bdd_t *bdd, *newbdd, *tmpbdd, *cube_bdd;
    node_t *fanin;
    int i,j,k;
    pset last, setp;
    bdd_stats stats;

    bdd = bdd_zero(manager);

    if (node_function(node) == NODE_PO){
        (void) ntbdd_node_to_bdd(node, manager, leaves);
        return;
    }

    foreach_set(node->F, last, setp) {
        cube_bdd = bdd_one(manager);
        for(k=0 ; k< node->nin; k++){
            if ((j= GETINPUT(setp,k)) == TWO)
                continue;
            if (j== ONE){
                fanin= node_get_fanin(node,k);
                tmpbdd = ntbdd_node_to_bdd(fanin, manager, leaves);
                newbdd = bdd_and(cube_bdd, tmpbdd, 1, 1);
            }else{
                fanin= node_get_fanin(node,k);
                tmpbdd = ntbdd_node_to_bdd(fanin, manager, leaves);
                newbdd = bdd_and(cube_bdd, tmpbdd, 1, 0);
            }
            bdd_free(cube_bdd);
            cube_bdd = newbdd;
            bdd_get_stats(manager, &stats);
            if (stats.nodes.total > BDD_NODE_LIMIT)
                return;
        }
        newbdd = bdd_or(bdd, cube_bdd, 1, 1);    
        bdd_get_stats(manager, &stats);
        if (stats.nodes.total > BDD_NODE_LIMIT)
            return;
        bdd_free(bdd);
        bdd_free(cube_bdd);
        bdd = newbdd;
    }
    ntbdd_set_at_node(node, bdd);
}
