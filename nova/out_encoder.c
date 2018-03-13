/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/out_encoder.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:10 $
 *
 */
#include "inc/out_encoder.h"
#include "inc/nova.h"

int iter, flag[MAXNODE];
int enc_iter;

/* main routine of the bounded row encoding strategy */
out_encoder(bits)
int bits;
{
    pgraph get_node_constraints();
	t_solution add_soln();

    char encoding[MAXDIM];
    int node, n_nodes, pow2_num, i;
    pgraph graph[MAXNODE];
    t_dag dag[MAXNODE];
    t_solution soln;

    if (bits > MAXDIM) {
        fprintf(stderr,"\nNot enough memory allocated in out_encoder.h\n");
        fprintf(stderr,"Please increase the memory allocated by: \n");
        fprintf(stderr,"#define MAXDIM     8+1 \n");
        fprintf(stderr,"#define MAXPOW2DIM 256 \n");
	exit(-1);
    }

	iter = 0;
	enc_iter = 0;
    n_nodes = read_constraints(graph, dag);
	soln.dim = bits;
    powers(soln.dim, &pow2_num);
    for (i = 0; i < n_nodes; i++) {
	    flag[i] = NOT_DONE;
	}
    for (i = 0; i < n_nodes; i++) {
	node = get_next_node(dag, n_nodes);
	while (try_encoding(graph[node], soln, pow2_num, encoding)
	       == FAIL) {
		change_graph(graph, n_nodes);
		update_dag(graph, dag, n_nodes);
	}
	flag[node] = DONE;
	update_dag(graph, dag, n_nodes);
	soln = add_soln(soln, node, encoding);
    }
    print_soln(soln, n_nodes);
}

powers(num, num_pow2)
    int num, *num_pow2;
{
    int i, j;

    for (i = 0, j = 1; i < num; i ++, j *= 2);
    *num_pow2 = j;
}

/*
 * format of constraint file <cover_node_name> <cover_node>: <covered_node> ... 
 */
read_constraints(graph, dag)
    pgraph graph[MAXNODE];
    t_dag dag[MAXNODE];
{
    FILE *fpin;
    char line[MAXLINE];
    int i, num, index;
    int count;
    pgraph new_node;

    if ((fpin = fopen(constraints, "r")) == NULL) {
	fprintf(stderr,"Error in opening file constraints\n");
	exit(1);
    }
    count = 0;
    while (fgets(line, MAXLINE, fpin) != NULL) {
	i = 0;
	index = getnum(line, &i);
	count++;
	graph[index] = NULL;
	while ((num = getnum(line, &i)) != EOF) {

	    /* insert into constraint graph */
	    new_node = (pgraph) malloc(sizeof(struct g_type));
	    new_node->num = num;
	    new_node->next = graph[index];
	    graph[index] = new_node;
	}
    }
    update_dag(graph, dag, count);
    fclose(fpin);

    return (count);
}

update_dag(graph, dag, count)
pgraph graph[MAXNODE];
t_dag dag[MAXNODE];
int count;
{
	int i;
	pgraph p;

	/* update the dag info fields */
	for (i = 0; i < count; i ++) {
		 dag[i].depth = 0;
		 dag[i].incard = 0;
		 dag[i].outcard = 0;
		 dag[i].out_done = 0;
    }

	for (i = 0; i < count; i ++) {
		for (p = graph[i]; p != NULL; p = p->next) {
			if (flag[p->num] == DONE) {
				dag[i].out_done ++;
            }
			dag[i].outcard ++;
			dag[p->num].incard ++;
			if (dag[i].depth >= dag[p->num].depth)
				dag[p->num].depth = 1 + dag[i].depth;
		}
	}
}

getnum(line, ind)
    char *line;
    int *ind;
{

    int i;
    char ch, str[MAXLINE];

    while (((ch = *(line + *ind)) == ' ') || (ch == ':'))
	(*ind)++;
    if ((ch == '\n') || (ch == EOS))
	return (EOF);
    for (i = 0; isdigit(line[*ind]); (*ind)++)
	str[i++] = line[*ind];
    str[i] = EOS;
    return (atoi(str));
}

/* remove edges to already encoded nodes */
change_graph(graph, n_nodes)
	pgraph graph[MAXNODE];
	int n_nodes;
{

	pgraph p, prev;
	int i, found = FALSE;

	for (i = 0; i < n_nodes; i++) {
		if (flag[i] == DONE)
			continue;
    	for (p = graph[i], prev = NULL; p != NULL; p = p->next) {
    		if (flag[p->num] == DONE) {
                found = TRUE;
				if (prev == NULL)
					graph[i] = NULL;
				else
					prev->next = p->next;
    		}
    	    prev = p;
    	}
    }

	/* if no edges deleted remove any one edge from node with max cardinality */
	if (!found) {
		fprintf(stderr,"Achtung : Reached the unreachable\n");
		exit(-1);
	}
}
	
/*
 * find the next node to be encoded - it should have no outgoing edges it is
 * then deleted from the graph 
 */
get_next_node(dag, n_nodes)
t_dag dag[MAXNODE];
int n_nodes;

{

    int consider[MAXNODE], card[MAXNODE];
    int i = 0, max_so_far;

    for (i = 0; i < n_nodes; i++) {
	consider[i] = FALSE;
	card[i] = 0;
	if (flag[i] == NOT_DONE) {
	    if (dag[i].outcard == dag[i].out_done) {
		consider[i] = TRUE;
	    }
	}
    }

    iter++;

    /* return the node with max out nodes done then max depth and then max 
	   incard */
    max_so_far = -1;
    for (i = 0; i < n_nodes; i++) {
	if (consider[i]) {
	    if (max_so_far == -1)
		max_so_far = i;
	    else if (dag[i].out_done > dag[max_so_far].out_done)
			max_so_far = i;
	    else if ((dag[i].out_done == dag[max_so_far].out_done) && 
				 (dag[i].depth > dag[max_so_far].depth))
		max_so_far = i;
	    else if ((dag[i].out_done == dag[max_so_far].out_done) && 
	             (dag[i].depth == dag[max_so_far].depth) && 
				 (dag[i].incard > dag[max_so_far].incard))
			max_so_far = i;
	}
    }
    if (max_so_far == -1) {
	fprintf(stderr,"No node found : Graph not acyclic");
	exit(-1);
    } else {
	return (max_so_far);
    }
    return(0);
}

/*
 * given the bit positions which should be set try using an encoding that has
 * not yet been used to cover this bit pattern. return FAIL on failure 
 */
try_encoding(constraint, soln, num, encoding)
    pgraph constraint;
    int num;
    t_solution soln;
    char encoding[MAXDIM];

{

    static int encodings_used[MAXPOW2DIM];
    static char enc[MAXPOW2DIM][MAXDIM];
    char set_bits[MAXDIM];
    int useful[MAXPOW2DIM], min_set, min_index, i, j;
    int one_kount, found;
    pgraph p;

    if (enc_iter == 0)
	for (i = 0; i < MAXPOW2DIM; i++)
	    encodings_used[i] = NOT_DONE;
    if (enc_iter == 0)
	for (i = 0; i < num; i++)
	    for (j = 0; j < soln.dim; j++)
		enc[i][j] = ((i >> j) & 1) ? ONE : ZERO;
    enc_iter++;

    for (i = 0; i < soln.dim; i++)
	set_bits[i] = ZERO;

    /* find all the bits that must be set using the constraint graph */
    for (p = constraint; p != NULL; p = p->next)
	for (i = 0; i < soln.dim; i++)
	    if (soln.node[p->num][i] == ONE)
		set_bits[i] = ONE;

    /* find the encoding still available with the fewest bits set */
    found = FALSE;
    for (i = 0; i < num; i++) {
	useful[i] = FALSE;
	if (encodings_used[i] == DONE)
	    continue;
	if (covers(enc[i], set_bits, soln.dim)) {
	    found = TRUE;
	    useful[i] = TRUE;
	}
    }
    if (found != TRUE)
	return (FAIL);
    min_set = -1;
    one_kount = 0;
    for (i = 0; i < num; i++)
	if (useful[i]) {
	    for (j = 0; j < soln.dim; j++)
		if (enc[i][j] == ONE)
		    one_kount++;
	    if ((min_set == -1) || (one_kount < min_set)) {
		min_set = one_kount;
		min_index = i;
	    }
	}
    for (j = 0; j < soln.dim; j++) {
	encoding[j] = enc[min_index][j];
	}
    encodings_used[min_index] = DONE;
    return (TRUE);
}

/* does the first string cover the second string bit-wise */
covers(a, b, dim)
    char *a, *b;
    int dim;
{

    int i;

    for (i = 0; i < dim; i++)
	if (b[i] == ONE)
	    if (a[i] != ONE)
		return (FALSE);
    return (TRUE);
}

t_solution
add_soln(soln, node, encoding)
    t_solution soln;
    int node;
    char encoding[MAXDIM];

{

    int i;

    for (i = 0; i < soln.dim; i++)
	soln.node[node][i] = encoding[i];
    return (soln);
}

print_soln(soln, num)
    t_solution soln;
    int num;
{

    int i;

    if (VERBOSE) printf("\nFINAL CODES\n");
    for (i = 0; i < num; i++)
	soln.node[i][soln.dim] = EOS;
    for (i=0; i<statenum; i++) {
         if (VERBOSE) printf("%s(%d) :  %s\n", states[i].name, i, soln.node[i]);
	     strcpy(states[i].exact_code,soln.node[i]);
    }

}
