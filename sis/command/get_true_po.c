/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/command/get_true_po.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:18 $
 *
 */
#include "sis.h"

static char *get_arg();
static node_t *get_true_node_by_name(); 


array_t *
com_get_true_nodes(network, argc, argv)
network_t *network;
int argc;
char **argv;
{
    array_t *node_list;
    int i, j;
    char *arg;
    node_t *node, *p;
    lsGen gen;

    node_list = array_alloc(node_t *, 32);

    if (argc == 1) {
	foreach_node(network, gen, p) {
	    array_insert_last(node_t *, node_list, p);
	}

    } else {
	for(i = 1; i < argc; i++) {
	    if (strcmp(argv[i], "*") == 0) {
		foreach_node(network, gen, p) {
		    array_insert_last(node_t *, node_list, p);
		}

	    } else if (strncmp(argv[i], "i(", 2) == 0) {
		arg = get_arg(argv[i]);
		if (strcmp(arg, "") == 0) {
		    foreach_primary_input(network, gen, p) {
			array_insert_last(node_t *, node_list, p);
		    }
		} else {
		    node = get_true_node_by_name(network, arg);
		    if (node == NIL(node_t)) {
			(void) fprintf(miserr, "'%s' not found\n", arg);
		    } else {
			foreach_fanin(node, j, p) {
			    array_insert_last(node_t *, node_list, p);
			}
		    }
		}
		FREE(arg);

	    } else if (strncmp(argv[i], "o(", 2) == 0) {
		arg = get_arg(argv[i]);
		if (strcmp(arg, "") == 0) {
		    foreach_primary_output(network, gen, p) {
			array_insert_last(node_t *, node_list, p);
		    }
		} else {
		    node = get_true_node_by_name(network, arg);
		    if (node == NIL(node_t)) {
			(void) fprintf(miserr, "'%s' not found\n", arg);
		    } else {
			foreach_fanout(node, gen, p) {
			    array_insert_last(node_t *, node_list, p);
			}
		    }
		}
		FREE(arg);

	    } else {
		p = get_true_node_by_name(network, argv[i]);
		if (p == NIL(node_t)) {
		    (void) fprintf(miserr, "'%s' not found\n", argv[i]);
		} else {
		    array_insert_last(node_t *, node_list, p);
		}
	    }
	}
    }
    return node_list;
}


/* Performs just like com_get_true_nodes for MISII, but for SIS it retrieves
   only real pi/po nodes from i() and o().  That is, the latch pi and po
   nodes are filtered out. */

array_t *
com_get_true_io_nodes(network, argc, argv)
network_t *network;
int argc;
char **argv;
{
    array_t *node_list;
    int i, j;
    char *arg;
    node_t *node, *p;
    lsGen gen;

    node_list = array_alloc(node_t *, 32);

    if (argc == 1) {
	foreach_node(network, gen, p) {
	    array_insert_last(node_t *, node_list, p);
	}

    } else {
	for(i = 1; i < argc; i++) {
	    if (strcmp(argv[i], "*") == 0) {
		foreach_node(network, gen, p) {
		    array_insert_last(node_t *, node_list, p);
		}

	    } else if (strncmp(argv[i], "i(", 2) == 0) {
		arg = get_arg(argv[i]);
		if (strcmp(arg, "") == 0) {
		    foreach_primary_input(network, gen, p) {
#ifdef SIS
			if (network_is_real_pi(network, p))
#endif
			array_insert_last(node_t *, node_list, p);
		    }
		} else {
		    node = get_true_node_by_name(network, arg);
		    if (node == NIL(node_t)) {
			(void) fprintf(miserr, "'%s' not found\n", arg);
		    } else {
			foreach_fanin(node, j, p) {
			    array_insert_last(node_t *, node_list, p);
			}
		    }
		}
		FREE(arg);

	    } else if (strncmp(argv[i], "o(", 2) == 0) {
		arg = get_arg(argv[i]);
		if (strcmp(arg, "") == 0) {
		    foreach_primary_output(network, gen, p) {
#ifdef SIS
			if (network_is_real_po(network, p))
#endif
			array_insert_last(node_t *, node_list, p);
		    }
		} else {
		    node = get_true_node_by_name(network, arg);
		    if (node == NIL(node_t)) {
			(void) fprintf(miserr, "'%s' not found\n", arg);
		    } else {
			foreach_fanout(node, gen, p) {
			    array_insert_last(node_t *, node_list, p);
			}
		    }
		}
		FREE(arg);

	    } else {
		p = get_true_node_by_name(network, argv[i]);
		if (p == NIL(node_t)) {
		    (void) fprintf(miserr, "'%s' not found\n", argv[i]);
		} else {
		    array_insert_last(node_t *, node_list, p);
		}
	    }
	}
    }
    return node_list;
}


static char *
get_arg(strng)
char *strng;
{
    char *paren, *arg;

    paren = strchr(strng, '(');
    assert(paren != 0);
    arg = util_strsav(paren+1);
    arg[strlen(arg)-1] = '\0';
    return arg;
}


static node_t *
get_true_node_by_name(network, name)
network_t *network;
char *name;
{
    int found;
    char *dummy;
    node_t *node;

    if (name_mode == LONG_NAME_MODE) {
	found = st_lookup(network->name_table, name, &dummy);
    } else {
	found = st_lookup(network->short_name_table, name, &dummy);
    }
    if (found) {
	node = (node_t *) dummy;
	return node;
    } else {
	return NIL(node_t);
    }
}
