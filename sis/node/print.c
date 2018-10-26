
#include "sis.h"
#include "node_int.h"


static void node_print_internal();
static int simple_print();


void
node_print(fp, node)
FILE *fp;
node_t *node;
{
    node_t *node1;

    if (node->type == PRIMARY_INPUT) {
	return;
    } else if (node->type == PRIMARY_OUTPUT && 
	    node->fanin[0]->type != PRIMARY_INPUT) {
	return;
    }

    node1 = node_sort_for_printing(node);
    (void) fprintf(fp, "     %s = ", node_name(node));
    node_print_internal(fp, node1, 1);
    (void) fputc('\n', fp);
    node_free(node1);
}


void 
node_print_negative(fp, node)
FILE *fp;
node_t *node;
{
    node_t *node1, *node2;
    node_type_t type;

    type = node->type;
    if (type == PRIMARY_INPUT) {
	return;
    }
    if (type == PRIMARY_OUTPUT && node->fanin[0]->type != PRIMARY_INPUT) {
	return;
    }

    node2 = (type == PRIMARY_OUTPUT) ? node : node_not(node);
    node1 = node_sort_for_printing(node2);
    (void) fprintf(fp, "     %s = ", node_name(node));
    node_print_internal(fp, node1, 0);
    (void) fputc('\n', fp);
    node_free(node1);
    if (type != PRIMARY_OUTPUT) {
        node_free(node2);
    }
}


void
node_print_rhs(fp, node)
FILE *fp;
node_t *node;
{
    node_t *node1;

    node1 = node_sort_for_printing(node);
    node_print_internal(fp, node1, 1);
    node_free(node1);
}



static void
node_print_internal(fp, node, phase)
FILE *fp;
node_t *node;
int phase;
{
    int var, x, first_literal, first_pterm;
    pset last, p;
    node_t *fanin;

    if (simple_print(fp, node, phase)) return;

    if (! phase) {
	(void) fputc('(', fp);
    }

    first_pterm = 1;
    foreach_set(node->F, last, p) {
	if (! first_pterm) {
	    (void) fputs(" + ", fp);
	}
	first_pterm = 0;

	first_literal = 1;
	foreach_fanin(node, var, fanin) {
	    switch(x = GETINPUT(p, var)) {
	    case ZERO:
	    case ONE:
		if (! first_literal) {
		    (void) fputc(' ', fp);
		}
		first_literal = 0;
		(void) fputs(node_name(fanin), fp);
		if (x == ZERO) (void) fputc('\'', fp);
		break;

	    case TWO:
		break;
	    
	    default:
		fail("node_print: corrupt function");
	    }
	}
    }
    if (! phase) {
	(void) fputs(")'", fp);
    }
}


static int
simple_print(fp, node, phase)
FILE *fp;
node_t *node;
int phase;
{
    if (node->type == PRIMARY_INPUT) {
	return 1;

    } else if (node->type == PRIMARY_OUTPUT) {
	if (node->fanin[0]->type == PRIMARY_INPUT) {
	    (void) fprintf(fp, "%s", node_name(node->fanin[0]));
	}
	return 1;

    } else {
	switch(node_function(node)) {
	case NODE_0:
	    (void) fprintf(fp, "-%s-", phase ? "0" : "1");
	    return 1;

	case NODE_1:
	    (void) fprintf(fp, "-%s-", phase ? "1" : "0");
	    return 1;

	case NODE_BUF:
	    (void) fprintf(fp, "%s%s", 
		node_name(node->fanin[0]), phase ? "" : "'");
	    return 1;

	case NODE_INV:
	    (void) fprintf(fp, "%s%s", 
		node_name(node->fanin[0]), phase ? "'" : "");
	    return 1;

	default:
	    return 0;
	}
    }
}
