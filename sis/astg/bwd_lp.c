/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/astg/bwd_lp.c,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2005/03/08 05:31:11 $
 *
 */
#ifdef SIS
/* Routines that insert inverter pairs where a potential hazard can actually 
 * occur, using the linear programming formulation of the ICCD92 paper. 
 */
#include "sis.h"
#include "astg_int.h"
#include "astg_core.h"
#include "bwd_int.h"
#include <math.h>

/* hacked from sim/interpret.c */
#define SIM_SLOT            simulation
#define GET_VALUE(node)         ((int) node->SIM_SLOT)
#define SET_VALUE(node, value)      (node->SIM_SLOT = (char *) (value))
#define	UP			(2 | 1)
#define	DOWN		(2 | 0)
#define	UNDEF		4
#define VAL_MASK	1

#define EQ 0
#define GE 1
#define LE 2
struct _constraint_struct {
	int *vars;
	int *coeffs;
	double val;
	int n; /* number of vars and coeffs */
	int type;
	int level; /* same level for same MAX */
};
typedef struct _constraint_struct constraint_t;

/* if UNDEF, set to new, otherwise set to UP and DOWN appropriately
 */
static void
new_value (node, new)
node_t *node;
int new;
{
	if ((GET_VALUE (node) & UNDEF) || (new & UNDEF)) {
		SET_VALUE (node, new);
	}
	else {
		if (new) {
			switch (GET_VALUE (node)) {
			case 1:
				SET_VALUE (node, 1);
				break;
			case 0:
				SET_VALUE (node, UP);
				break;
			case DOWN:
				fprintf (siserr, "error: up and down transition for %s\n",
					node_name (node));
				break;
			case UP:
				SET_VALUE (node, UP);
				break;
			default:
				break;
			}
		}
		else {
			switch (GET_VALUE (node)) {
			case 0:
				SET_VALUE (node, 0);
				break;
			case 1:
				SET_VALUE (node, DOWN);
				break;
			case UP:
				fprintf (siserr, "error: up and down transition for %s\n",
					node_name (node));
				break;
			case DOWN:
				SET_VALUE (node, DOWN);
				break;
			default:
				break;
			}
		}
	}
}

/* simulate a node; hacked from simulate.c
 */
static void
sim_node(node)
node_t *node;
{
    int i, value;
    node_t *fanin, *func, *func1, *lit;

    func = node_dup(node);
    foreach_fanin(node, i, fanin) {
		if (GET_VALUE(fanin) & UNDEF) {
			continue;
		}
		value = GET_VALUE(fanin) & VAL_MASK;

		lit = node_literal(fanin, value);
		func1 = node_cofactor(func, lit);
		node_free(func);
		node_free(lit);
		func = func1;

		if (node_function(func) == NODE_0 || node_function(func) == NODE_1) {
			break;
		}
    }

    switch (node_function(func)) {
	case NODE_0:
		new_value (node, 0);
		break;
	case NODE_1:
		new_value (node, 1);
		break;
	default:
		break;
    }
	if (astg_debug_flag > 3) {
		switch (GET_VALUE (node)) {
		case 0:
			fprintf (sisout, "    %s 0\n", node_long_name (node));
			break;
		case 1:
			fprintf (sisout, "    %s 1\n", node_long_name (node));
			break;
		case UP:
			fprintf (sisout, "    %s +\n", node_long_name (node));
			break;
		case DOWN:
			fprintf (sisout, "    %s -\n", node_long_name (node));
			break;
		default:
			break;
		}
	}

    node_free(func);
}

/* simulate a network; hacked from simulate.c
 * cb gives the pattern, and change_index signals which PI (if any) is changing
 * to change_val
 */

static void
sim_network(network, node_vec, pi_index, cb, change_index, change_val)
network_t *network;
array_t *node_vec;
st_table *pi_index;
pset cb;
int change_index, change_val;
{
    int i;
    node_t *node, *pi;
    lsGen gen;

    /* set the values on the primary inputs */
    foreach_primary_input(network, gen, pi) {
		if (! st_lookup (pi_index, bwd_po_name (node_long_name (pi)), 
			(char **) &i)) {
			/* test input... set it to 1 */
			new_value (pi, 1);
			if (astg_debug_flag > 3) {
				fprintf (sisout, "    %s 1 (test)\n", node_long_name (pi));
			}
			continue;
		}
		if (change_index == i) {
			new_value (pi, change_val);
		}
		else {
			switch (GETINPUT (cb, i)) {
			case ONE:
				new_value (pi, 1);
				break;
			case ZERO:
				new_value (pi, 0);
				break;
			default:
				new_value (pi, UNDEF);
				break;
			}
		}

		if (astg_debug_flag > 3) {
			switch (GET_VALUE (pi)) {
			case 0:
				fprintf (sisout, "    %s 0\n", node_long_name (pi));
				break;
			case 1:
				fprintf (sisout, "    %s 1\n", node_long_name (pi));
				break;
			case UP:
				fprintf (sisout, "    %s +\n", node_long_name (pi));
				break;
			case DOWN:
				fprintf (sisout, "    %s -\n", node_long_name (pi));
				break;
			default:
				break;
			}
		}
	}

    /* simulate the values through the network */
    for(i = 0; i < array_n(node_vec); i++) {
		node = array_fetch(node_t *, node_vec, i);
		if (node->type == INTERNAL) {
			sim_node(node);
		}
    }
}

/* find the subnet that is changing value.
 * we can have more than one fanin changing because they are killed
 * inside a complex gate... so the path is actually a subnetwork
 */
static void
find_subnet (node, subnet)
node_t *node;
st_table *subnet;
{
	node_t *fanin;
	int i;

	foreach_fanin (node, i, fanin) {
		switch (GET_VALUE (fanin)) {
		case UP:
			if (astg_debug_flag > 2) {
				fprintf (sisout, " %s+", node_long_name (node));
			}
			st_insert (subnet, (char *) fanin, (char *) GET_VALUE (fanin));
			find_subnet (fanin, subnet);
			break;
		case DOWN:
			if (astg_debug_flag > 2) {
				fprintf (sisout, " %s-", node_long_name (node));
			}
			st_insert (subnet, (char *) fanin, (char *) GET_VALUE (fanin));
			find_subnet (fanin, subnet);
			break;
		default:
			break;
		}
	}
}

/* Compute the delay inside subnet1 (which should end in po).
 * Gates also in subnet2 have their upper bound used anyway, otherwise
 * if we are computing a lower bound, min_delay_factor is used
 * We should become smarter here...
 */
static double
subnet_delay (node_vec, po, subnet1, subnet2, upper_bound, min_delay_factor)
array_t *node_vec;
node_t *po;
st_table *subnet1, *subnet2;
int upper_bound;
double min_delay_factor;
{
	node_t *node, *fanin;
	int i, j, node_dir, fanin_dir, cnt;
	double delta;
	delay_time_t delay;

	for (j = 0; j < array_n (node_vec); j++) {
		node = array_fetch (node_t *, node_vec, j);
		BWD(node)->arrival = -1;
	}
	for (j = 0; j < array_n (node_vec); j++) {
		node = array_fetch (node_t *, node_vec, j);
		if (! st_lookup (subnet1, (char *) node, (char **) &node_dir)) {
			continue;
		}

		/* compute the arrival time at node */
		if (node_num_fanin (node) == 0) {
			/* PI */
			delay = delay_node_pin (node, 0, DELAY_MODEL_LIBRARY);
			if (node_dir == UP) {
				delta = delay.rise;
			}
			else {
				delta = delay.fall;
			}
			if (! upper_bound) {
				delta *= min_delay_factor;
			}
			BWD(node)->arrival = delta;
		}
		else {
			cnt = 0;
			foreach_fanin (node, i, fanin) {
				if (! st_lookup (subnet1, (char *) fanin, 
					(char **) &fanin_dir)) {
					continue;
				}
				cnt++;
				delay = delay_node_pin (node, i, DELAY_MODEL_LIBRARY);
				if (node_dir == UP) {
					delta = delay.rise;
				}
				else {
					delta = delay.fall;
				}
				/* use upper bound it if it is also on subnet2 */
				if (! upper_bound && ! st_is_member (subnet2, (char *) node)) {
					delta *= min_delay_factor;
				}

				/* here we should take into account the 
				 * controlling/noncontrolling values 
				 */
				if (upper_bound) {
					if (BWD(node)->arrival < 0 || 
						BWD(node)->arrival < BWD(fanin)->arrival + delta) {
						BWD(node)->arrival = BWD(fanin)->arrival + delta;
					}
				}
				else {
					if (BWD(node)->arrival < 0 || 
						BWD(node)->arrival > BWD(fanin)->arrival + delta) {
						BWD(node)->arrival = BWD(fanin)->arrival + delta;
					}
				}
				if (astg_debug_flag > 4) {
					if (node_dir == UP) {
						fprintf (sisout, "%s+ from %s %f\n", 
							node_long_name (node), node_long_name (fanin), 
							BWD(fanin)->arrival + delta);
					}
					else {
						fprintf (sisout, "%s- from %s %f\n", 
							node_long_name (node), node_long_name (fanin), 
							BWD(fanin)->arrival + delta);
					}
				}
			}
			if (cnt > 1 && astg_debug_flag > 1) {
				fprintf (sisout, "warning: more than one fanin of %s changed\n",
					node_long_name (node));
			}
		}
		assert (BWD(node)->arrival >= 0);
		if (node_dir == UP) {
			if (astg_debug_flag > 3) {
				fprintf (sisout, "%s+ %f\n", node_long_name (node), 
					BWD(node)->arrival);
			}
		}
		else {
			if (astg_debug_flag > 3) {
				fprintf (sisout, "%s- %f\n", node_long_name (node), 
					BWD(node)->arrival);
			}
		}
	}

	node = node_get_fanin (po, 0);
	return BWD(node)->arrival;
}

/* return the upper bound on d (hazard.s2, po) - d (hazard.s1, po)
 */
static double
hazard_delay (network, pi_index, po, hazard, min_delay_factor)
network_t *network;
st_table *pi_index;
node_t *po;
hazard_t *hazard;
double min_delay_factor;
{
	node_t *node;
	array_t *node_vec;
	st_table *subnet1, *subnet2;
	int i, old_val, new_val;
	double d2, d1;
	pset cb = cube.temp[0];

	node_vec = network_dfs (network);
	/* simulate under on_cb1 -> off_cb1 */
	for (i = 0; i < array_n (node_vec); i++) {
		node = array_fetch (node_t *, node_vec, i);
		SET_VALUE (node, UNDEF);
	}
	consensus (cb, hazard->on_cb1, hazard->off_cb1);
	if (astg_debug_flag > 2) {
		fprintf (sisout, "simulating %s under %s\n",
			node_long_name (po), pc1(cb));
	}
	assert (st_lookup (pi_index, bwd_po_name (hazard->s1), (char **) &i));
	if (hazard->dir1 == '+') {
		old_val = 0;
		new_val = 1;
	}
	else {
		old_val = 1;
		new_val = 0;
	}
	sim_network(network, node_vec, pi_index, cb, i, old_val);
	if (astg_debug_flag > 2) {
		fprintf (sisout, "simulating %s under %s with %s%c\n",
			node_long_name (po), pc1(cb), hazard->s1, hazard->dir1);
	}
	sim_network(network, node_vec, pi_index, cb, i, new_val);

	/* now get the subnet */
	subnet1 = st_init_table (st_ptrcmp, st_ptrhash);
	if (astg_debug_flag > 2) {
		fprintf (sisout, "subnet 1:");
	}
	find_subnet (po, subnet1);
	if (astg_debug_flag > 2) {
		fprintf (sisout, "\n");
	}

	/* simulate under off_cb2 -> on_cb2 */
	for (i = 0; i < array_n (node_vec); i++) {
		node = array_fetch (node_t *, node_vec, i);
		SET_VALUE (node, UNDEF);
	}
	consensus (cb, hazard->on_cb2, hazard->off_cb2);
	if (astg_debug_flag > 2) {
		fprintf (sisout, "simulating %s under %s\n",
			node_long_name (po), pc1(cb));
	}
	assert (st_lookup (pi_index, bwd_po_name (hazard->s2), (char **) &i));
	if (hazard->dir2 == '+') {
		old_val = 0;
		new_val = 1;
	}
	else {
		old_val = 1;
		new_val = 0;
	}
	sim_network(network, node_vec, pi_index, cb, i, old_val);
	if (astg_debug_flag > 2) {
		fprintf (sisout, "simulating %s under %s with %s%c\n",
			node_long_name (po), pc1(cb), hazard->s2, hazard->dir2);
	}
	assert (st_lookup (pi_index, bwd_po_name (hazard->s2), (char **) &i));
	sim_network(network, node_vec, pi_index, cb, i, new_val);

	/* get the subnet */
	subnet2 = st_init_table (st_ptrcmp, st_ptrhash);
	if (astg_debug_flag > 2) {
		fprintf (sisout, "subnet 2:");
	}
	find_subnet (po, subnet2);
	if (astg_debug_flag > 2) {
		fprintf (sisout, "\n");
	}

	/* compute d1 and d2 */
	if (astg_debug_flag > 3) {
		fprintf (sisout, "computing d1\n");
	}
	d1 = subnet_delay (node_vec, po, subnet1, subnet2, /*upper_bound*/ 0,
		min_delay_factor);

	if (astg_debug_flag > 3) {
		fprintf (sisout, "computing d2\n");
	}
	d2 = subnet_delay (node_vec, po, subnet2, subnet1, /*upper_bound*/ 1,
		min_delay_factor);

	st_free_table (subnet1);
	st_free_table (subnet2);
	if (astg_debug_flag > 2) {
		fprintf (sisout, "d1 %f d2 %f\n", d1, d2);
	}
	if (d1 < 0 || d2 < 0) {
		return -1;
	}
	return d2 - d1;
}

/* find a subset of vertices that are on a path to a vertex that is on the
 * subset 
 */
static int
find_astg_subset (v)
astg_vertex *v;
{
	astg_edge *e;
	astg_generator gen;

	if (v->on_path || ! v->active) {
		return 0;
	}
	v->on_path = ASTG_TRUE;

	if (v->subset) {
		v->on_path = ASTG_FALSE;
		return 1;
	}
	if (v->unprocessed) {
		v->unprocessed = ASTG_FALSE;
		astg_foreach_out_edge (v, gen, e) {
			if (find_astg_subset (astg_head (e))) {
				v->subset = ASTG_TRUE;
			}
		}
	}
	v->on_path = ASTG_FALSE;
	return v->subset;
}

/* assign a topological sort index to each vertex in the subset
 */
static void
ts_dfs (g, v, num_p)
astg_graph  *g;
astg_vertex *v;
int *num_p;
{
	astg_generator gen;
	astg_edge *e;

	if (v->unprocessed) {
		v->unprocessed = ASTG_FALSE;
		astg_foreach_out_edge (v,gen,e) {
			ts_dfs (g,astg_head(e),num_p);
		}
		if (v->vtype == ASTG_TRANS) {
			v->alg.ts.index = (*num_p)++;
		}
	}
}

/* return the minimum estimated delay between from and to
 */
static double
min_del (from, to, external_del, default_del)
astg_trans *from, *to;
st_table *external_del;
double default_del;
{
	st_table *to_table;
	char *n;
	min_del_t *delay;

	n = astg_signal_name (astg_trans_sig (from));
	if (! st_lookup (external_del, n, (char **) &to_table)) {
		return default_del;
	}
	n = astg_signal_name (astg_trans_sig (to));
	if (! st_lookup (to_table, n, (char **) &delay)) {
		return default_del;
	}
	switch (astg_trans_type (from)) {
	case ASTG_POS_X:
		switch (astg_trans_type (to)) {
		case ASTG_POS_X:
			return delay->rise.rise;
		case ASTG_NEG_X:
			return delay->rise.fall;
		}
	case ASTG_NEG_X:
		switch (astg_trans_type (to)) {
		case ASTG_POS_X:
			return delay->fall.rise;
		case ASTG_NEG_X:
			return delay->fall.fall;
		}
	default:
		break;
	}
	fail ("invalid transition type\n");
	return 0;
}

static void
print_constr (file, constr)
FILE *file;
constraint_t *constr;
{
	int i;

	for (i = 0; i < constr->n; i++) {
		if (i != 0 && constr->coeffs[i] >= 0) {
			fprintf (file, "+ ");
		}
		fprintf (file, "%d x%d ", constr->coeffs[i], constr->vars[i]);
	}
	switch (constr->type) {
	case EQ:
		fprintf (file, "=");
		break;
	case GE:
		fprintf (file, ">=");
		break;
	case LE:
		fprintf (file, "<=");
		break;
	default:
		fail ("illegal LP constraint type");
	}
	fprintf (file, "%f\n", constr->val);
}

/* define an order, and stick to it... "level" does not matter */
static int
constr_cmp (c1, c2)
constraint_t *c1, *c2;
{
	int result, i;

	result = c1->n - c2->n;
	if (result) {
		return result;
	}
	result = c1->type - c2->type;
	if (result) {
		return result;
	}

	for (i = 0; i < c1->n; i++) {
		result = c1->vars[i] - c2->vars[i];
		if (result) {
			return result;
		}
		result = c1->coeffs[i] - c2->coeffs[i];
		if (result) {
			return result;
		}
	}
	if (c1->val < c2->val) {
		return -1;
	}
	if (c1->val > c2->val) {
		return 1;
	}
	return 0;
}

/* this should be optimized ... */
static void
add_constr (constrs, constr)
array_t *constrs;
constraint_t *constr;
{
	constraint_t *old_constr;
	int i;

	for (i = 0; i < array_n (constrs); i++) {
		old_constr = array_fetch (constraint_t *, constrs, i);
		if (! constr_cmp (constr, old_constr)) {
			return;
		}
	}
	array_insert_last (constraint_t *, constrs, constr);
	if (astg_debug_flag > 2) {
		print_constr (sisout, constr);
	}
}

static void
add_constr_1 (constrs, coeff, var, type, level, val)
array_t *constrs;
int coeff, var, type, level;
double val;
{
	constraint_t *constr;

	constr = ALLOC (constraint_t, 1);
	constr->n = 1;
	constr->vars = ALLOC (int, 1);
	constr->vars[0] = var;
	constr->coeffs = ALLOC (int, 1);
	constr->coeffs[0] = coeff;
	constr->val = val;
	constr->type = type;
	constr->level = level;
	add_constr (constrs, constr);
}

static void
add_constr_3 (constrs, coeff1, var1, coeff2, var2, coeff3, var3, type, level, val)
array_t *constrs;
int coeff1, var1, coeff2, var2, coeff3, var3, type, level;
double val;
{
	constraint_t *constr;

	constr = ALLOC (constraint_t, 1);
	constr->n = 3;
	constr->vars = ALLOC (int, 3);
	constr->vars[0] = var1;
	constr->vars[1] = var2;
	constr->vars[2] = var3;
	constr->coeffs = ALLOC (int, 3);
	constr->coeffs[0] = coeff1;
	constr->coeffs[1] = coeff2;
	constr->coeffs[2] = coeff3;
	constr->val = val;
	constr->type = type;
	constr->level = level;
	add_constr (constrs, constr);
}

/* write a set of LP constraints for the sub-STG between t2 and t1
 * Returns the number of equations written.
 */
static int
write_constr (t1, t2, constrs, order, names, min_var, external_del, sig_table, pair, level, delta, default_del)
astg_trans *t1, *t2;
array_t *constrs, *order, *names, *min_var;
st_table *external_del, *sig_table;
int pair, *level;
double delta, default_del;
{
	int k, to_n, from_n, sig_n;
	double w;
	astg_trans *to, *from;
	astg_generator pgen, tgen;
	astg_place *p;
	static char buf[512], *n;
	st_table *trans_table;

	trans_table = st_init_table (st_ptrcmp, st_ptrhash);
	for (k = 0; k < array_n (order); k++) {
	/* get all transition in topological order */
		to = array_fetch (astg_vertex *, order, k);
	/* create a variable with the delay of to and name D_to_pair */
		to_n = array_n (names);
		sprintf (buf, "D_%s_%d", astg_trans_name (to), pair);
		n = util_strsav (buf);
		array_insert_last (char *, names, n);
		st_insert (trans_table, (char *) to, (char *) to_n);

		if (k == 0) {
	/* source: the variable is 0 */
			assert (to == t2);
			add_constr_1 (constrs, 1, to_n, EQ, -1, (double) 0);
			continue;
		}
		if (k == array_n (order) - 1) {
			assert (to == t1);
			add_constr_1 (constrs, 1, to_n, GE, -1, delta);
		}

		astg_foreach_input_place (to, pgen, p) {
			astg_foreach_input_trans (p, tgen, from) {
				/* it may not be in trans_table yet if it also belongs to
				 * another MG component, i.e. if there is a loop pivoting on
				 * a predecessor place of "to"
				 */
				if (! from->subset ||
					! st_lookup (trans_table, (char *) from, 
						(char **) &from_n)) {
					continue;
				}
				/* create a constraint 
				 * D_to_pair {>=,=} D_from_pair + W(from,to) + X_to 
				 * where D are arrival times 
				 */
				w = min_del (from, to, external_del, default_del);

				assert (st_lookup (sig_table, astg_signal_name (astg_trans_sig (to)), (char **) &sig_n));

				add_constr_3 (constrs, 1, to_n, -1, from_n, -1,
					sig_n, GE, *level, w);
			}
		}
		(*level)++;
	}
	st_free_table (trans_table);
}

static double
solve_lp (constrs, min_var, names, slow, eval, lindo)
array_t *constrs, *min_var, *names;
double *slow;
int *eval, lindo;
{
	static char buf[512], tmp[L_tmpnam], out[L_tmpnam], in[L_tmpnam];
	FILE *lp_out, *lp_in, *lp_tmp;
	int i, j, cnt, m, n, m1, m2, m3, *izrov, *iposv, r1, r2, r3, row, icase;
	constraint_t *constr, *constr1;
	char *gt, *str, *name;
	double cost, **a;
	st_table *to_min;
	int tmpfd, infd, outfd;

	(*eval)++;
	if (lindo) {
		/* run the LP and get the cost */
		strcpy(tmp, "/tmp/astg_tmp_XXXXXX");
		tmpfd = mkstemp(tmp);
		if(tmpfd == -1) {
		  perror ("error creating tmp file");
		  return;
		}
		strcpy(tmp, "/tmp/astg_out_XXXXXX");
		outfd = mkstemp(out);
		if(outfd == -1) {
		  perror ("error creating out file");
		  return;
		}
		strcpy(tmp, "/tmp/astg_in_XXXXXX");
		infd = mkstemp(in);
		if(infd == -1) {
		  perror ("error creating in file");
		  return;
		}
		/* now create the LP input file lp_in */
		lp_in = fdopen (infd, "w");
		if (lp_in == NULL) {
			perror (in);
			return;
		}

		fprintf (lp_in, "MIN");
		if (astg_debug_flag > 1) {
			fprintf (sisout, "MIN");
		}
		for (i = 0; i < array_n (min_var); i++) {
			if (i > 0) {
				fprintf (lp_in, " +");
				if (astg_debug_flag > 1) {
					fprintf (sisout, " +");
				}	
			}
			/* signal: cost 1 */
			fprintf (lp_in, " x%d\n", array_fetch (int, min_var, i));
			if (astg_debug_flag > 1) {
				fprintf (sisout, " x%d\n", array_fetch (int, min_var, i));
			}
		}
		fprintf (lp_in, "ST\n");
		if (astg_debug_flag > 1) {
			fprintf (sisout, "ST\n");
		}
		for (i = 0; i < array_n (constrs); i++) {
			constr = array_fetch (constraint_t *, constrs, i);
			print_constr (lp_in, constr);
			if (astg_debug_flag > 1) {
				print_constr (sisout, constr);
			}
		}
		fprintf (lp_in, "END\nGO\nn\nQUIT\n");
		if (astg_debug_flag > 1) {
			fprintf (sisout, "END\nGO\nn\nQUIT\n");
		}
		fclose (lp_in);

		/* write the symbol table to the beginning of the LP output file */
		lp_out = fdopen (outfd, "w");
		if (lp_out == NULL) {
			perror (out);
			return;
		}
		if (astg_debug_flag > 1) {
			fprintf (sisout, "variable names\n");
		}
		fprintf (lp_out, "variable names\n");
		for (i = 0; i < array_n (names); i++) {
			str = array_fetch (char *, names, i);
			if (astg_debug_flag > 1) {
				fprintf (sisout, "%s x%d\n", str, i);
			}
			fprintf (lp_out, "%s x%d\n", str, i);
		}
		fclose (lp_out);

		sprintf (buf, "rsh dent run_lindo < %s >> %s", in, out);
		if (astg_debug_flag > 1) {
			fprintf (sisout, "%s\n", buf);
		}
		fprintf (sisout, "running lindo...");
		fflush(sisout);
		system (buf);
		fprintf (sisout, " done\n");

		/* analyze the output */
		if (astg_debug_flag > 1) {
			sprintf (buf, 
				"cat %s; tr a-z A-Z < %s | awk -f lindo.awk > %s", 
				out, out, tmp);
		}
		else {
			sprintf (buf, "tr a-z A-Z < %s | awk -f lindo.awk > %s", 
				out, tmp);
		}
		if (astg_debug_flag > 1) {
			fprintf (sisout, "%s\n", buf);
		}
		system (buf);

		lp_tmp = fdopen (tmpfd, "r");
		if (lp_tmp == NULL) {
			perror (tmp);
			return;
		}
		cost = -2;
		while (fgets (buf, sizeof(buf), lp_tmp) != NULL) {
			fputs (buf, sisout);
			if (! strncmp (buf, "total", 5)) {
				sscanf (buf, "total %lf", &cost);
			}
		}
		fclose (lp_tmp);
		unlink (tmp);
		unlink (in);
		unlink (out);
	}
	else {
		m1 = m2 = m3 = 0;
		for (i = 0; i < array_n (constrs); i++) {
			constr = array_fetch (constraint_t *, constrs, i);
			switch (constr->type) {
			case LE:
				m1++;
				break;
			case GE:
				m2++;
				break;
			case EQ:
				m3++;
				break;
			default:
				fail ("illegal linear constraint type");
			}
		}
		m = m1 + m2 + m3;
		n = array_n (names);
		a = ALLOC (double *, m + 3);
		for (i = 1; i <= m + 2; i++) {
			a[i] = ALLOC (double, n + 2);
			for (j = 1; j <= n + 1; j++) {
				a[i][j] = 0;
			}
		}
		iposv = ALLOC (int, m + 1);
		izrov = ALLOC (int, n + 1);

		if (astg_debug_flag > 1) {
			fprintf (sisout, "variable names\n");
			for (i = 0; i < array_n (names); i++) {
				str = array_fetch (char *, names, i);
				fprintf (sisout, "%s x%d\n", str, i);
			}
			fprintf (sisout, "linear program:\nMIN\n");
		}
		/* the cost function: MIN, hence cost -1 */
		to_min = st_init_table (st_numcmp, st_numhash);
		for (i = 0; i < array_n (min_var); i++) {
			j = array_fetch (int, min_var, i);
			slow[j] = 0;
			st_insert (to_min, (char *) j, (char *) i);
			a[1][j+2] = -1;
			if (astg_debug_flag > 1) {
				fprintf (sisout, " x%d\n", j);
			}
		}
		if (astg_debug_flag > 1) {
			fprintf (sisout, "ST\n");
		}
		r1 = r2 = r3 = 0;
		for (i = 0; i < array_n (constrs); i++) {
			constr = array_fetch (constraint_t *, constrs, i);
			if (astg_debug_flag > 1) {
				print_constr (sisout, constr);
			}
			switch (constr->type) {
			case LE:
				row = r1 + 2;
				r1++;
				break;
			case GE:
				row = m1 + r2 + 2;
				r2++;
				break;
			case EQ:
				row = m2 + r3 + 2;
				r3++;
				break;
			default:
				break;
			}
			for (j = 0; j < constr->n; j++) {
				a[row][constr->vars[j]+2] = -constr->coeffs[j];
			}
			a[row][1] = constr->val;
		}

		if (astg_debug_flag > 1) {
			fprintf (sisout, "solving...\n");
		}
		/* run the simplex and analyze the result */
		if (re_simplx (a, m, n, m1, m2, m3, &icase, izrov, iposv)) {
			fprintf (siserr, "%s\n", error_string ());
			error_init();
			return -2;
		}
		switch (icase) {
		case 1:
			fprintf (siserr, "unbounded solution ???\n");
			return -2;
		case -1:
			if (astg_debug_flag > 1) {
				fprintf (sisout, "infeasible\n");
			}
			return -2;
		default:
			break;
		}

		cost = 0;
		for (i = 1; i <= m; i++) {
			if (iposv[i] <= n) {
				if (! st_lookup_int (to_min, (char *) (iposv[i] - 1), &j)) {
					continue;
				}
				cost += a[i+1][1];
				slow[j] = a[i+1][1];
				if (astg_debug_flag > 1) {
					fprintf (sisout, "x%d by %f\n", i, a[i+1][1]);
				}
			}
		}

		for (i = 1; i <= m + 2; i++) {
			FREE (a[i]);
		}
		FREE (a);
		st_free_table (to_min);
	}
	return cost;
}

static void
write_lp_recur (constrs, min_var, names, slow, best_slow, n, do_bound, lindo, bound, eval, best_cost)
array_t *constrs, *min_var, *names;
double *slow, *best_slow;
int n, do_bound, lindo, *bound, *eval;
double *best_cost;
{
	int i, cnt;
	constraint_t *constr, *constr1;
	char *str;
	double cost;

	/* look for the first MAX type constraint */
	for (; n < array_n (constrs); n++) {
		constr = array_fetch (constraint_t *, constrs, n);
		if (constr->level >= 0) {
		/* we must resolve it... */
			break;
		}
	}

	if (n >= array_n (constrs)) {
		/* bottom: solve the LP and return */
		cost = solve_lp (constrs, min_var, names, slow, eval, lindo);
		if (*best_cost < 0 || (cost > -1 && cost < *best_cost)) {
			for (i = 0; i < array_n (min_var); i++) {
				best_slow[i] = slow[i];
			}
			*best_cost = cost;
			if (astg_debug_flag > 0) {
				fprintf (sisout, "new BEST cost %f\n", *best_cost);
			}
		}
		return;
	}


	/* choose one constraint at the current level */
	constr = array_fetch (constraint_t *, constrs, n);
	cnt = 1;
	for (i = n + 1; i < array_n (constrs); i++) {
		constr1 = array_fetch (constraint_t *, constrs, i);
		if (constr1->level != constr->level) {
			break;
		}
		cnt++;
	}
	if (cnt > 1 && astg_debug_flag > 1) {
		fprintf (sisout, "we must resolve %d constraints at level %d\n",
			cnt, constr->level);
	}
	if (do_bound && cnt > 1) {
	/* try to bound... */
		if (astg_debug_flag > 1) {
			fprintf (sisout, "trying to bound...\n");
		}
		cost = solve_lp (constrs, min_var, names, slow, eval, lindo);
		if (cost < 0) {
			(*bound)++;
			if (astg_debug_flag > 1) {
				fprintf (sisout, "bounding %f due to infeasibility\n",
					cost, *best_cost);
			}
			return;
		}
		if (*best_cost > 0 && cost > *best_cost) {
			(*bound)++;
			if (astg_debug_flag > 1) {
				fprintf (sisout, "bounding %f due to old best cost %f\n",
					cost, *best_cost);
			}
			return;
		}
	}
	/* sorry, we must recur... */
	for (i = 0; i < cnt; i++) {
	/* set constraint "i" at the current level to be an equality */
		constr = array_fetch (constraint_t *, constrs, n + i);
		assert (constr->type == GE);
		constr->type = EQ;
		if (astg_debug_flag > 1) {
			fprintf (sisout, "  setting equality for %d\n    ", i);
			print_constr (sisout, constr);
		}
		write_lp_recur (constrs, min_var, names, slow, best_slow, 
			n + cnt, do_bound, lindo, bound, eval, best_cost);
		constr->type = GE;
	}
}

/* remove hazards by linear programming (JVSP paper)
 */
void
bwd_lp_slow (astg, network, hazard_list, external_del, pi_index, tol, default_del, min_delay_factor, do_bound, lindo)
astg_graph *astg;
network_t *network;
st_table *hazard_list, *external_del, *pi_index;
double tol, default_del, min_delay_factor;
int do_bound, lindo;
{
	array_t *hazards, *order, *comp, *names, *min_var, *constrs;
	constraint_t *constr;
	st_table *sig_table;
	hazard_t hazard;
	lsGen gen;
	node_t *po, *node;
	char *n;
	int i, j, k, num, pair, sig_n, max_delta, level, bound, eval;
	double w, best_cost, *slow, *best_slow;
	double delta;
	astg_trans *t1, *t2, *t3, *from, *to;
	astg_place *p;
	astg_signal *sig1, *sig2;
	astg_generator tgen1, tgen2, tgen3, vgen, tgen, pgen, sgen;
	astg_trans_enum type1, type2;
	astg_vertex *v;
	static char buf[512];
	bwd_node_t *bwd;

	pair = 0;
	max_delta = 1;
	names = array_alloc (char *, 0);
	min_var = array_alloc (int, 0);
	define_cube_size (st_count (pi_index));
	(void) get_mg_comp (astg, /*verbose*/ 0);
	/* do a first update, to take into account the logic delays */
	bwd_external_delay_update (network, external_del, min_delay_factor,
		/*silent*/ 1);

	sig_table = st_init_table (st_ptrcmp, st_ptrhash);
	astg_foreach_signal (astg, sgen, sig1) {
		/* create a new variable for the padded delay at the signal */
		sig_n = array_n (names);
		sprintf (buf, "X_%s", astg_signal_name (sig1));
		n = util_strsav (buf);
		array_insert_last (char *, names, n);
		/* we want to minimize the padded delays */
		array_insert_last (int, min_var, sig_n);

		st_insert (sig_table, astg_signal_name (sig1), (char *) sig_n);
	}

	foreach_node (network, gen, node) {
		bwd = ALLOC (bwd_node_t, 1);
		node->BWD_SLOT = (char *) bwd;
	}
	constrs = array_alloc (constraint_t *, 0);
	level = 0;
	foreach_primary_output (network, gen, po) {
		if (network_is_real_po(network, po)) {
		/* skip real PO's connected directly to the PI */
			node = node_get_fanin (po, 0);
			if (node_type (node) == PRIMARY_INPUT) {
				continue;
			}
		}

		n = bwd_po_name (node_long_name (po));
		assert (st_lookup (hazard_list, n, (char **) &hazards));
		for (i = 0; i < array_n (hazards); i++) {
			hazard = array_fetch (hazard_t, hazards, i);
			sig1 = astg_find_named_signal (astg, bwd_po_name (hazard.s1));
			assert (sig1 != NIL(astg_signal));
			sig2 = astg_find_named_signal (astg, bwd_po_name (hazard.s2));
			assert (sig2 != NIL(astg_signal));
			/* derive the upper bound on d2 - d1 */
			delta = hazard_delay (network, pi_index, po, &hazard,
				min_delay_factor);

			if (astg_debug_flag > 2) {
				fprintf (sisout, 
					"%s%c -> %s%c for %s d2 - d1 %f\n",
					 astg_signal_name (sig2), hazard.dir2, 
					 astg_signal_name (sig1), hazard.dir1, 
					 bwd_po_name (node_long_name (po)), delta);
			}
			if (delta < 0) {
				if (astg_debug_flag > 2) {
					fprintf (sisout, "trivially satisfied (%f < 0)\n", delta);
				}
				continue;
			}

			/* now search each firing sequence between t2 and t1 ... */
			astg_foreach_vertex (astg, vgen, v) {
				v->active = ASTG_FALSE;
				v->on_path = ASTG_FALSE;
			}
			for (j = 0; j < array_n (astg->mg_comp); j++) {
				/* explore MG component j */
				comp = array_fetch (array_t *, astg->mg_comp, j);
				for (k = 0; k < array_n (comp); k++) {
					v = array_fetch (astg_vertex *, comp, k);
					v->active = ASTG_TRUE;
				}
			
				type1 = (hazard.dir1 == '+') ? ASTG_POS_X : ASTG_NEG_X;
				type2 = (hazard.dir2 == '+') ? ASTG_POS_X : ASTG_NEG_X;
				astg_foreach_trans (astg, tgen1, t1) {
					if (! t1->active  ||
						astg_trans_sig (t1) != sig1 ||
						astg_trans_type (t1) != type1) {
						continue;
					}
					astg_foreach_trans (astg, tgen2, t2) {
						if (! t2->active  ||
							astg_trans_sig (t2) != sig2 ||
							astg_trans_type (t2) != type2) {
							continue;
						}
						if (astg_debug_flag > 2) {
							fprintf (sisout, "exploring %s -> %s\n",
								astg_trans_name (t2), astg_trans_name (t1));
						}
						astg_foreach_vertex (astg, vgen, v) {
							v->unprocessed = ASTG_TRUE;
							v->subset = ASTG_FALSE;
						}
					/* do not process transitions of the same signal as t1  */
						astg_foreach_trans (astg, tgen3, t3) {
							if (astg_trans_sig (t3) == astg_trans_sig (t1)) {
								t3->unprocessed = ASTG_FALSE;
							}
						}
						t1->subset = ASTG_TRUE;
						find_astg_subset (t2);
						astg_foreach_vertex (astg, vgen, v) {
							v->alg.ts.index = 0;
							v->unprocessed = v->subset;
						}
						num = 0;
						ts_dfs (astg, t2, &num);
						order = array_alloc (astg_vertex *, num);
						if (num) {
							astg_foreach_vertex (astg, vgen, v) {
								if (! v->subset || v->vtype != ASTG_TRANS) {
									continue;
								}
								array_insert (astg_vertex *, order,
									(num - v->alg.ts.index - 1), v);
								v->weight1.f = 0;
							}
						}
						if (astg_debug_flag > 3) {
							fprintf (sisout, "subset:");
						}
						/* now check if the constraint is trivial */
						for (k = 0; k < array_n (order); k++) {
							to = array_fetch (astg_vertex *, order, k);
							astg_foreach_input_place (to, pgen, p) {
								astg_foreach_input_trans (p, tgen, from) {
									if (! from->subset) {
										continue;
									}
									w = min_del (from, to, external_del,
										default_del);
									to->weight1.f = MAX (to->weight1.f, 
										from->weight1.f + w);
								}
							}
							if (astg_debug_flag > 3) {
								fprintf (sisout, " %s %f", astg_v_name (to),
									to->weight1.f);
							}
						}
						if (astg_debug_flag > 3) {
							fprintf (sisout, "\n");
						}
						if (t1->weight1.f > delta) {
							if (astg_debug_flag > 2) {
								fprintf (sisout, 
									"%s -> %s trivially satisfied (%f > %f)\n",
									astg_trans_name (t2), astg_trans_name (t1),
									t1->weight1.f, delta);
							}
							array_free (order);
							continue;
						}

						/* produce the constraints */
						pair++;
						if (astg_debug_flag > 2) {
							fprintf (sisout, 
								"%s -> %s produces constraint\n",
								astg_trans_name (t2), astg_trans_name (t1));
						}
						write_constr (t1, t2, constrs, order, 
							names, min_var, external_del, sig_table, pair, 
							&level, delta, default_del);
						max_delta = MAX (delta, max_delta);
						array_free (order);
					}
				}
			}
		}
	}
	foreach_node (network, gen, node) {
		FREE (node->BWD_SLOT);
	}

	if (astg_debug_flag > 0) {
		fprintf (sisout, "%d constraints %d variables\n", 
			array_n (constrs), array_n (names));
	}
	bound = 0;
	eval = 0;
	if (array_n (constrs)) {
		best_cost = -1;
		slow = ALLOC (double, array_n (min_var));
		best_slow = ALLOC (double, array_n (min_var));
		write_lp_recur (constrs, min_var, names, slow, best_slow, 
			0, do_bound, lindo, &bound, &eval, &best_cost);

		for (i = 0; i < array_n (min_var); i++) {
			if (best_slow[i] > 0.0) {
				n = array_fetch (char *, names, i);
				fprintf (sisout, "slowing %s by %f\n", &n[2], best_slow[i]);
			}
		}
		FREE (slow);
		FREE (best_slow);
	}
	else {
		best_cost = 0;
	}
	fprintf (sisout, "final BEST slow down %f %d boundings %d evaluations\n",
		best_cost, bound, eval);

	/* free and wrap up */
	for (i = 0; i < array_n (names); i++) {
		n = array_fetch (char *, names, i);
		FREE (n);
	}
	for (i = 0; i < array_n (constrs); i++) {
		constr = array_fetch (constraint_t *, constrs, i);
		FREE (constr->vars);
		FREE (constr->coeffs);
		FREE (constr);
	}
	array_free (constrs);
	array_free (names);
	array_free (min_var);
	st_free_table (sig_table);
}
#endif /* SIS */
