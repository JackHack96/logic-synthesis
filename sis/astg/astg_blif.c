/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/astg/astg_blif.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:58 $
 *
 */
/* -------------------------------------------------------------------------- *\
   astg_blif.c -- Write BLIF description of astg flow graph.

   After an exhaustive token flow has been completed successfully, function
   astg_to_blif will write a BLIF description of the corresponding sis
   network.  Real pi/po names are preserved, and latches are generated for
   the feedback signals.  The naming convention is illustrated for an astg
   which has input a and output b:

			+-------+
		 a ---->|	|
			| logic +------> b_next  (fake PO)
     (fake PI)	b_ --+->|	|
		     |  +-------+
		     |
		     +-----------------> b

   A latch is connected from b_next to b_ and a don't care network is
   created for b_next.  If the internal names with the _ and _next suffixes
   would conflict with existing names, a number is appended to make them
   unique.  The latch type is specified as an argument to the function.

   Since additional synthesis steps will usually need to identify the two
   POs and single PI associated with an output signal, function
   astg_find_pi_or_po is provided to do this.
\* ---------------------------------------------------------------------------*/

#ifdef SIS
#include "astg_int.h"
#include "astg_core.h"

static astg_bool astg_uses_name (stg,name)
astg_graph *stg;
char *name;
{
    /*	Return true if astg has a signal with this name. */
    astg_generator gen;
    astg_signal *sig_p;
    astg_foreach_signal (stg,gen,sig_p) {
	if (!strcmp(name,sig_p->name)) return ASTG_TRUE;
    }
    return ASTG_FALSE;
}

static char *astg_make_fake_name (stg,name,suffix)
astg_graph *stg;
char *name;
char *suffix;
{
    /*	Generate name+suffix[+digits] which is not a signal name. */
    int real_len = strlen(name) + strlen(suffix);
    char *buffer = ALLOC (char,real_len + 10);
    int name_index = 0;
    strcat(strcpy(buffer,name), suffix);
    while (astg_uses_name(stg,buffer)) {
	sprintf(buffer+real_len,"%d",++name_index);
    }
    return buffer;
}

static void astg_print_inputs (fp,header,stg,fake_pis)
FILE *fp;
char *header;
astg_graph *stg;
char **fake_pis;
{
    /*	Print a .names line with all inputs. */
    astg_generator gen;
    astg_signal *sig_p;
    int sig_n = 0;
    fprintf(fp,"%s",header);
    astg_foreach_signal (stg,gen,sig_p) {
	if (astg_is_input(sig_p)) {
	    fprintf(fp," %s",sig_p->name);
	} else if (astg_is_noninput (sig_p) && fake_pis != NULL) {
	    fprintf(fp," %s",fake_pis[sig_n++]);
	}
    }
}

static void astg_print_real_outputs (fp,stg)
FILE *fp;
astg_graph *stg;
{
    /*	Print a .outputs command for all the real outputs. */
    astg_generator gen;
    astg_signal *sig_p;
    fprintf(fp,".outputs");
    astg_foreach_signal (stg,gen,sig_p) {
	if (astg_is_noninput(sig_p)) fprintf(fp," %s",sig_p->name);
    }
    fprintf(fp,"\n");
}

static void astg_print_cube (fp,stg,scode,fr)
FILE *fp;
astg_graph *stg;
astg_scode scode;
int fr;
{
    /*	Print cube in signal order, followed by 1 or 0 based on fr flag. */
    astg_generator gen;
    astg_signal *sig_p;
    astg_foreach_signal (stg,gen,sig_p) {
	if (astg_is_input(sig_p) || astg_is_noninput(sig_p)) {
	fprintf(fp,"%d",(sig_p->state_bit&scode)!=0);
	}
    }
    fprintf(fp," %d\n",fr);
}

extern void astg_to_blif (fp,stg,latch_type)
FILE *fp;
astg_graph *stg;
char *latch_type;
{
    /*	Write a sequential BLIF description of the token flow state graph.
	The network can be read in using read_blif of the SIS io package.
	Primary input and output names are preserved.  In addition, for each
	output variable x, a latch of the specified type is added with input
	x_next, and output x_.  Digits are appended if these generated names
	would conflict with existing io names. */

    char **fake_pi_names, **fake_po_names;
    astg_generator gen, tgen;
    astg_signal *sig_p;
    int sig_n = 0;
    char *dc_name;
    astg_state *state_p;
    astg_scode scode = 0;
    astg_scode enabled = 0;

    if (stg->flow_info.out_width == 0) {
	printf("warning: no noninput signals; no network generated.\n");
	return;
    }

    fake_pi_names = ALLOC(char *, stg->flow_info.out_width);
    fake_po_names = ALLOC(char *, stg->flow_info.out_width);

    astg_foreach_signal (stg,gen,sig_p) {
	if (astg_is_input(sig_p) || astg_is_dummy(sig_p)) continue;
	fake_pi_names[sig_n] = astg_make_fake_name (stg,sig_p->name,"_");
	fake_po_names[sig_n] = astg_make_fake_name (stg,sig_p->name,"_next");
	sig_n++;
    }
    assert (sig_n == stg->flow_info.out_width);

    /* Write model and list of real primary inputs and outputs. */
    fprintf(fp,".model %s\n", stg->name);
    astg_print_inputs (fp,".inputs",stg,NULL); fputs("\n",fp);
    astg_print_real_outputs(fp,stg);

    /* Write sis descriptions of latches. */
    sig_n = 0;
    astg_initial_state (stg,&scode);
    astg_foreach_signal (stg,gen,sig_p) {
        if (astg_is_input(sig_p)) continue;
		if (astg_is_noninput(sig_p)) {
        fprintf(fp,".latch %s %s %s NIL %d\n",
        		fake_po_names[sig_n], fake_pi_names[sig_n],
        		latch_type, (sig_p->state_bit&scode) != 0);
        sig_n++;
		}
    }

    sig_n = 0;
    astg_foreach_signal (stg,gen,sig_p) {
        if (astg_is_input(sig_p)) continue;
		if (astg_is_noninput(sig_p)) {
        astg_print_inputs (fp,".names",stg,fake_pi_names);
		fprintf(fp," %s\n",fake_po_names[sig_n]);
		astg_foreach_state (stg,tgen,state_p) {
			scode = astg_state_code (state_p);
			enabled = astg_state_enabled (state_p);
			if ((scode^enabled)&sig_p->state_bit) {
			astg_print_cube(fp,stg,scode,1);
			}
		}
		fprintf(fp,".names %s %s\n1 1\n",fake_pi_names[sig_n++],sig_p->name);
		}
    }

    fprintf(fp,".exdc\n");
    astg_print_inputs (fp,".inputs",stg,fake_pi_names);
    sig_n = 0;
    fputs("\n.outputs",fp);
    astg_foreach_signal (stg,gen,sig_p) {
	if (astg_is_noninput(sig_p)) fprintf(fp," %s",fake_po_names[sig_n++]);
    }
    fputs("\n",fp);
    dc_name = astg_make_fake_name (stg,"DC","");
    astg_print_inputs (fp,".names",stg,fake_pi_names);
    fprintf(fp," %s\n",dc_name);
    astg_foreach_state (stg,gen,state_p) {
	scode = astg_state_code(state_p);
	astg_print_cube (fp,stg,scode,0);
    }

    sig_n = 0;
    astg_foreach_signal (stg,gen,sig_p) {
	if (astg_is_input(sig_p)) continue;
	if (astg_is_noninput(sig_p)) {
	fprintf(fp,".names %s %s\n1 1\n",dc_name,fake_po_names[sig_n++]);
	}
    }

    fprintf(fp,".end\n");
    FREE (dc_name);
    for (sig_n=stg->flow_info.out_width; sig_n--; ) {
	FREE (fake_pi_names[sig_n]);
	FREE (fake_po_names[sig_n]);
    }
}

static node_t *astg_trace_fake_pi (node)
node_t *node;
{
    node_t *fake_pi = node;

    if (fake_pi != NULL) {
	do {
	    fake_pi = node_get_fanin (fake_pi,0);
	    if (fake_pi == NULL) return NULL;
	} while (node_function(fake_pi) == NODE_BUF);

	if (node_function(fake_pi) != NODE_PI
		|| network_is_real_pi(node_network(node),fake_pi)) {
	    fake_pi = NULL;
	}
    }
    return fake_pi;
}

extern node_t *astg_find_pi_or_po (network,sig_name,type)
network_t *network;
char *sig_name;
astg_io_enum type;
{
    /*	Find the primary input or output of the given type for a signal.  This
	assumes the network has a specific structure: any output which is used
	for feedback is connected to the fake PI for the SIS latch.  If no
	node matches the name/type, NULL is returned.  Possible io types are:
	   ASTG_REAL_PI, ASTG_REAL_PO, ASTG_FAKE_PI, ASTG_FAKE_PO. */

    node_t *pi_or_po, *node;
    name_mode_t old_mode;

    old_mode = name_mode;
    name_mode = LONG_NAME_MODE;
    pi_or_po = network_find_node (network,sig_name);
    name_mode = old_mode;

    if (pi_or_po == NULL) {
	node = NULL;
    }
    else if (type == ASTG_REAL_PI) {
	node = (network_is_real_pi(network,pi_or_po)) ? pi_or_po : NULL;
    }
    else if (!network_is_real_po(network,pi_or_po)) {
	/* All the other types are derived from a real PO. */
	node = NULL;
    }
    else if (type == ASTG_REAL_PO) {
	node = pi_or_po;
    }
    else if (type == ASTG_FAKE_PI) {
	node = astg_trace_fake_pi (pi_or_po);
    }
    else if (type == ASTG_FAKE_PO) {
	node = astg_trace_fake_pi (pi_or_po);
	if (node != NULL) {
	    node = network_latch_end (node);
	    if (node_function(node) != NODE_PO || network_is_real_po(network,node)) {
		node = NULL;
	    }
	}
    }

    return node;
}
#endif /* SIS */
