
#include "sis.h"
#include "io_int.h"


static void
write_cover(fp, p)
FILE *fp;
node_t *p;
{
    register pset last, ps;
    register int c, i;

    if (p->type == PRIMARY_OUTPUT) {
	io_fputs_break(fp, "1 1\n");
    }
    else {
	foreach_set (p->F, last, ps) {
	    for (i = 0; i < p->nin; i++) {
		c = "?01-"[GETINPUT(ps,i)];
		io_fputc_break(fp, c);
	    }
	    io_fputs_break(fp, " 1\n");
	}
    }
}

int
io_po_fanout_count(node, first)
node_t *node;
node_t **first;
{
    register int po_cnt;
    register node_t *p;
    lsGen gen;

    po_cnt = 0;
    foreach_fanout(node, gen, p) {
	if (p->type == PRIMARY_OUTPUT) {
	    if (first != NIL(node_t *) && po_cnt == 0) {
	        *first = p;
	    }
	    po_cnt++;
	}
    }
    return po_cnt;
}

#ifdef SIS
int
io_rpo_fanout_count(node, first)
node_t *node;
node_t **first;
{
    register int rpo_cnt;
    register node_t *p;
    lsGen gen;
    network_t *network;

    network = node_network(node);
    rpo_cnt = 0;
    foreach_fanout(node, gen, p) {
	if (network_is_real_po(network, p) != 0) {
	    if (first != NIL(node_t *) && rpo_cnt == 0) {
	        *first = p;
	    }
	    rpo_cnt++;
	}
    }
    return rpo_cnt;
}

int
io_lpo_fanout_count(node, first)
node_t *node;
node_t **first;
{
    register int lpo_cnt;
    register node_t *p;
    lsGen gen;

    lpo_cnt = 0;
    foreach_fanout(node, gen, p) {
	if (p->type == PRIMARY_OUTPUT && network_latch_end(p) != NIL(node_t)) {
	    if (first != 0 && lpo_cnt == 0) {
	        *first = p;
	    }
	    lpo_cnt++;
	}
    }
    return lpo_cnt;
}
#endif /* SIS */

char *
io_name(node, short_flag)
node_t *node;
int short_flag;
{
    node_t *fo;
#ifdef SIS
    node_type_t type;
    network_t *network;
    
    type = node_type(node);
    network = node_network(node);

    if (type == PRIMARY_OUTPUT) {
        if (network_is_real_po(network, node) != 0) {
	    goto got_name;
	}
	else {
	    node = node_get_fanin(node, 0);
	    type = node_type(node);
	}
    }
    if (type == PRIMARY_INPUT) {
        if (network_is_real_pi(network, node) != 0) {
	    goto got_name;
	}
    }
    if (io_rpo_fanout_count(node, &fo) == 1) {
        node = fo;
    }
got_name:

#else
    if (node_type(node) == INTERNAL) {
        if (io_po_fanout_count(node, &fo) == 1) {
	    node = fo;
	}
    }
#endif /* SIS */

    return(short_flag ? node->short_name : node->name);
}

char *
io_node_name(node)
node_t *node;
{
    return(io_name(node, 0));
}

void
io_write_name(fp, node, short_flag)
FILE *fp;
node_t *node;
int short_flag;
{
    io_fputs_break(fp, io_name(node, short_flag));
}

/*
 * During file write, some nodes shouldn't be printed, because they are
 * artifacts of the sis network representation.
 *
 * 1. PIs are never printed
 * 2. If node is a PO, only print node if all of:
 *    a. node is a real PO (not a latch input).
 *    b. fanin of node is internal and the fanin feeds > 1 real POs
 *    c. fanin of node is a real PI feeding > 1 real POs.
 */
int
io_node_should_be_printed(node)
node_t *node;
{
    node_type_t type;
    node_t *fanin;
#ifdef SIS
    network_t *network;
    int rpo;
#endif /* SIS */
    
    type = node_type(node);

    /* never print inputs ... */
    if (type == PRIMARY_INPUT) {
        return(0);
    }

    if (type == PRIMARY_OUTPUT) {

	/* print outputs only if fed by inputs, or if fed by > 1 node */

#ifdef SIS
        network = node_network(node);

	/* never print latch inputs */
        if (network_is_real_po(network, node) == 0) {
	    return(0);
	}
	fanin = node_get_fanin(node, 0);
	type = node_type(fanin);

	rpo = io_rpo_fanout_count(fanin, NIL(node_t *));
	if (type != PRIMARY_INPUT) {
	    if (rpo == 1) {
	        return(0);
	    }
	}
	/* just a sec, is this a real PI? */
	else if (rpo <= 1 && network_is_real_pi(network, fanin) == 0) {
	    return(0);
	}

#else

	fanin = node_get_fanin(node, 0);
	if (node_type(fanin) != PRIMARY_INPUT) {
	    if (io_po_fanout_count(fanin, NIL(node_t *)) == 1) {
	        return(0);
	    }
	}

#endif /* SIS */
    }
    return(1);
}    

static void
io_write_func(fp, n, short_flag, mapped)
FILE *fp;
node_t *n;
int short_flag;
int mapped;
{
    int i;
    node_t *fanin;
    lib_gate_t *gate;
#ifdef SIS
    node_t *lpo;
    int mlatch;
#endif /* SIS */

    if (io_node_should_be_printed(n) == 0) {
        return;
    }
    gate = lib_gate_of(n);

    if (mapped == 0 || gate == NIL(lib_gate_t)) {
        io_fputs_break(fp, ".names");
	foreach_fanin(n, i, fanin) {
	    io_fputc_break(fp, ' ');
	    io_write_name(fp, fanin, short_flag);
	}
	io_fputc_break(fp, ' ');
	io_write_name(fp, n, short_flag);
	io_fputc_break(fp, '\n');

	write_cover(fp, n);
    }
    else {

#ifdef SIS
      mlatch = io_lpo_fanout_count(n, &lpo);
      if (mlatch == 0) {
        io_fputs_break(fp, ".gate ");
      }
      else {
        io_fputs_break(fp, ".mlatch ");
      }
      io_fputs_break(fp, lib_gate_name(gate));
      foreach_fanin (n, i, fanin) {
      	    /*
	     * Do not output internal feedback pin from Qnext
	     */
	if (lib_gate_latch_pin(gate) != i) {
	  io_fprintf_break(fp, " %s=%s", lib_gate_pin_name(gate, i, /*in*/ 1),
	    		io_name(fanin, short_flag));
	}
      }
      if (mlatch != 0) {	/* latch gate */
      	n = latch_get_output(latch_from_node(lpo));
      }

#else
      io_fprintf_break(fp, ".gate %s", lib_gate_name(gate));
      foreach_fanin (n, i, fanin) {
	io_fprintf_break(fp, " %s=%s", lib_gate_pin_name(gate, i, 1),
			io_name(fanin, short_flag));
      }
      

#endif /* SIS */

      io_fprintf_break(fp, " %s=%s", lib_gate_pin_name(gate, 0, /*in*/ 0),
		io_name(n, short_flag));
    }
}

void
io_write_node(fp, node, short_flag)
FILE *fp;
node_t *node;
int short_flag;
{
    io_write_func(fp, node, short_flag, 0);
}

void
io_write_gate(fp, node, short_flag)
FILE *fp;
node_t *node;
int short_flag;
{
    io_write_func(fp, node, short_flag, 1);
}


static int colnum;
static char *io_break_string = 0;
static int io_break_column = 32000;


/*
 * write a string to a file, keeping track of current column number;
 * force a newline to keep all lines < 80 columns (when possible)
 */

/*VARARGS2*/
void
io_fprintf_break(FILE *fp, char *fmt, ...)
{
    va_list args;
    char buf[MAX_WORD];

    va_start(args, fmt);
    (void) vsprintf(buf, fmt, args);
    io_fputs_break(fp, buf);
    va_end(args);
}

void
io_fputs_break(fp, s)
register FILE *fp;
register char *s;
{
    char *p;

    if (colnum+strlen(s) > io_break_column) {
	if (io_break_string != 0) {
	    for(p = io_break_string; *p; p++) {
		(void) putc(*p, fp);
		colnum = (*p == '\n') ? 0 : colnum + 1;
	    }
	}
    }
    for( ; *s; s++) {
	(void) putc(*s, fp);
	colnum = (*s == '\n') ? 0 : colnum + 1;
    }
}

void
io_fputc_break(fp, c)
FILE *fp;
int c;
{
    (void) putc(c, fp);
    colnum = (c == '\n') ? 0 : colnum + 1;
}


void
io_fput_params(s, count)
char *s;
int count;
{
    io_break_string = s;
    io_break_column = count;
    colnum = 0;
}



static int get_default_delay_param();
static int get_delay_param();

#ifdef SIS
static void write_synch_edge();
#endif /* SIS */

void
write_blif_slif_delay(fp, network, slif, short_flag)
FILE *fp;
network_t *network;
int slif;
int short_flag;
{
    delay_time_t delay;
    node_t *pi, *po;
    lsGen gen;
    char *method;
    char term;

    method = (slif == 0) ? "default_" : "global_attribute ";
    term = (slif == 0) ? ' ' : ';';

    if (slif){
	/* Also need to write out the type declarations for SLIF */
	io_fprintf_break(fp, ".type input_arrival %%f %%f ;\n");
	io_fprintf_break(fp, ".type output_required %%f %%f ;\n");
	io_fprintf_break(fp, ".type input_drive %%f %%f ;\n");
	io_fprintf_break(fp, ".type output_load %%f ;\n");
    }

    if (get_default_delay_param(network, DELAY_DEFAULT_ARRIVAL_RISE, &delay)) {
	io_fprintf_break(fp, ".%sinput_arrival %4.2f %4.2f%c\n", method, 
		       delay.rise, delay.fall, term);
    }
    if (get_default_delay_param(network, DELAY_DEFAULT_REQUIRED_RISE, &delay)) {
	io_fprintf_break(fp, ".%soutput_required %4.2f %4.2f%c\n", method, 
		       delay.rise, delay.fall, term);
    }
    if (get_default_delay_param(network, DELAY_DEFAULT_DRIVE_RISE, &delay)) {
	io_fprintf_break(fp, ".%sinput_drive %4.2f %4.2f%c\n", method,
			delay.rise, delay.fall, term);
    }
    if (get_default_delay_param(network, DELAY_DEFAULT_OUTPUT_LOAD, &delay)) {
	io_fprintf_break(fp, ".%soutput_load %4.2f%c\n", method, delay.rise,
			term);
    }
    if (get_default_delay_param(network, DELAY_DEFAULT_MAX_INPUT_LOAD, &delay)){
	io_fprintf_break(fp, ".%smax_input_load %4.2f%c\n", method, delay.rise,
			term);
    }

    if (slif == 0) {
	method = "";
    }

    if (get_default_delay_param(network, DELAY_WIRE_LOAD_SLOPE, &delay)) {
	io_fprintf_break(fp, ".%swire_load_slope %4.2f%c\n", method,
			delay.rise, term);
    }

    if (slif == 0) {
        delay_print_blif_wire_loads(network, fp);
    }
/*
    else {
        delay_print_slif_wire_loads(network, fp);
    }
*/

    if (slif != 0) {
        method = "attribute ";
    }
    foreach_primary_input(network, gen, pi) {
#ifdef SIS
	 if (network_is_real_pi(network, pi) != 0) {
#endif /* SIS */
	    if (get_delay_param(pi, DELAY_ARRIVAL_RISE, &delay)) {
	        io_fprintf_break(fp, ".%sinput_arrival %s %4.2f %4.2f",
			   method, io_name(pi, short_flag), delay.rise, delay.fall);
#ifdef SIS
		if (slif == 0) {
		    write_synch_edge(fp, pi, short_flag);
                }
#endif /* SIS */
 		io_fputc_break(fp, term);
		io_fputc_break(fp, '\n');
	    }
	    if (get_delay_param(pi, DELAY_DRIVE_RISE, &delay)) {
	        io_fprintf_break(fp, ".%sinput_drive %s %4.2f %4.2f%c\n", 
			   method, io_name(pi, short_flag), delay.rise, delay.fall, term);
	    }
	    if (get_delay_param(pi, DELAY_MAX_INPUT_LOAD, &delay)) {
	        io_fprintf_break(fp, ".%smax_input_load %s %4.2f%c\n", 
			   method, io_name(pi, short_flag), delay.rise, term);
	    }
#ifdef SIS
	}
#endif /* SIS */
    }
    foreach_primary_output(network, gen, po) {
#ifdef SIS
        if (network_is_real_po(network, po) != 0) {
#endif /* SIS */
	    if (get_delay_param(po, DELAY_REQUIRED_RISE, &delay)) {
	        io_fprintf_break(fp, ".%soutput_required %s %4.2f %4.2f", 
			   method, io_name(po, short_flag), delay.rise, delay.fall);
#ifdef SIS
		if (slif == 0) {
		    write_synch_edge(fp, po, short_flag);
		}
#endif /* SIS */
		io_fputc_break(fp, term);
		io_fputc_break(fp, '\n');
	    }
	    if (get_delay_param(po, DELAY_OUTPUT_LOAD, &delay)) {
	        io_fprintf_break(fp, ".%soutput_load %s %4.2f%c\n", method,
			   io_name(po, short_flag), delay.rise, term);
	    }
#ifdef SIS
	}
#endif /* SIS */
    }
}

#ifdef SIS

static void
write_synch_edge(fp, pio, short_flag)
FILE *fp;
node_t *pio;
int short_flag;
{
    clock_edge_t edge;
    int flag;
    node_t *node;

    if (delay_get_synch_edge(pio, &edge, &flag) != 0) {
	if ((node = network_find_node(node_network(pio), clock_name(edge.clock))) == NIL(node_t)) {
	    fail("Clock specified in sychronization edge is not in network");
	}
        io_fprintf_break(fp, " %c %c'%s",
			((flag == BEFORE_CLOCK_EDGE) ? 'b' : 'a'),
			((edge.transition == RISE_TRANSITION) ? 'r' : 'f'),
			io_name(node, short_flag));
    }
}
#endif /* SIS */

static int
get_default_delay_param(network, param, delay)
network_t *network;
delay_param_t param;
delay_time_t *delay;
{
    switch(param) {
    case DELAY_DEFAULT_ARRIVAL_RISE:
	delay->rise = delay_get_default_parameter(network,
		DELAY_DEFAULT_ARRIVAL_RISE);
	delay->fall = delay_get_default_parameter(network,
		DELAY_DEFAULT_ARRIVAL_FALL);
	break;
    case DELAY_DEFAULT_DRIVE_RISE:
	delay->rise = delay_get_default_parameter(network,
		DELAY_DEFAULT_DRIVE_RISE);
	delay->fall = delay_get_default_parameter(network,
		DELAY_DEFAULT_DRIVE_FALL);
	break;
    case DELAY_DEFAULT_REQUIRED_RISE:
	delay->rise = delay_get_default_parameter(network,
		DELAY_DEFAULT_REQUIRED_RISE);
	delay->fall = delay_get_default_parameter(network,
		DELAY_DEFAULT_REQUIRED_FALL);
	break;
    case DELAY_DEFAULT_OUTPUT_LOAD:
	delay->rise = delay_get_default_parameter(network,
		DELAY_DEFAULT_OUTPUT_LOAD);
	delay->fall = 0;
	break;
    case DELAY_DEFAULT_MAX_INPUT_LOAD:
	delay->rise = delay_get_default_parameter(network,
		DELAY_DEFAULT_MAX_INPUT_LOAD);
	delay->fall = 0;
	break;
    case DELAY_WIRE_LOAD_SLOPE:
	delay->rise = delay_get_default_parameter(network,
		DELAY_WIRE_LOAD_SLOPE);
	delay->fall = 0;
	break;
    }

    if (delay->rise == DELAY_NOT_SET || delay->fall == DELAY_NOT_SET) {
        return(0);
    }
    return(1);
}

static int
get_delay_param(node, param, delay)
node_t *node;
delay_param_t param;
delay_time_t *delay;
{
    switch(param) {
    case DELAY_ARRIVAL_RISE:
	delay->rise = delay_get_parameter(node, DELAY_ARRIVAL_RISE);
	delay->fall = delay_get_parameter(node, DELAY_ARRIVAL_FALL);
	break;
    case DELAY_DRIVE_RISE:
	delay->rise = delay_get_parameter(node, DELAY_DRIVE_RISE);
	delay->fall = delay_get_parameter(node, DELAY_DRIVE_FALL);
	break;
    case DELAY_REQUIRED_RISE:
	delay->rise = delay_get_parameter(node, DELAY_REQUIRED_RISE);
	delay->fall = delay_get_parameter(node, DELAY_REQUIRED_FALL);
	break;
    case DELAY_OUTPUT_LOAD:
	delay->rise = delay_get_parameter(node, DELAY_OUTPUT_LOAD);
	delay->fall = 0;
	break;
    case DELAY_MAX_INPUT_LOAD:
	delay->rise = delay_get_parameter(node, DELAY_MAX_INPUT_LOAD);
	delay->fall = 0;
	break;
    }

    if (delay->rise == DELAY_NOT_SET || delay->fall == DELAY_NOT_SET) {
        return 0;
    }
    return(1);
}
