
#include "sis.h"
#include "io_int.h"

static int
com_read(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int i, c, append, single_output, status;
    char *fname, *error, *real_filename;
    network_t *new_network;
    FILE *fp;
#ifdef SIS
    graph_t *stg;
#endif /* SIS */

    append = 0;
    single_output = 1;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "acs")) != EOF) {
	switch(c) {
	case 'a':
	    append = 1;
	    break;
	case 's':
	    single_output = 1;
	    break;
	case 'c':
	    single_output = 0;
	    break;
	default:
	    goto usage;
	}
    }

    if (argc - util_optind == 0) {
	fname = "-";
    }
    else if (argc - util_optind == 1) {
	fname = argv[util_optind];
    }
    else {
	goto usage;
    }

    fp = com_open_file(fname, "r", &real_filename, /* silent */ 0);
    if (fp == NIL(FILE)) {
	FREE(real_filename);
	return 1;
    }

    error_init();
    read_register_filename(real_filename);
    new_network = 0;

    if (strcmp(argv[0], "read_blif") == 0) {
        read_lineno = 0;
	i = read_blif(fp, &new_network);
	if (i == EOF) {
	    (void) fprintf(siserr, "read_blif: no network found\n");
	    new_network = NIL(network_t);
	}
	else if (i != 1) {
	    new_network = NIL(network_t);
	}

    }
#ifdef SIS
    else if (strcmp(argv[0], "read_slif") == 0) {
        new_network = read_slif(fp);
    }
#endif /* SIS */
    else if (strcmp(argv[0], "read_eqn") == 0) {
	new_network = read_eqn(fp);
    }
    else if (strcmp(argv[0], "read_pla") == 0) {
	new_network = sis_read_pla(fp, single_output);
	if (new_network == NIL(network_t)) {
	    (void) fprintf(siserr, "read_pla: error reading PLA\n");
	}
    }
#ifdef SIS
    else if (strcmp(argv[0],"read_kiss") == 0) {
        read_lineno = 0;
	if (!read_kiss(fp, &stg)) {
	    (void) fprintf(siserr,"read_kiss: error reading\n");
	    new_network = NIL(network_t);
	}
	else if (stg != NIL(graph_t)) {
	    new_network = network_alloc();
            read_filename_to_netname(new_network, read_filename);
	    network_set_stg(new_network,stg);
	}
    }
#endif /* SIS */
    else {
	fail("com_read: called with bad argv[0]");
    }

    error = error_string();
    if (error[0] != '\0') {
	(void) fprintf(siserr, "%s", error);
    }
    if (new_network != NIL(network_t)) {
	if (append != 0) {
	    error_init();
	    if (network_append(*network, new_network) == 0) {
		(void) fprintf(siserr, "%s", error_string());
	    }
	    network_free(new_network);
	}
	else {
	    network_free(*network);
	    *network = new_network;
	}
	network_reset_short_name(*network);
	status = 0;
    }
    else {
	status = 1;
    }

    FREE(real_filename);
    read_register_filename(NIL(char));
    if (fp != stdin) {
        (void) fclose(fp);
    }
    return(status);

usage:
    (void) fprintf(siserr, "usage: %s [-a] [file]\n", argv[0]);
    (void) fprintf(siserr, "    -a\t\tappend to current network\n");
    if (strcmp(argv[0], "read_pla") == 0) {
	(void) fprintf(siserr, "    -s\t\tread PLA as single-output\n");
    }
    return(1);
}

static int
com_write(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c, short_name, net_list, status, delays_flag, wslif;
    FILE *fp;
    network_t *dc_network;

    short_name = 0;
    net_list = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "snd")) != EOF) {
	switch(c) {
	case 's':
	    short_name = 1;
	    break;
	case 'n':
	    net_list = 1;
	    break;
	case 'd':
	    delays_flag = 1;
	    break;
	default:
	    goto usage;
	}
    }

    /* open the destination file */
    if (argc - util_optind == 0) {
	fp = com_open_file("-", "w", NIL(char *), /* silent */0);
    }
    else if (argc - util_optind == 1) {
	fp = com_open_file(argv[util_optind], "w", NIL(char *), /* silent */0);
    }
    else {
	goto usage;
    }
    if (fp == NIL(FILE)) {
        return(1);
    }

    /* allow error reporting */
    error_init();
    dc_network = network_dc_network(*network);
    
    if (strcmp(argv[0], "write_blif") == 0) {
	write_blif(fp, *network, short_name, net_list);
	status = 1;
    }
    else if (strcmp(argv[0], "write_eqn") == 0) {
#ifdef SIS
	if (network_num_latch(*network)) {
	    (void) fprintf(sisout, "Warning: only combinational portion is being written.\n");
	}
#endif /* SIS */
	write_eqn(fp, *network, short_name);
	if (dc_network != NIL(network_t)) {
	    io_fputs_break(fp, "\nDon't care:\n");
	    write_eqn(fp, dc_network, short_name);
	    io_fputc_break(fp, '\n');
	}
	status = 1;
    }
#ifdef SIS
    else if (strcmp(argv[0], "write_slif") == 0) {
        (void) fprintf(siserr, "WARNING:\n\n");
/*	(void) fprintf(siserr,
"The following messages reflect inadequacies of SLIF, not any user fault.\n\n");*/
        if (delays_flag == 0) {
	    (void) fprintf(siserr,
"   Delay information is not being printed because a standard has not been\n");

	    (void) fprintf(siserr,
"established for SLIF. Specify -d on the command line to force printing of delays.\n\n");
	}
	(void) fprintf(siserr,
"   Clock events have no equivalent SLIF structure and will not be printed.\n");
	(void) fprintf(siserr,
"Synch edges, which depend on clock edges, will also not be printed.\n\n");

	(void) fprintf(siserr,
"   SLIF supports only rising edge D flip-flops. Every latch is printed as a\n");
	(void) fprintf(siserr,
"rising edge D-ff.  Also, SLIF does not store the reset state for a latch.\n\n");

        write_slif(fp, *network, short_name, net_list, delays_flag);
	if (dc_network != NIL(network_t)) {
	    io_fputs_break(fp, "\nDon't care:\n");
	    write_slif(fp, dc_network, short_name, net_list, 0);
	    io_fputc_break(fp, '\n');
	}
	status = 1;
    }
#endif /* SIS */    
    else if (strcmp(argv[0], "write_pla") == 0) {
#ifdef SIS
	if (network_num_latch(*network)) {
	    (void) fprintf(sisout, "Warning: only combinational portion is being written.\n");
	}
#endif /* SIS */
	write_pla(fp, *network);
	status = 1;
    }
    else if (strcmp(argv[0], "write_bdnet") == 0) {
	status = write_bdnet(fp, *network);
    }
#ifdef SIS
    else if (strcmp(argv[0],"write_kiss") == 0) {
        status = write_kiss(fp,network_stg(*network));
    }
#endif /* SIS */
    else {
	(void) fprintf(siserr, " ... how did I get here ?\n");
	return(1);
    }

    if (fp != stdout) {
	(void) fclose(fp);
    }

    if (status == 0) {
	(void) fprintf(siserr, "%s", error_string());
	return(1);
    }
    return(0);

usage:
    (void) fprintf(siserr, "usage: %s [-s] [filename]\n", argv[0]);
    (void) fprintf(siserr, "    -s\t\tuse network short-names\n");
    wslif = strcmp(argv[0], "write_slif");
    if (strcmp(argv[0], "write_blif") == 0 || wslif == 0) {
	(void) fprintf(siserr, "    -n\t\tuse net-list format when possible\n");
    }
    if (wslif == 0) {
        (void) fprintf(siserr,
		"    -d\t\tprint the slif-format delay information\n");
    }
    return(1);
}


/* Generates pds descriptions for single output CLBs, i.e. LUTs. */

#define node_short_name(n) n->short_name
extern char *node_decide_name();

static int
com_pds(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c, short_name, combinational_flag;
    FILE *fp;
    /* Init */
    error_init();
    util_getopt_reset();
    debug = 0;
    short_name = 0;
    combinational_flag = 0;
    name_mode = LONG_NAME_MODE;
    /* Default params */

    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "cds")) != EOF) {
	switch (c) {
	case'c':
	  combinational_flag = 1;
	  break;
	case'd':
	  debug = 1;
	  break;  
	case's':
	  short_name = 1;
	  name_mode = SHORT_NAME_MODE;
	  break;
        default:
	  goto usage;
	}
    }
    if(argc - util_optind == 0){
        fp = com_open_file("-", "w", NIL(char *), /* silent */0);
    }else if((argc - util_optind) == 1) {
        fp = com_open_file(argv[util_optind], "w", NIL(char *), /* silent */0);
    } else {
        goto usage;
    }

    (void)write_palasm(fp, *network, short_name, combinational_flag);
    if((argc - util_optind) == 1){
        fclose(fp);
    }
    return 0;

    usage:
        (void)fprintf(sisout,"usage: %s [-csd] [filename]\n", argv[0]);
	(void)fprintf(sisout,"-c\t\ttreat the network as a combinational one - do not print latch eqns\n");
	(void)fprintf(sisout,"-s\t\tuse network short-name\n");
    return 1;
}


write_palasm(fp, network, short_name, combinational_flag)
FILE *fp;
network_t *network;
int short_name, combinational_flag; /* combinational_flag = 1 =>  do not write out latch equations */
{
    node_t *po, *pi, *node;
    lsGen gen;
    int i;
#ifdef SIS
    latch_t *latch;
    node_t *input, *output;
    char *input_name,  *output_name;
#endif

    (void)fprintf(fp, "CHIP dummy LCA\n");
    (void)fprintf(fp,"\n");
    
    (void)fprintf(fp, ";PINLIST\n");
    i = 0;
    
    foreach_primary_input(network, gen, pi){
#ifdef SIS
        if (!combinational_flag) {
            if (!network_is_real_pi(network, pi)) continue;
        }
#endif
        (void)fprintf(fp, "%s ", node_decide_name(pi, short_name));
	    i++;
	if( i > 9){
	    (void)fprintf(fp, "\n");
	    i = 0;
	}
    }

    foreach_primary_output(network, gen, po){
#ifdef SIS
        if (!combinational_flag) {
            if (!network_is_real_po(network, po)) continue;
        }
#endif
        (void)fprintf(fp, "%s ", node_decide_name(po, short_name));
        i++;
	if( i > 9){
	    (void)fprintf(fp, "\n");
	    i = 0;
	}
    }
    
    (void)fprintf(fp, "\n");
#ifdef SIS
    if (!combinational_flag) {
        if (network_num_latch(network) > 0) {
            (void) fprintf(fp, "clock\n");
        }
    }
#endif
    (void)fprintf(fp, "EQUATIONS\n");
    
    foreach_node(network, gen, node){
        (void)write_pds_sop(fp, node, short_name);
    }
#ifdef SIS
    if (!combinational_flag) {
        /* write out the flip-flop equations */
        /*-----------------------------------*/
        foreach_latch(network, gen, latch) {
            input = latch_get_input(latch);
            output = latch_get_output(latch);
            input_name = node_decide_name(input, short_name);
            output_name = node_decide_name(output, short_name);        
            fprintf(fp, "%s := %s\n", output_name, input_name);
            fprintf(fp, "%s.CLKF = clock\n", output_name);
        }
    }
#endif
        
}

write_pds_sop(fp, node, short_name)
FILE *fp;
node_t *node;
int short_name;
{
    int first_cube, first_literal;
    int i, j;
    node_cube_t cube;
    node_literal_t literal;
    node_t *fanin;
    extern int io_node_should_be_printed();
       
    if (io_node_should_be_printed(node) == 0) {
        return;
    }

    (void)fprintf(fp, "%s = ", node_decide_name(node, short_name));
    switch(node_function(node)){
    case NODE_0:
        (void)fprintf(fp, "GND\n");
	return;
    case NODE_1:
	(void)fprintf(fp, "VCC\n");
	return;
    case NODE_PO:
	(void)fprintf(fp, "%s\n", 
		      node_decide_name(node_get_fanin(node, 0), short_name));
	return;
      }
    first_cube = 1;
    for(i = node_num_cube(node)-1; i >= 0; i--){
        if(!first_cube){
	    (void)fprintf(fp, " + ");
	}
	cube = node_get_cube(node, i);
	
	first_literal = 1;
	foreach_fanin(node, j, fanin){
	    literal = node_get_literal(cube, j);
	    switch(literal){
	    case ZERO:
	    case ONE:
	      if(!first_literal){
		  (void)fprintf(fp, " * ");
	      }
	      if(literal == ZERO) (void)fprintf(fp, "/");
	      (void)fprintf(fp, "%s", node_decide_name(fanin, short_name));
	      first_literal = 0;
	      break;
	    }
	
	}
	first_cube = 0;
    }
    (void)fprintf(fp, "\n");
}
	        
	    
int
po_fanout_count(n)
node_t *n;
{
    node_t *fo;
    lsGen gen;
    int po_count;

    po_count = 0;
    foreach_fanout(n, gen, fo){
        if(node_function(fo) == NODE_PO) po_count++;
    }
    return po_count;
}


char *
node_decide_name(node, short_name)
node_t *node;
int short_name;
{
    extern char *io_name();

    return io_name(node, short_name);
}

static int
com_print(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    network_t *net, *dc_net;
    node_t *node, *po, *pin, *pnode;
	node_t *fanin;
    array_t *node_vec;
    int c, i, negative, print_dc;
	int po_cnt;
	lsGen gen;
	char *name;

    negative = 0;
    print_dc = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "nd")) != EOF) {
	switch(c) {
	case 'n':
	    negative = 1;
	    break;
	case 'd':
	    print_dc = 1;
	    break;
	default:
	    goto usage;
	}
    }

    if (print_dc) {
        dc_net = network_dc_network(*network);
        if (dc_net == NIL(network_t)) {
            return(0);
        }
    	net = network_dup(dc_net);
#ifdef SIS
    	foreach_primary_output(*network, gen, po){
    		if (network_is_real_po(*network, po))
    			continue;
            if (!(node = network_find_node(net, po->name)))
    			continue;
            if (node_function(node) != NODE_PO)
    			continue;
    		pin = po->fanin[0];
    		if (node_function(pin) == NODE_PI){
    			fanin = node->fanin[0];
    			network_delete_node(net, node);
    			if (node_num_fanout(fanin) == 1)
    			    network_delete_node(net, fanin);
    			continue;
            }
            po_cnt = io_rpo_fanout_count(pin, &pnode);
    		if (po_cnt != 0){
    			fanin = node->fanin[0];
    			network_delete_node(net, node);
    			if (node_num_fanout(fanin) == 0)
    			    network_delete_node(net, fanin);
    			continue;
    		}
    		name = util_strsav(pin->name);
          	if (!network_find_node(net, name)) {
              	    /* If the name were in the dc_network, that means that a
                       DC function for this latch was already found.  The user
                       can give two constructs ".latch a b" ".latch a c" to
                       create two latches, but can only give one DC set for
                       these two latches.  extract_seq_dc will produce a DC
                       set for each of these latches, but the DC set will be
 		       the same. */
                    network_change_node_name(net, node, name);
           	} else {
		    network_delete_node(net, node);
		}
    	}
#endif /* SIS */
    } else {
        net = *network;
    }
    node_vec = com_get_nodes(net, argc-util_optind+1, argv+util_optind-1);
    if (array_n(node_vec) < 1) {
        goto usage;
    }

    for (i = 0; i < array_n(node_vec); i++) {
	node = array_fetch(node_t *, node_vec, i);
	if (negative) {
	    node_print_negative(sisout, node);
	}
	else {
	    node_print(sisout, node);
	}
    }
    array_free(node_vec);
	if (print_dc){
		network_free(net);
	}

    return(0);

usage:
    (void) fprintf(siserr, "usage: print [-d] [-n] n1 n2 ...\n");
    return 1;
}

/* ARGSUSED */
com_print_stats(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    network_t *net;
    node_t *p, *node;
    int i, c, pi, po, nodes, lits, lits_fac, factored_lits, print_dc;
#ifdef SIS
    int latches, ns;
    graph_t *g = NIL(graph_t);
#endif /* SIS */
    lsGen gen;
    array_t *node_vec;

    factored_lits = 0;
    print_dc = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "fd")) != EOF) {
	switch(c) {
	case 'f':
	    factored_lits = 1;
	    break;
	case 'd':
	    print_dc = 1;
	    break;
	default:
	    goto usage;
	}
    }

    if (print_dc != 0) {
        net = network_dc_network(*network);
	if (net == NIL(network_t)) {
	    return(0);
	}
    }
    else {
        net = *network;
    }

    if (argc - util_optind >= 1) {
	node_vec = com_get_nodes(net, argc-util_optind+1, argv+util_optind-1);
	if (array_n(node_vec) < 1) {
	    goto usage;
	}
	for (i = 0; i < array_n(node_vec); i++) {
	    node = array_fetch(node_t *, node_vec, i);
	    if (node_type(node) == PRIMARY_INPUT) {
		(void) fprintf(sisout, "%-10s (primary input)\n", 
		    node_name(node));
	    }
	    else if (node_type(node) == PRIMARY_OUTPUT) {
		(void) fprintf(sisout, "%-10s (primary output)\n", 
		    node_name(node));
	    }
	    else {
		(void) fprintf(sisout, "%-10s %d terms, %d literals\n", 
		    node_name(node), node_num_cube(node), 
		    node_num_literal(node));
	    }
	}
	array_free(node_vec);

    }
    else {
	pi = network_num_pi(net);
	po = network_num_po(net);
	nodes = network_num_internal(net);
#ifdef SIS
	latches = network_num_latch(net);
	if ((g = network_stg(net)) != NIL(graph_t)) {
	    ns = stg_get_num_states(g);
	    pi = stg_get_num_inputs(g);
	    po = stg_get_num_outputs(g);
	} else {
	    pi = pi - latches;
	    po = po - latches;
 	}
#endif /* SIS */

	lits = 0;
	lits_fac = 0;
	foreach_node (net, gen, p) {
	    lits += node_num_literal(p);
	    if (factored_lits) {
		lits_fac += factor_num_literal(p);
	    }
	
	}

#ifdef SIS
	(void) fprintf(sisout, 
	    "%-14s\tpi=%2d\tpo=%2d\tnodes=%3d\tlatches=%2d\nlits(sop)=%4d",
	    network_name(net), pi, po, nodes, latches, lits);
#else
	(void) fprintf(sisout, 
	    "%-14s\tpi=%2d\tpo=%2d\tnodes=%3d\tlits(sop)=%4d",
	    network_name(net), pi, po, nodes, lits);
#endif /* SIS */

	if (factored_lits) {
	    (void) fprintf(sisout, "\tlits(fac)=%4d", lits_fac);
	}
#ifdef SIS
	if (g != NIL(graph_t)) {
	    (void) fprintf(sisout, "\t#states(STG)=%4d", ns);
        }
#endif
	(void) fprintf(sisout, "\n");
    }
    return(0);

usage:
    (void) fprintf(siserr, "print_stats [-d] [-f] [n1 n2 ...]\n");
    return(1);
}

static int
com_print_io(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    network_t *net;
    lsGen gen;
    node_t *node, *p;
	node_t *fanin, *pnode;
	int po_cnt;
    array_t *node_vec;
    int c, i, j, print_dc;

    print_dc = 0;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "d")) != EOF) {
        switch(c) {
	case 'd':
	    print_dc = 1;
	    break;
	default:
	    goto usage;
	}
    }

    if (print_dc) {
        net = network_dc_network(*network);
	if (net == NIL(network_t)) {
	    return(0);
	}
    }
    else {
        net = *network;
    }
    
    if (argc - util_optind >= 1) {
	node_vec = com_get_nodes(net, argc-util_optind+1, argv+util_optind-1);
	for (i = 0; i < array_n(node_vec); i++) {
	    node = array_fetch(node_t *, node_vec, i);
	    (void) fprintf(sisout, "%s\n", node_name(node));
	    (void) fprintf(sisout, "   inputs:  ");
	    foreach_fanin(node, j, p) {
		(void) fprintf(sisout, " %s", node_name(p));
	    }
	    (void) fprintf(sisout, "\n");
	    (void) fprintf(sisout, "   outputs: ");
	    foreach_fanout (node, gen, p) {
		(void) fprintf(sisout, " %s", node_name(p));
	    }
	    (void) fprintf(sisout, "\n");
	}
	array_free(node_vec);
    }
    else {
	(void) fprintf(sisout, "primary inputs:  ");
	foreach_primary_input (net, gen, p) {
	    (void) fprintf(sisout, " %s", node_name(p));
	}
	(void) fprintf(sisout, "\n");
	(void) fprintf(sisout, "primary outputs: ");
    foreach_primary_output (net, gen, p) {
    	if (print_dc){
    		if (node = network_find_node(*network, p->name)){
    			fanin = node->fanin[0];
#ifdef SIS
    	        if (!network_is_real_po(*network, node)){
                po_cnt = io_rpo_fanout_count(fanin, &pnode);
            	if ((po_cnt != 0) && (node_function(fanin) != NODE_PI)){
                    (void) fprintf(sisout, " {%s} (latch input)", pnode->name);
            		continue;
            	}
    	        }
#endif /* SIS */
                (void) fprintf(sisout, " %s", node_name(node));
    			continue;
            }
            (void) fprintf(sisout, " \n Error: %s not in care network \n", node_name(p));
    		continue;
    	}
    	fanin = p->fanin[0];
#ifdef SIS
    	if (!network_is_real_po(net, p)){
        po_cnt = io_rpo_fanout_count(fanin, &pnode);
    	if ((po_cnt != 0)){
            (void) fprintf(sisout, " {%s} (latch input)", pnode->name);
    		continue;
    	}
    	}
#endif /* SIS */
        (void) fprintf(sisout, " %s", node_name(p));
    }
	(void) fprintf(sisout, "\n");
    }
    return(0);
usage:
    (void) fprintf(siserr, "print_io [-d] [n1 n2 ...]\n");
    return(1);
}

/* ARGSUSED */
static int
com_chname(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    if (argc != 1) {
	(void) fprintf(siserr, "usage: chname\n");
	return(1);
    }

    if (name_mode == LONG_NAME_MODE) {
	name_mode = SHORT_NAME_MODE;
	(void) fprintf(sisout, "changing to short-name mode\n");
    }
    else {
	name_mode = LONG_NAME_MODE;
	(void) fprintf(sisout, "changing to long-name mode\n");
    }
    return(0);
}

static int
com_reset_name(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int c, short_names, long_names;

    short_names = long_names = 1;
    util_getopt_reset();
    while ((c = util_getopt(argc, argv, "sl")) != EOF) {
	switch(c) {
	case 's':
	    long_names = 0;
	    break;
	case 'l':
	    short_names = 0;
	    break;
	default:
	    goto usage;
	}
    }
    if (argc != util_optind) goto usage;

    if (short_names) {
	network_reset_short_name(*network);
    }
    if (long_names) {
	network_reset_long_name(*network);
    }
    return(0);

usage:
    (void) fprintf(siserr, "usage: reset_names [-sl]\n");
    (void) fprintf(siserr, "\t-s reset only short names\n");
    (void) fprintf(siserr, "\t-l reset only long, made-up names\n");
    return(1);
}

/* ARGSUSED */
static int
com_print_altname(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    array_t *node_vec;
    node_t *node;
    int i;

    if (argc < 2) {
	(void) fprintf(siserr, "usage: print_altname n1 n2 ...\n");
	return(1);
    }

    node_vec = com_get_nodes(*network, argc, argv);
    for (i = 0; i < array_n(node_vec); i++) {
	node = array_fetch(node_t *, node_vec, i);
	(void) fprintf(sisout, "%s = %s\n", node->name, node->short_name);
    }
    array_free(node_vec);
    return(0);
}

/* ARGSUSED */
com_bdsyn(old_network, argc, argv)
network_t **old_network;		/* never used ... */
int argc;
char **argv;
{
    lsGen gen;
    int i, min_node;
    network_t *network;
    node_t *node;

    if (argc == 1) {
	min_node = 1;
    }
    else if (argc == 2) {
	min_node = 0;
    }
    else {
	(void) fprintf(siserr, "usage: bdsyn [1]\n");
	(void) fprintf(siserr, "\t\t1 implies don't minimize each node\n");
	return 1;
    }

    error_init();
    read_register_filename("(stdin)");
    network = 0;
    do {
	i = read_blif(stdin, &network);
	if (i == EOF) {
	    break;
	}
	else if (i != 1) {
	    (void) fprintf(siserr, "%s", error_string());
	    return 1;
	}

	(void) network_collapse(network);
	if (min_node) {
	    foreach_node(network, gen, node) {
		if (node_type(node) == INTERNAL) {
		    (void) node_simplify_replace(node, 
					NIL(node_t), NODE_SIM_ESPRESSO);
		}
	    }
	}
	write_blif_for_bdsyn(stdout, network);
	network_free(network);
	(void) fflush(stdout);
    } while (! feof(stdin));

    return(0);
}


 /* 
  *----------------------------------------------------------------------
  *
  * com_force_init_state_to_zero -- INTERNAL ROUTINE
  *
  * Forces the initial state to be all zeroes
  * by adding inverters around latches initially at 1
  * Very handy when latches cannot be initialized at 1
  * in a given technology (e.g. certain types of FPGAs).
  *
  * Results:
  *
  * Side effects:
  *	removes the don't care network if any.
  *
  *----------------------------------------------------------------------
  */

static int invert_polarity();
static void free_dc_network();

#ifdef SIS
static int com_force_init_state_to_zero(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    latch_t *latch;
    lsGen gen;
    array_t *node_vec;
    int value, status;
    node_t *pi, *po;

    node_vec = array_alloc(node_t *, 0);
    foreach_latch(*network, gen, latch) {
	value = latch_get_initial_value(latch);
	if (value == 0) continue;
	pi = latch_get_output(latch);
	array_insert_last(node_t *, node_vec, pi);
	po = latch_get_input(latch);
	array_insert_last(node_t *, node_vec, po);
	latch_set_initial_value(latch, 0);
    }
    status = invert_polarity(*network, node_vec);
    array_free(node_vec);
    network_sweep(*network);
    free_dc_network(*network);
    return status;
}
#endif /* SIS */

static void free_dc_network(network)
network_t *network;
{
    network_free(network->dc_network);
    network->dc_network = NIL(network_t);
}


 /* inverts the polarity of the PI/PO in 'node_vec' */

static int invert_polarity(network, node_vec)
network_t *network;
array_t *node_vec;
{
    int i;
    node_t *inv, *node;
    lsGen gen;
    node_t *fanout, *fanin;

    for (i = 0; i < array_n(node_vec); i++) {
	node = array_fetch(node_t *, node_vec, i);
	if (node->type == PRIMARY_INPUT) {
	    inv = node_literal(node, 0);
	    foreach_fanout(node, gen, fanout) {
		assert(node_patch_fanin(fanout, node, inv));
		node_scc(fanout);
	    }
	} else if (node->type == PRIMARY_OUTPUT) {
	    fanin = node_get_fanin(node, 0);
	    inv = node_literal(fanin, 0);
	    assert(node_patch_fanin(node, fanin, inv));
	    node_scc(node);
	} else {
	    (void) fprintf(siserr, "\"com_io.c:invert_polarity()\" called for node != PI or PO");
	    return 1;
	}
	network_add_node(network, inv);
    }
    return 0;
}


 /* 
  *----------------------------------------------------------------------
  *
  * Change the polarity (active high <-> active low) 
  * of specified external PIPO.
  *
  * Results:
  *
  * Side effects:
  *	Removes the don't care network if any.
  *
  *----------------------------------------------------------------------
  */

static int
com_invert_io(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
    int i, status;
    node_t *node;
    array_t *node_vec;

    if (argc < 2) goto usage;
    node_vec = com_get_true_io_nodes(*network, argc, argv);

    /* check input consistency */
    for (i = 0; i < array_n(node_vec); i++) {
	node = array_fetch(node_t *, node_vec, i);
#ifdef SIS
	if (! network_is_real_pi(*network, node) && ! network_is_real_po(*network, node)) {
#else
	if (node_function(node) != NODE_PI && node_function(node) != NODE_PO) {
#endif /* SIS */
	    (void) fprintf(siserr, "Error: node %s is not an external PI or PO\n", node_name(node));
	    return 1;
	}
    }

    /* do the work */
    status = invert_polarity(*network, node_vec);

    /* cleanup */
    array_free(node_vec);
    network_sweep(*network);
    free_dc_network(*network);
    return status;

  usage:
    (void) fprintf(siserr, "usage: invert_io n1 n2 ...\n");
    return 1;
}



init_io()
{
    com_add_command("bdsyn", com_bdsyn, 0);
    com_add_command("read_blif", com_read, 1);
    com_add_command("read_pla", com_read, 1);
    com_add_command("read_eqn", com_read, 1);
#ifdef SIS
    com_add_command("read_kiss",com_read, 1);
    com_add_command("read_slif", com_read, 1);
#endif /* SIS */
    com_add_command("write_bdnet", com_write, 0);
    com_add_command("write_blif", com_write, 0);
    com_add_command("write_eqn", com_write, 0);
    com_add_command("write_pla", com_write, 0);
    com_add_command("write_pds", com_pds, 0);
#ifdef SIS
    com_add_command("write_kiss",com_write,0);
    com_add_command("write_slif", com_write, 0);
#endif /* SIS */
    com_add_command("print", com_print, 0);
    com_add_command("print_io", com_print_io, 0);
    com_add_command("print_stats", com_print_stats, 0);
    com_add_command("chng_name", com_chname, 0);
    com_add_command("reset_name", com_reset_name, 0);
    com_add_command("print_altname", com_print_altname, 0);
    com_add_command("invert_io", com_invert_io, 1);
#ifdef SIS
    com_add_command("force_init_0", com_force_init_state_to_zero, 1);
#endif /* SIS */

    if (com_graphics_enabled()) {
	com_add_command("plot_blif", com_plot_blif, 0);
    }
}

end_io()
{
    read_register_filename(NIL(char));
}

