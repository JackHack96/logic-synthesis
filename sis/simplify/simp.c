
#include "sis.h"
#include "simp_int.h"

static void simp_accept();
static node_t *local_dc();
void simplify_cspf_node();

void
simplify_node(f, method, dctype, accept, filter, level_table)
node_t *f;
sim_method_t method;
sim_dctype_t dctype;
sim_accept_t accept;
sim_filter_t filter;
st_table *level_table;
{
    node_t *DC, *newf;
    node_sim_type_t node_sim_type;
    char buffer[80];

    if (simp_trace) {
    (void) fprintf(sisout, "%-6s ", node_name(f));
    (void) fprintf(sisout, "(%3d): ", factor_num_literal(f));
    (void) fflush(sisout);
    }

    /* generate the don't care set */ 
    switch (dctype) {
    case SIM_DCTYPE_NONE:
    DC = node_constant(0); 
    break;
    case SIM_DCTYPE_FANIN:
    DC = simp_tfanin_dc(f, simp_fanin_level, simp_fanin_fanout_level);
    break;
    case SIM_DCTYPE_FANOUT:
    DC = simp_fanout_dc(f);
    break;
    case SIM_DCTYPE_INOUT:
    DC = simp_inout_dc(f, simp_fanin_level, simp_fanin_fanout_level);
    break;
    case SIM_DCTYPE_SUB_FANIN:
    DC = simp_sub_fanin_dc(f);
    break;
    case SIM_DCTYPE_ALL:
    DC = simp_all_dc(f);
    break;
    case SIM_DCTYPE_LEVEL:
    DC = simp_level_dc(f,level_table);
    break;
    case SIM_DCTYPE_UNKNOWN:
    default:
    fail("Error: unknown dctype");
    /* NOTREACHED */
    }

    if (simp_trace) {
    (void) sprintf(buffer, "DCb=(%d,%d,%d)", 
        node_num_fanin(DC), node_num_cube(DC), node_num_literal(DC));
    (void) fprintf(sisout, "%-24s", buffer);
    (void) fflush(sisout);
    }
    if (simp_debug) {
    (void) fprintf(sisout, "DC(%s) = ", node_name(f));
        node_print_rhs(sisout, DC);
    (void) fprintf(sisout, "\n");
    (void) fflush(sisout);
    }

    /* trim the don't-care set */
    DC = simp_dc_filter(f, DC, filter);

    if (simp_trace) {
    (void) sprintf(buffer, "DCa=(%d,%d,%d)", 
        node_num_fanin(DC), node_num_cube(DC), node_num_literal(DC));
    (void) fprintf(sisout, "%-24s", buffer);
    (void) fflush(sisout);
    }
    if (simp_debug) {
    (void) fprintf(sisout, "simp_dc_filtered(%s) = ", node_name(f));
        node_print_rhs(sisout, DC);
    (void) fprintf(sisout, "\n");
    (void) fflush(sisout);
    }


    /* minimize f */
    switch (method) {
    case SIM_METHOD_SIMPCOMP:
    node_sim_type = NODE_SIM_SIMPCOMP;
    break;
    case SIM_METHOD_ESPRESSO:
    node_sim_type = NODE_SIM_ESPRESSO;
    break;
    case SIM_METHOD_EXACT:
    node_sim_type = NODE_SIM_EXACT;
    break;
    case SIM_METHOD_EXACT_LITS:
    node_sim_type = NODE_SIM_EXACT_LITS;
    break;
    case SIM_METHOD_DCSIMP:
    node_sim_type = NODE_SIM_DCSIMP;
    break;
    case SIM_METHOD_NOCOMP:
    node_sim_type = NODE_SIM_NOCOMP;
    break;
    case SIM_METHOD_SNOCOMP:
    node_sim_type = NODE_SIM_SNOCOMP;
    break;
    case SIM_METHOD_UNKNOWN:
    default:
    fail("Error: unknown simplication method");
    /* NOTREACHED */
    }

    newf = node_simplify(f, DC, node_sim_type);
    node_free(DC);

    /* accept ? */
    simp_accept(f, newf, accept);

    /* save update the sim_flag */ 
    SIM_FLAG(f)->method = method;
    SIM_FLAG(f)->accept = accept;
    SIM_FLAG(f)->dctype = dctype;
}

static void
simp_accept(f, newf, accept)
node_t *f, *newf;
sim_accept_t accept;
{
    bool do_accept;
    int old, new;

    do_accept = FALSE;

    switch (accept) {
    case SIM_ACCEPT_FCT_LITS:
    old = factor_num_literal(f);
    new = factor_num_literal(newf);
    break;
    case SIM_ACCEPT_SOP_LITS:
    old = node_num_literal(f);
    new = node_num_literal(newf);
    break;
    case SIM_ACCEPT_CUBES:
    old = node_num_cube(f);
    new = node_num_cube(newf);
    break;
    case SIM_ACCEPT_ALWAYS:
    old = factor_num_literal(f);
    new = factor_num_literal(newf);
    do_accept = TRUE;
    break;
    case SIM_ACCEPT_UNKNOWN:
    default:
    fail("Error: unknown acceptance criteria.");
    exit(-1);
    /* NOTREACHED */
    }

    if ((new == old) && (accept == SIM_ACCEPT_FCT_LITS)) {
        old = node_num_literal(f);
        new = node_num_literal(newf);
    }
    if (do_accept || new < old) {
    if (simp_trace) {
        (void) fprintf(sisout, "simplified ", node_name(f));
        (void) fprintf(sisout, "(%d)\n", new);
        (void) fflush(sisout);
    }
    (void) node_replace(f, newf);
    } else {
    if (simp_trace) {
        (void) fprintf(sisout, "not simplified\n", node_name(f));
        (void) fflush(sisout);
    }
    node_free(newf);
    }
}

void
simplify_cspf_node(f, method, dctype, accept, filter, level_table, mg, leaves)
node_t *f;
sim_method_t method;
sim_dctype_t dctype;
sim_accept_t accept;
sim_filter_t filter;
st_table *level_table;
bdd_manager *mg;
st_table *leaves;
{
    node_t *DC, *newf;
    node_sim_type_t node_sim_type;
    char buffer[80];

    if (simp_trace) {
    (void) fprintf(sisout, "%-6s ", node_name(f));
    (void) fprintf(sisout, "(%3d): ", factor_num_literal(f));
    (void) fflush(sisout);
    }
    DC = local_dc(f, mg, leaves, filter, level_table);

    if (simp_trace) {
    (void) sprintf(buffer, "DCb=(%d,%d,%d)", 
        node_num_fanin(DC), node_num_cube(DC), node_num_literal(DC));
    (void) fprintf(sisout, "%-24s", buffer);
    (void) fflush(sisout);
    }

    if (filter != SIM_FILTER_NONE){
    DC = simp_obssatdc_filter(DC, f, 2);
    if ((node_num_cube(DC) > 100) && (node_num_fanin(DC)> 20))
        DC = simp_obssatdc_filter(DC, f, 1);
    if ((node_num_cube(DC) > 100) && (node_num_fanin(DC)> 20))
        DC = simp_obssatdc_filter(DC, f, 0);
    }

    if (simp_trace) {
    (void) sprintf(buffer, "DCa=(%d,%d,%d)", 
        node_num_fanin(DC), node_num_cube(DC), node_num_literal(DC));
    (void) fprintf(sisout, "%-24s", buffer);
    (void) fflush(sisout);
    }

    /* minimize f */
    switch (method) {
    case SIM_METHOD_SIMPCOMP:
    node_sim_type = NODE_SIM_SIMPCOMP;
    break;
    case SIM_METHOD_ESPRESSO:
    node_sim_type = NODE_SIM_ESPRESSO;
    break;
    case SIM_METHOD_EXACT:
    node_sim_type = NODE_SIM_EXACT;
    break;
    case SIM_METHOD_EXACT_LITS:
    node_sim_type = NODE_SIM_EXACT_LITS;
    break;
    case SIM_METHOD_DCSIMP:
    node_sim_type = NODE_SIM_DCSIMP;
    break;
    case SIM_METHOD_NOCOMP:
    node_sim_type = NODE_SIM_NOCOMP;
    break;
    case SIM_METHOD_SNOCOMP:
    node_sim_type = NODE_SIM_SNOCOMP;
    break;
    case SIM_METHOD_UNKNOWN:
    default:
    fail("Error: unknown simplication method");
    /* NOTREACHED */
    }
    
    newf = node_simplify(f, DC, node_sim_type);
    if(hsimp_debug){
      fprintf(sisout, ".");
      fflush(sisout);
    }

    node_free(DC);

    /* accept ? */
    simp_accept(f, newf, accept);

    /* save update the sim_flag */ 
    SIM_FLAG(f)->method = method;
    SIM_FLAG(f)->accept = accept;
    SIM_FLAG(f)->dctype = dctype;
}

static node_t *local_dc(f, mg, leaves, filter, level_table)
node_t *f;
bdd_manager *mg;
st_table *leaves;
sim_filter_t filter;
st_table *level_table;
{
    node_t  *dc1, *careset, *node;
    bdd_t *bdd;
    array_t *bdd_list, *node_list;
    int num_in, i;
    node_t *DC;
    node_t *ldc;
    bdd_t *newbdd, *c, *cnot, *outbdd;
    array_t *tfo_list;
    int set_size_sort();

    num_in= node_num_fanin(f);
    if (filter == SIM_FILTER_LEVEL){
      dc1= simp_level_dc(f, level_table);
    }else{
      dc1= simp_sub_fanin_dc(f);
    }

    node_list= array_alloc(node_t *,0);
    bdd_list= array_alloc(bdd_t *,0);


    tfo_list= network_tfo(f, INFINITY);
    newbdd= bdd_one(mg);
    for(i=0 ; i< array_n(tfo_list); i++){
      node= array_fetch(node_t *, tfo_list, i);
      if(node_function(node)== NODE_PO){
        outbdd= ntbdd_at_node(CSPF(node)->node) ;
        bdd= bdd_and(newbdd, outbdd, 1, 1);
        bdd_free(newbdd);
        newbdd= bdd;
      }
    }
    bdd = cspf_bdd_dc(f, mg);
    c= bdd_or(bdd, newbdd, 1, 1);
    bdd_free(bdd);
    bdd_free(newbdd);
    cnot = bdd_not(c);
    bdd_free(c);

    for(i=0 ; i< num_in ; i++){
      node= node_get_fanin(f, i);
      array_insert_last(node_t *, node_list, node);
    }
    array_sort(node_list, set_size_sort);
    if( !bdd_is_tautology(cnot, 0)){
      for(i=0 ; i< num_in ; i++){
          node= array_fetch(node_t *, node_list, i);
          bdd= ntbdd_at_node(node);
        array_insert_last(bdd_t *, bdd_list, bdd_cofactor(bdd, cnot));
      }

      careset = simp_bull_cofactor(bdd_list, node_list, mg, leaves);
    }else{
      careset= node_constant(0);
    }
    bdd_free(cnot);
    array_free(tfo_list);

    if(hsimp_debug){
      fprintf(sisout, "1");
      fflush(sisout);
    }
    ldc = node_not(careset);
    if(hsimp_debug){
      fprintf(sisout, "2");
      fflush(sisout);
    }
    node_free(careset);
    DC= node_or(dc1, ldc);
    node_free(dc1);
    node_free(ldc);
    return DC;
}
