/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/io/plot_blif.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:28 $
 *
 */
/*
 * plot_blif.c -- transmit blif data to graphical front end.
 *
 * Since BLIF is already easy to parse, the graphics data for the BLIF
 * representation of a network is just the BLIF output itself.
 */

#include "sis.h"
#include "io_int.h"

void io_plot_network (fp,network,internal)
FILE *fp;
network_t *network;
int internal;
{
    /*	Write a description of the network for plotting.  Use names which
	will be guaranteed unique for each node. */

    lsGen gen;
    int igen;
    char *long_name, *pretty_name;
    node_t *node, *fanin;
#ifdef SIS
    latch_t *latch;
#endif /* SIS */

    fprintf(fp,".model\t%s\n",network_name(network));

    fputs (".inputs",fp);
    foreach_primary_input (network,gen,node) {
#ifdef SIS
	if (network_is_real_pi(network,node)) {
#else
	if (node->type == PRIMARY_INPUT) {
#endif /* SIS */
	    fprintf(fp,"\t%s",node_long_name(node));
	}
    }
    fputs ("\n",fp);

    fputs (".outputs",fp);
    foreach_primary_output (network,gen,node) {
#ifdef SIS
	if (network_is_real_po(network,node)) {
#else
	if (node->type == PRIMARY_INPUT) {
#endif /* SIS */
	    fprintf(fp,"\t%s",node_long_name(node));
	}
    }
    fputs ("\n",fp);

    foreach_node (network,gen,node) {
	if (node_num_fanin(node) == 0) continue;
	long_name = node_long_name(node);
	pretty_name = node_name(node);
	fprintf(fp,".node\t%s",node_long_name(node));
	foreach_fanin (node,igen,fanin) {
	    fprintf(fp,"\t%s",node_long_name(fanin));
	}
	fputs ("\n",fp);
	if (!internal && strcmp(long_name,pretty_name) != 0) {
	    fprintf(fp,".label\t%s\t%s\n",long_name,pretty_name);
	}
    }

#ifdef SIS
    foreach_latch (network,gen,latch) {
	fanin = latch_get_input (latch);
	node = latch_get_output (latch);
	fprintf(fp,".latch\t%s\t%s\n",
		node_long_name(fanin), node_long_name(node));
    }
#endif /* SIS */
}

int com_plot_blif (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    int c, result = 0;
    int replace = 0, close = 0, internal = 0, dc_net = 0;
    char *geom = NULL;
    char *plot_name = network_name(*network_p);
    FILE *fp;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"n:dikrg:")) != EOF) {
      switch (c) {
	case 'i': internal = 1; break;
	case 'k': close = 1; break;
	case 'r': replace = 1; break;
	case 'g': geom = util_optarg; break;
	case 'n': plot_name = util_optarg; break;
	case 'd': dc_net = 1; break;
	default : result = 1; break;
      }
    }

    if (result || *network_p == NULL) {
	fprintf(sisout,"usage: %s [-n name] [-i] [-k] [-g geom] [-r]\n",argv[0]);
	fputs("    -k\t\tclose plot window.\n",sisout);
	fputs("    -r\t\treplace plot with updated network.\n",sisout);
	fputs("    -i\t\tuse internal node names.\n",sisout);
	fputs("    -g geom\tinitial geometry, WxH+X+Y formats\n",sisout);
	fputs("    -n name\tuse name instead of network name.\n",sisout);
	return result;
    }

    if (close) {
	com_graphics_exec ("blif",plot_name,"close", NIL(char));
    }
    else if (replace) {
	fp = com_graphics_open ("blif",plot_name,"replace");
	if (dc_net) {
	    io_plot_network(fp, (*network_p)->dc_network, internal);
	} else {
	    io_plot_network (fp, *network_p, internal);
	}
	com_graphics_close (fp);
    }
    else {
	fp = com_graphics_open ("blif",plot_name,"new");
	if (geom) fprintf(fp,".geometry\t%s\n",geom);
	if (dc_net) {
	    io_plot_network (fp, (*network_p)->dc_network, internal);
	} else {
	    io_plot_network (fp, *network_p, internal);
  	}
	com_graphics_close (fp);
    }

    return result;
}
