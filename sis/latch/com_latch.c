
#ifdef SIS
#include "sis.h"

static int
com_print_latch(network, argc, argv)
network_t **network;
int argc;
char **argv;
{    
    latch_t *l;
    latch_synch_t latch_type;
    node_t *p;
    node_t *control;
    array_t *node_vec;
    int i;
    lsGen gen;
    char *buf;
    int summary = 0;
    int c;

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "s")) != EOF) {
	switch(c) {
	  case 's':
	    summary = 1;
	    break;
	  default:
	    goto usage;
	}
    }

    node_vec = com_get_nodes(*network, argc-util_optind+1, argv+util_optind-1);
    if ((argc == 1) || ((argc == 2) && (summary))) {
	foreach_latch(*network, gen, l) {
	    latch_type = latch_get_type(l);
	    if (latch_type == ACTIVE_HIGH) buf = "ah";
	    if (latch_type == ACTIVE_LOW) buf = "al";
	    if (latch_type == RISING_EDGE) buf = "re";
	    if (latch_type == FALLING_EDGE) buf = "fe";
	    if (latch_type == COMBINATIONAL) buf = "co";
	    if (latch_type == ASYNCH) buf = "as";
	    if (latch_type == UNKNOWN) buf = "un";
	    (void) fprintf(sisout, "input: %s output: %s",
			   node_name(latch_get_input(l)),
			   node_name(latch_get_output(l)));
	    (void) fprintf(sisout, " init val: %d cur val: %d",
			   latch_get_initial_value(l),
			   latch_get_current_value(l));
	    if (!summary) {
		control = latch_get_control(l);
		if (control != NIL(node_t)) {
		    (void) fprintf(sisout, " type: %s control: %s", buf,
				   node_name(control));
		} else {
		    (void) fprintf(sisout, " type: %s control: none", buf);
		}
	    }
	    (void) fprintf(misout, "\n");
        }
        (void) array_free(node_vec);
        return 0;
    }
    for (i = 0; i < array_n(node_vec); i++){
        p = array_fetch(node_t *, node_vec, i);
	l = latch_from_node(p);
	if ((l = latch_from_node(p)) != NIL(latch_t)) {
	    latch_type = latch_get_type(l);
	    if (latch_type == ACTIVE_HIGH) buf = "ah";
	    if (latch_type == ACTIVE_LOW) buf = "al";
	    if (latch_type == RISING_EDGE) buf = "re";
	    if (latch_type == FALLING_EDGE) buf = "fe";
	    if (latch_type == COMBINATIONAL) buf = "co";
	    if (latch_type == ASYNCH) buf = "as";
	    if (latch_type == UNKNOWN) buf = "un";
	    (void) fprintf(misout, "input: %s output: %s",
			   node_name(latch_get_input(l)),
			   node_name(latch_get_output(l)));
	    (void) fprintf(misout, " init val: %d cur val: %d",
			   latch_get_initial_value(l),
			   latch_get_current_value(l));
	    if (!summary) {
		control = latch_get_control(l);
		if (control != NIL(node_t)) {
		    (void) fprintf(misout, " type: %s control: %s", buf,
				   node_name(control));
		} else {
		    (void) fprintf(misout, " type: %s control: none", buf);
		}
	    }
	    (void) fprintf(misout, "\n");
	} else {
	    (void) fprintf(misout,
			   "\tNode %s is not a latch input or output\n",
			   node_name(p));
	}
    }
    array_free(node_vec);
    return 0;

  usage:

    (void) fprintf(miserr, "usage: print_latch [-s] n1 n2 ... \n");
    return 1;
}
        

init_latch()
{
    com_add_command("print_latch", com_print_latch, 0);
}

end_latch()
{
}
#endif /* SIS */
