
#include "sis.h"
#include "atpg_int.h"

static int *c_sum_literals  = NIL(int);
static int *c_cube_literals = NIL(int);
static int c_max_cubes = 0;
static int c_max_fanin = 0;

static void add_sum ();
static void atpg_sop_clauses();
static void fault_path_outputs();
static void tfanin_cone();
static void exdc_tfanin_cone();
static void atpg_gen_fault_clauses(); 
static void atpg_gen_fault_site();
static void atpg_gen_active_clauses();

static void 
add_sum(sat_prob, literals, nlits, out_var)
sat_t *sat_prob;
int *literals;
int nlits;
int out_var;
{
    int i;

    for (i = nlits; i --; ) {
	sat_add_2clause(sat_prob, sat_neg(literals[i]), out_var);
    }
    switch (nlits) {
	case 0: sat_add_1clause(sat_prob, sat_neg(out_var));
	    break;
	case 1: sat_add_2clause(sat_prob, literals[0], sat_neg(out_var));
	    break;
	default:
	    sat_begin_clause(sat_prob);
	    for (i = 0; i < nlits; i ++) {
		sat_add_literal(sat_prob, literals[i]);
	    }
	    sat_add_literal(sat_prob, sat_neg(out_var));
    }

}

/* gates of fanin > 1 */
static void
atpg_sop_clauses(sat_prob, node, output_var)
sat_t *sat_prob;
node_t *node;
int output_var;
{
    int lit;
    atpg_clause_t *node_info;
    int n_cubes, n_lit, value, i, j, index;
    node_cube_t cube;
    node_t *fanin;

    n_cubes = node_num_cube(node);
    if (n_cubes > c_max_cubes) {
	c_max_cubes = n_cubes;
	c_sum_literals  = REALLOC(int, c_sum_literals, c_max_cubes);
    }
    i = node_num_fanin(node);
    if (i > c_max_fanin) {
	c_max_fanin = i;
	c_cube_literals = REALLOC(int, c_cube_literals, c_max_fanin);
    }
    node_info = ATPG_CLAUSE_GET(node);
    for (i = n_cubes; i --; ) {
	cube = node_get_cube(node, i);
        n_lit = 0;
	for (j = 0; j < node_info->nfanin; j ++) {
	    index = node_info->fanin[j];
	    fanin = node_get_fanin(node, index);
	    value = node_get_literal(cube, index);
	    if (value == ZERO) {
		c_cube_literals[n_lit ++] = ATPG_GET_VARIABLE(fanin);
	    }
	    else if (value == ONE) {
		c_cube_literals[n_lit ++] = sat_neg(ATPG_GET_VARIABLE(fanin));
	    }
	}

	if (n_lit == 1) {
	    c_sum_literals[i] = sat_neg(c_cube_literals[0]);
	}
	else {
	    lit = (n_cubes == 1) ? 
			output_var : sat_new_variable(sat_prob);
	    c_sum_literals[i] = lit;
	    add_sum(sat_prob, c_cube_literals, n_lit, sat_neg(lit));
	}
    }

    if (n_cubes > 1) {
	add_sum(sat_prob, c_sum_literals, n_cubes, output_var);
    }
}

void
atpg_node_clause(node, sat_prob)
node_t *node;
sat_t *sat_prob;
{
    int in_var, out_var;

    switch (node_function(node)) {
        case NODE_0:
	    sat_add_1clause(sat_prob, sat_neg(ATPG_GET_VARIABLE(node)));
	    break;
        case NODE_1:
	    sat_add_1clause(sat_prob, ATPG_GET_VARIABLE(node));        
	    break;
        case NODE_PI:
	    break;
        case NODE_PO:
        case NODE_BUF:
	    in_var = ATPG_GET_VARIABLE(node_get_fanin(node, 0));
	    out_var = ATPG_GET_VARIABLE(node);
	    sat_add_2clause(sat_prob, in_var, sat_neg(out_var));
	    sat_add_2clause(sat_prob, sat_neg(in_var), out_var);
	    break;
        case NODE_INV:
	    in_var = ATPG_GET_VARIABLE(node_get_fanin(node, 0));
	    out_var = ATPG_GET_VARIABLE(node);
	    sat_add_2clause(sat_prob, in_var, out_var);
	    sat_add_2clause(sat_prob, sat_neg(in_var), sat_neg(out_var));
	    break;
        case NODE_AND:
        case NODE_OR:
        case NODE_COMPLEX:
	    atpg_sop_clauses(sat_prob, node, ATPG_GET_VARIABLE(node));
	    break;
        default:
	    fail ("illegal node function");
    }
}

static void
fault_path_outputs(node, fault_tfo, cone_heads)
node_t  *node;
array_t *fault_tfo;
array_t *cone_heads;
{
    atpg_clause_t *node_info;
    int i;

    node_info = ATPG_CLAUSE_GET(node);
    if (! node_info->visited) {
	node_info->visited = TRUE;
	for (i = 0; i < node_info->nfanout; i ++) {
	    fault_path_outputs(node_info->fanout[i], fault_tfo, cone_heads);
	}
	array_insert_last(node_t *, fault_tfo, node);
	/* boths PO's and fanout free nodes into cone_heads */
	if (node_info->nfanout == 0) {
	    array_insert_last(node_t *, cone_heads, node);
	}
    }
}

/* generate good circuit clauses 
 * return list of PI's in transitive fanin of cone_heads */
static void
tfanin_cone(node, sat_prob, pi_list)
node_t *node;
sat_t *sat_prob;
array_t *pi_list;
{
    atpg_clause_t *node_info;
    node_t *fanin;
    int i;
    
    node_info = ATPG_CLAUSE_GET(node);
    if (! node_info->visited) {
	node_info->visited = TRUE;
	if (node_function(node) == NODE_PI) {
	    array_insert_last(node_t *, pi_list, node);
	}
	node_info->true_id = sat_new_variable(sat_prob);
	node_info->current_id = node_info->true_id;
	for (i = 0; i < node_info->nfanin; i ++) {
	    fanin = node_get_fanin(node, node_info->fanin[i]);
	    tfanin_cone(fanin, sat_prob, pi_list);
	}
	atpg_node_clause(node, sat_prob);
    }
}

/* generate clauses for ex_dc network */
static void
exdc_tfanin_cone(node, sat_prob, network, pi_list)
node_t *node;
sat_t *sat_prob;
network_t *network;
array_t *pi_list;
{
    atpg_clause_t *node_info, *pi_info;
    node_t *pi, *fanin;
    int i;
    
    node_info = ATPG_CLAUSE_GET(node);
    if (! node_info->visited) {
	node_info->visited = TRUE;
	if (node_function(node) == NODE_PI) {
	    pi = network_find_node(network, node->name);
	    assert(pi != NIL(node_t));
	    pi_info = ATPG_CLAUSE_GET(pi);
	    if (pi_info->visited) {
		node_info->true_id = pi_info->true_id;
	    }
	    else {
		node_info->true_id = sat_new_variable(sat_prob);
		pi_info->true_id = node_info->true_id;
		pi_info->visited = TRUE;
	    	array_insert_last(node_t *, pi_list, pi);
	    }
	    node_info->current_id = node_info->true_id;
	}
	else {
	    node_info->true_id = sat_new_variable(sat_prob);
	    node_info->current_id = node_info->true_id;
	    for (i = 0; i < node_info->nfanin; i ++) {
		fanin = node_get_fanin(node, node_info->fanin[i]);
		exdc_tfanin_cone(fanin, sat_prob, network, pi_list);
	    }
	    atpg_node_clause(node, sat_prob);
	}
    }
}

/* setup fault_id field and generate faulty clauses - skips last entry */
static void
atpg_gen_fault_clauses(sat_prob, fault_shadow) 
sat_t *sat_prob;
array_t *fault_shadow;
{
    atpg_clause_t *node_info;
    int i;
    node_t *node;

    /* set up fault id's - process fanin before fanout so id's are set up
     * right */
    for (i = array_n(fault_shadow); i --; ) {
	node = array_fetch(node_t *, fault_shadow, i);
	node_info = ATPG_CLAUSE_GET(node);
	node_info->fault_id = sat_new_variable(sat_prob);
	node_info->current_id = ATPG_CLAUSE_GET(node)->fault_id;

	/* skip the fault node itself */
	if (i != (array_n(fault_shadow) - 1)) {
	    atpg_node_clause(node, sat_prob);
	}
    }
}

static void
atpg_gen_fault_site(sat_prob, fault)
sat_t *sat_prob;
fault_t *fault;
{
    int faulty_site_id, good_site_id;
    node_t *fanin;

    if (fault->fanin == NIL(node_t)) {
	faulty_site_id = ATPG_CLAUSE_GET(fault->node)->fault_id;
	good_site_id = ATPG_CLAUSE_GET(fault->node)->true_id;
    }
    else {
	fanin = node_get_fanin(fault->node, fault->index);
	faulty_site_id = sat_new_variable(sat_prob);
	good_site_id = ATPG_CLAUSE_GET(fanin)->true_id;
	ATPG_CLAUSE_GET(fanin)->fault_id = faulty_site_id;
	ATPG_CLAUSE_GET(fanin)->current_id = faulty_site_id;
	atpg_node_clause(fault->node, sat_prob);
    }

    if (fault->value == S_A_0) { 
        sat_add_1clause(sat_prob,  good_site_id);
        sat_add_1clause(sat_prob, sat_neg(faulty_site_id));
    }
    else { 
        sat_add_1clause(sat_prob, sat_neg(good_site_id));
        sat_add_1clause(sat_prob,  faulty_site_id);
    }
}

static void
atpg_gen_active_clauses(sat_prob, fault_shadow)
sat_t *sat_prob;
array_t *fault_shadow;
{
    int i, n;
    atpg_clause_t *node_info;
    node_t *node, *fanout;
    lsGen gen;

    /* set up active id's and generate clauses 
     * do fanout before fanin */
    for (i = 0, n = array_n(fault_shadow); i < n; i ++) {
	node = array_fetch(node_t *, fault_shadow, i);
	node_info = ATPG_CLAUSE_GET(node);
	node_info->active_id = sat_new_variable(sat_prob);
	sat_add_3clause(sat_prob, sat_neg(node_info->active_id), 
				  node_info->true_id, node_info->fault_id);
	sat_add_3clause(sat_prob, sat_neg(node_info->active_id), 
		    sat_neg(node_info->true_id), sat_neg(node_info->fault_id));
	if (node_function(node) != NODE_PO) {
	    /* atleast one fanout must be active */
	    if (node_num_fanout(node) == 1) {
		sat_add_2clause(sat_prob, sat_neg(node_info->active_id), 
		    ATPG_CLAUSE_GET(node_get_fanout(node, 0))->active_id);
	    }
	    else {
		sat_begin_clause(sat_prob);
		sat_add_literal(sat_prob, sat_neg(node_info->active_id));
		foreach_fanout(node, gen, fanout) {
		    sat_add_literal(sat_prob, 
				    ATPG_CLAUSE_GET(fanout)->active_id);
		}
	    }
	}
	if (i == (n - 1)) {
	    /* fault site is always active */
	    sat_add_1clause(sat_prob, ATPG_CLAUSE_GET(node)->active_id);
	}
    }
}

/* returns array of primary inputs in transitive fanin of fault */
int
atpg_network_fault_clauses(info, exdc_info, seq_info, fault)
atpg_ss_info_t *info;
atpg_ss_info_t *exdc_info;
seq_info_t *seq_info;
fault_t *fault;
{
    int i, pi_cnt;
    node_t  *node, *dc_po, *output;
    array_t *cone_heads, *fault_shadow, *pi_list;
    atpg_clause_t *node_info;
    lsGen gen;
    sat_input_t sv;

    /* find coneheads */
    cone_heads = array_alloc(node_t *, 0);
    fault_shadow = array_alloc(node_t *, 0);
    foreach_node(info->network, gen, node) {
	node_info = ATPG_CLAUSE_GET(node);
	node_info->visited = FALSE;
	node_info->true_id = -1;
	node_info->fault_id = -1;
	node_info->active_id = -1;
	node_info->current_id = -1;
    }
    fault_path_outputs(fault->node, fault_shadow, cone_heads);

    /* generate good circuit clauses */
    foreach_node(info->network, gen, node) {
	node_info = ATPG_CLAUSE_GET(node);
	node_info->visited = FALSE;
    }
    pi_list = array_alloc(node_t *, 0);
    for (i = 0; i < array_n(cone_heads); i ++) {
	tfanin_cone(array_fetch(node_t *, cone_heads, i),
		    info->atpg_sat, pi_list);
    }

    /* generate fault clauses */
    atpg_gen_fault_clauses(info->atpg_sat, fault_shadow);
    atpg_gen_fault_site(info->atpg_sat, fault);
    atpg_gen_active_clauses(info->atpg_sat, fault_shadow);

    /* add test condition */
    sat_begin_clause(info->atpg_sat);
    for (i = array_n(cone_heads); i --; ) {
	node = array_fetch(node_t *, cone_heads, i);
	if (node_function(node) == NODE_PO) {
	    sat_add_literal(info->atpg_sat, ATPG_CLAUSE_GET(node)->active_id);
	}
    }

    /* add external don't cares */
    if (exdc_info != NIL(atpg_ss_info_t)) {
	foreach_node(exdc_info->network, gen, node) {
	    node_info = ATPG_CLAUSE_GET(node);
	    node_info->visited = FALSE;
	}
	for (i = 0; i < array_n(cone_heads); i ++) {
	    node = array_fetch(node_t *, cone_heads, i);
	    if (node_function(node) == NODE_PO) {
		dc_po = network_find_node(exdc_info->network, node->name);
		assert(dc_po != NIL(node_t));
		exdc_tfanin_cone(dc_po, info->atpg_sat, info->network, pi_list);
		sat_add_2clause(info->atpg_sat,
			    sat_neg(ATPG_CLAUSE_GET(node)->active_id),
			    sat_neg(ATPG_CLAUSE_GET(dc_po)->true_id));
	    }
	}
    }
    /* restrict solutions to reachable states */
    if (seq_info != NIL(seq_info_t)) {
	foreach_node(seq_info->valid_states_network, gen, node) {
	    node_info = ATPG_CLAUSE_GET(node);
	    node_info->visited = FALSE;
	}
    	output = network_get_po(seq_info->valid_states_network, 0);
	exdc_tfanin_cone(output, info->atpg_sat, info->network, pi_list);
    	/* add a clause requiring the output to be 1 */
    	sat_add_1clause(info->atpg_sat, ATPG_CLAUSE_GET(output)->true_id);
    }
    
    pi_cnt = array_n(pi_list);
    for (i = pi_cnt; i --; ) {
	node = array_fetch(node_t *, pi_list, i);
	sv.info = (char *) node;
	sv.sat_id = ATPG_CLAUSE_GET(node)->true_id;
	array_insert(sat_input_t, info->sat_input_vars, i, sv);
    }
    array_free(cone_heads);
    array_free(fault_shadow);
    array_free(pi_list);

    return pi_cnt;
}

void
atpg_sat_clause_begin()
{
    c_sum_literals  = NIL(int);
    c_cube_literals = NIL(int);
    c_max_cubes = 0;
    c_max_fanin = 0;
}

void
atpg_sat_clause_end()
{
    if (c_sum_literals) {
	FREE(c_sum_literals);
	c_max_cubes = 0;
    }
    if (c_cube_literals) {
	FREE(c_cube_literals);
	c_max_fanin = 0;
    }
}
