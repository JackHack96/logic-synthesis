/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/gcd/com_gcd.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:23 $
 *
 */
#include "sis.h"
#include "gcd.h"
#include "gcd_int.h"

static int
com_print_gcd(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int i;
    array_t *node_vec;
    node_t *node;

    node_vec = com_get_nodes(*network, argc, argv);
    if (array_n(node_vec) < 1) goto usage;

    for(i = 0; i < array_n(node_vec); i++) {
	node = array_fetch(node_t *, node_vec, i);
	if (node_function(node) == NODE_PI) goto usage;
	if (node_function(node) == NODE_1) {
	    (void)fprintf(misout,"-1-\n");
	    array_free(node_vec);
	    return 0;
	}
    }

    node = gcd_nodevec(node_vec);
    array_free(node_vec);
    node_print_rhs(misout,node);
    (void)fprintf(misout, "\n");
    return 0;
    
usage:
    (void) fprintf(miserr, 
	"_gcd n1 n2 ...\n");
    
    (void) fprintf(miserr, 
	"   where none of n1, n2,..., nn are primary inputs\n");
    return 1;
}

static int
com_print_factor(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int i, j;
    array_t *node_vec;
    array_t *factor_vec;
    node_t *node, *factor_node;

    node_vec = com_get_nodes(*network, argc, argv);
    if (array_n(node_vec) < 1) goto usage;

    for(i = 0; i < array_n(node_vec); i++) {
	node = array_fetch(node_t *, node_vec, i);
	factor_vec = gcd_prime_factorize(node);
	(void)
	 fprintf(misout, "Prime factorization for node %s\n", node_name(node));
	for(j = 0; j < array_n(factor_vec); ++j) {
	    factor_node = array_fetch(node_t *, factor_vec, j);
	    node_print_rhs(misout,factor_node);
	    node_free(factor_node);
	    (void)fprintf(misout,"\n");
	}
	array_free(factor_vec);
    }
	
	
    array_free(node_vec);
    return 0;
    
usage:
    (void) fprintf(miserr, 
	"_prime_factor n1 n2 ...\n");
    
    return 1;
}

init_gcd()
{
    com_add_command("_gcd", com_print_gcd, /* changes-network */ 0);
    com_add_command("_prime_factor", com_print_factor, /* changes-network */ 0);
}


end_gcd()
{
}
