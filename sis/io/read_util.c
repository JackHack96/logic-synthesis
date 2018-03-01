/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/io/read_util.c,v $
 * $Author: pchong $
 * $Revision: 1.2 $
 * $Date: 2004/02/09 07:06:12 $
 *
 */
#include "sis.h"
#include "io_int.h"

/* from map/map_int.h */
extern lib_gate_t *choose_smallest_gate();

int read_lineno;
char *read_filename;
void dc_network_check();

static int is_in_list();
static void compute_relax();
static int read_special_case_for_stg();
static void node_map_copy();

/* no changes allowed to rcil, re, rftn, rfocn, rho, rrf */

/*VARARGS1*/
void
read_error(char *fmt, ...)
{
    va_list args;
    char buf[MAX_WORD];

    va_start(args, fmt);
    if (read_filename != NIL(char)) {
	(void) sprintf(buf, "\"%s\", line %d: ", read_filename, read_lineno);
	error_append(buf);
    }
    (void) vsprintf(buf, fmt, args);
    error_append(buf);
    error_append("\n");
    va_end(args);
}


void
read_register_filename(fname)
char *fname;
{
    read_lineno = 1;
    if (read_filename != NIL(char)) {
	FREE(read_filename);
    }
    read_filename = fname ? util_strsav(fname) : 0;
}

/*
 * HACK to support the user specifying manes with trailing "'".
 * Depending on the context can do two things (hence the flag)
 * Assume that the string s == "foo'" then
 * If flag == 0 then ( node = foo; foo' = !node;)
 * If flag == 1 then ( node = foo'; foo = !node;)
 *
 * Flag == 1 essentially when the output of a call is named "name'"
 */
node_t *
read_slif_find_or_create_node(network, s, flag)
network_t *network;
char *s;
int flag;
{
    int len;
    int complement = FALSE;
    node_t *node, *pos_node, *temp_node;

    len = strlen(s);
    if (len > 1 && s[len-1] == '\047'){
	complement = TRUE;
    }
    if ((node = network_find_node(network, s)) == 0) {
	if (complement){
	    /* First search for the node with the positive function */
	    s[len-1] = '\0';
	    pos_node = read_find_or_create_node(network, s);
	    s[len-1] = '\047';
	}
	if (complement && !flag){
	    node = node_literal(pos_node, 0);
	} else {
	    node = node_alloc();
	}
	node->name = util_strsav(s);
	network_add_node(network, node);
	if (complement && flag){
	    temp_node = node_literal(node, 0);
	    node_replace(pos_node, temp_node);
	}
    }
    /*
     * need to recognize names "0" and "1" as constants. These nodes have
     * been given names "0" and "1" and so we will not duplicate them on
     * successive references.
     */
    if (strcmp(s, "0") == 0){
	temp_node = node_constant(0);
	node_replace(node, temp_node);
    } else if (strcmp(s, "1") == 0){
	temp_node = node_constant(1);
	node_replace(node, temp_node);
    }

    if(node_type(node) == PRIMARY_OUTPUT) 
	node = node_get_fanin(node, 0);
    return node;
}

node_t *
read_find_or_create_node(network, s)
network_t *network;
char *s;
{
    node_t *node;

    if ((node = network_find_node(network, s)) == 0) {
	node = node_alloc();
	node->name = util_strsav(s);
	network_add_node(network, node);
    }
    if(node_type(node) == PRIMARY_OUTPUT) 
	node = node_get_fanin(node, 0);
    return node;
}

#ifdef SIS
/*
 * Go change the internal node name to make sure that the name cannot clash
 * with user names that might come later.  Does this by putting a " " as the
 * first character of the name.
 */
void
read_change_madeup_name(network, node)
network_t *network;
node_t *node;
{
    char buf[32];
    int index;

    if (node_is_madeup_name(node_long_name(node), &index)) {
        (void) sprintf(buf, " %d", index);
	network_change_node_name(network, node, util_strsav(buf));
    }
}
#endif /* SIS */

int
read_check_io_list(network, po_list, print_warning)
network_t *network;
lsList po_list;
int print_warning;
{
    int relax_i, relax_o;
    node_t *node;
    lsGen gen;
    node_type_t type;
    lib_gate_t *gate;
    char *error;
    int mapped_flag = 0;

    /* Check for nodes not driven or not faning out */
	/* check if .inputs or .outputs not specified */

    (void) compute_relax(network, po_list, &relax_i, &relax_o);

    if (lib_network_is_mapped(network)) mapped_flag = 1;
    foreach_node (network, gen, node) {
        type = node_type(node);
	if (node->F == 0 && type == INTERNAL) {
	    if (relax_i != 0) {
	        network_change_node_type(network, node, PRIMARY_INPUT);
	    }
	    else {
		if (print_warning)
	        read_error("Warning: network `%s', node \"%s\" is not driven (zero assumed)",
			network_name(network), io_node_name(node));
		node_replace(node, node_constant(0));
		if (mapped_flag) {
		    error = util_strsav(error_string());
		    gate = choose_smallest_gate(lib_get_library(), "f=0;");
		    error_init();
		    error_append(error);
		    if (gate == NIL(lib_gate_t)) {
			error_append("Unable to map constant 0 node\n");
			return 0;
		    } else {
			lib_set_gate(node, gate, NIL(char *), NIL(node_t *), 0);
		    }
		}
	    }
	}
	else if (type != PRIMARY_OUTPUT && node_num_fanout(node) == 0 &&
			is_in_list(po_list, node) == 0) {
	    if (relax_o != 0) {
	        (void) lsNewEnd(po_list, (lsGeneric) node, LS_NH);
	    }
	    else {
		if (print_warning)
	        read_error("Warning: network `%s', node \"%s\" does not fanout",
			network_name(network), io_node_name(node));
	    }
	}
    }
    return 1;
}

void
read_hack_outputs(network, po_list)
network_t *network;
lsList po_list;
{
   lsGen gen;
   node_t *po, *node;
   char *name, *po_name;

    /* hack the primary outputs -- create the real PO nodes */

    lsForeachItem(po_list, gen, node) {
	po = network_add_primary_output(network, node);
#ifdef SIS
        if (node->type == PRIMARY_INPUT &&
		network_latch_end(node) == NIL(node_t)){
#else
 	if (node->type == PRIMARY_INPUT) {
#endif
	    po_name = node_long_name(po);
	    name = ALLOC(char, strlen(po_name)+4);
	    (void)sprintf(name, "IN-%s", po_name);
            (void) fprintf(siserr, "Warning: input and output named \"%s\":  renaming input \"%s\"\n",
		po_name, name);
	    network_change_node_name(network, node, name);
        }
    }
}

/*
 * remove technology independent buffers that are inputs to PO nodes
 * EXCEPTION CONDITION FOR SIS:
 * We want to retain the buffers if the latch output (PI node "fanin") 
 * could be input to the exdc network. We want the buffer to provide proper
 * naming for the latch output (it could be feeding a true primary output)
 */
void
read_cleanup_buffers(network)
network_t *network;
{

    int i;
    lsGen gen, gen1;
    node_t *po, *node, *fanin, *fo, **fo_array;

    foreach_primary_output(network, gen, po){
	node = node_get_fanin(po, 0);
	if (node_function(node) == NODE_BUF &&
		lib_gate_of(node) == NIL(lib_gate_t)) {
	    fanin = node_get_fanin(node, 0);
#ifdef SIS
	    if ((network_latch_end(fanin) == NIL(node_t))){
#endif /* SIS */
		fo_array = ALLOC(node_t *, node_num_fanout(node));
		i = 0;
		foreach_fanout(node, gen1, fo) {
		    fo_array[i++] = fo;
		}
		for (; i-- > 0;) {
		    assert(node_patch_fanin(fo_array[i], node, fanin));
		}
		FREE(fo_array);
		network_delete_node(network, node);
#ifdef SIS
	    }
#endif /* SIS */
	}
    }
}


#ifdef SIS
void
read_check_control_signals(network)
network_t *network;
{
    lsGen gen;
    latch_t *latch;
    node_function_t func;
    node_t *control, *node;
    char *name;
    st_table *table;
    
    table = st_init_table(st_ptrcmp, st_ptrhash);
    foreach_latch (network, gen, latch) {
        control = latch_get_control(latch);
	if (control != NIL(node_t)) {
	    if (!st_is_member(table, (char *)control)) {
	        node = node_get_fanin(control, 0);
	        func = node_function(node);
	        if (func == NODE_1 || func == NODE_0) {
	            read_error("Warning: network `%s', latch control `%s' is constant (disconnecting)",
				network_name(network), io_node_name(control));
	            latch_set_control(latch, NIL(node_t));
		    (void) st_insert(table, (char *)control, NIL(char));
		    name = util_strsav(node_long_name(control));
		    network_delete_node(network, control);
		    network_change_node_name(network, node, name);
		}
	    } else {
		latch_set_control(latch, NIL(node_t));
	    }
	}
    }
    st_free_table(table);
}
#endif /* SIS */        

static int
is_in_list(list, node)
lsList list;
node_t *node;
{
    node_t *p;
    lsGen gen;

    lsForeachItem(list, gen, p) {
        if (p == node) {
	    LS_ASSERT(lsFinish(gen));
	    return 1;
	}
    }
    return 0;
}

void
read_filename_to_netname(network, filename)
network_t *network;
char *filename;
{
    char *name, *base;

    base = strrchr(filename, '/');
    name = (base == 0) ? filename : base + 1;
    network_set_name(network, name);
}

static void
compute_relax(network, po_list, prelax_i, prelax_o)
network_t *network;
lsList po_list;
int *prelax_i, *prelax_o;
{
    lsGen gen;
    node_t  *pi;

    *prelax_i = 1;

	/*
	 * If any real PIs exist, then .inputs were specified.
	 */
    foreach_primary_input (network, gen, pi) {
#ifdef SIS
        if (network_is_real_pi(network, pi) != 0 &&
		clock_get_by_name(network, node_long_name(pi)) == 0) {
#endif /* SIS */
	    *prelax_i = 0;
	    (void) lsFinish(gen);
	    break;
#ifdef SIS
	}
#endif /* SIS */
    }
    *prelax_o = (lsLength(po_list) == 0);
}

/*
 * Returns the next space seperated word in the string `str'.
 * Duplicates function of strtok, but returns pointer to rest of string
 * if `rest' is non-null.
 */
char *
gettoken(str, rest)
char *str, **rest;
{
    static char *r = NIL(char);

    if (str == NIL(char)) {
        str = r;
	if (r == NIL(char)) {
	    return(NIL(char));
	}
    }
    else {
        r = str;
	if (*r == '\0') {
	    r = NIL(char);
	    return(NIL(char));
	}
    }

    do {				/* skip to next blank */
        r++;
    } while (*r != '\0' && *r != ' ');

    if (*r == '\0') {	/* end of string */
        r = NIL(char);
    }
    else {
        *r++ = '\0';
    }
    if (rest != NIL(char *)) {
        *rest = r;
    }
    return(str);
}

/**************  read_delay  **************/

static void read_delay_set_parameters();
static node_t *read_input_node();
static node_t *read_output_node();
static int read_delay_common();

/*
 *  check a blif keyword for a delay-related keyword   OR
 *  check a slif global or local attribute
 *
 *  returns -1 if not delay related, 0 if delay-related and an error occured, 
 *  or 1 if delay-related and no error.
 *
 * `slif_global' packs two binary values into one var (# is in decimal)
 * `slif_global' = 00		==> not reading slif
 *               = 10		==> slif, .attribute
 *	         = 11		==> slif, .global_attribute
 */
int
read_delay(network, po_list, word, rest, slif_global)
network_t *network;
lsList po_list;
char *word, *rest;
int slif_global;
{
    double br, bf, dr, df;
    double area, load, max_load, phase; 
    node_t *node;
    int def;
    lsGen gen;
    delay_param_t d1, d2;

    if (strcmp(word, "area") == 0 || strcmp(word, "cost") == 0) {
        if (slif_global == 10) {
	    read_error("area is a global attribute");	/* slif error */
	    return(0);
	}
	if (sscanf(rest, "%lf", &area) != 1) {
	    read_error("error reading number after area");
	    return(0);
	}
	if (network->area_given) {
	    read_error("area given twice for same model");
	    return(0);
	}
	network->area_given = 1;
	network->area = area;
	return(1);
    }

    else if (strcmp(word, "delay") == 0) {
        if (slif_global == 11) {	/* set delay for all inputs */
	}	
	else {				/* current arg is the one to set */
	    node = read_input_node(&rest, network, word);
	    if (node == NIL(node_t)) {
	        return(0);
	    }
	}
	if (sscanf(rest, "%s %lf %lf %lf %lf %lf %lf", word, &load, &max_load, 
		&br, &dr, &bf, &df) != 7) {
	    read_error("need phase, load, max_load, and 4 delay values in delay");
	    return(0);
	}
	if (strcmp(word, "INV") == 0) {
	    phase = DELAY_PHASE_INVERTING;
	}
	else if (strcmp(word, "NONINV") == 0) {
	    phase = DELAY_PHASE_NONINVERTING;
	}
	else if (strcmp(word, "UNKNOWN") == 0) {
	    phase = DELAY_PHASE_NEITHER;
	}
	else {
	    read_error("bad phase specification in .delay");
	    return(0);
	}

	if (slif_global == 11) {
	    foreach_primary_input (network, gen, node) {
	        read_delay_set_parameters(node, br, bf, dr, df, load,
				max_load, phase);
	    }
	}
	else {
	    read_delay_set_parameters(node, br, bf, dr, df, load, max_load,
	    		phase);
	}
	return(1);
    }

    else if (strcmp(word, "wire_load_slope") == 0) {
        if (slif_global == 10) {
	    read_error("wire_load_slope is a global attribute");
	}
	if (sscanf(rest, "%lf", &load) != 1) {
	    read_error("expect load value in wire_load_slope");
	    return(0);
	}
	delay_set_default_parameter(network, DELAY_WIRE_LOAD_SLOPE, load);
	return(1);
    }

    else if (strcmp(word, "wire") == 0 ) {
	/* discard any existing wire load table */
	delay_set_default_parameter(network, DELAY_ADD_WIRE_LOAD, -1.0);

	/* read the wire loads */
	while ((word = gettoken(NIL(char), NIL(char *))) != NIL(char)) {
	    load = atof(word);
	    delay_set_default_parameter(network, DELAY_ADD_WIRE_LOAD, load);
	}
	return(1);
    }

    else {
        if (strncmp(word, "default_", 8) == 0) {
	    def = 1;
	    word += 8;
	}
	else {
	    def = (slif_global == 11);
	}

	node = NIL(node_t);
	if (strcmp(word, "input_arrival") == 0) {
	    if (def == 0) {
	        if ((node = read_input_node(&rest, network, word)) == 0) {
		    return(0);
		}
		d1 = DELAY_ARRIVAL_RISE;
		d2 = DELAY_ARRIVAL_FALL;
	    }
	    else {
	        d1 = DELAY_DEFAULT_ARRIVAL_RISE;
		d2 = DELAY_DEFAULT_ARRIVAL_FALL;
	    }
#ifdef SIS
	    return(read_delay_common(node, network, word, rest, 4, d1, d2));
#else
	    return(read_delay_common(node, network, word, rest, 2, d1, d2));
#endif /* SIS */
	}

	else if (strcmp(word, "output_required") == 0) {
	    if (def == 0) {
	        if ((node = read_output_node(&rest, po_list, word)) == 0) {
		    return(0);
		}
		d1 = DELAY_REQUIRED_RISE;
		d2 = DELAY_REQUIRED_FALL;
	    }
	    else {
	        d1 = DELAY_DEFAULT_REQUIRED_RISE;
		d2 = DELAY_DEFAULT_REQUIRED_FALL;
	    }
#ifdef SIS
	    return(read_delay_common(node, network, word, rest, 4, d1, d2));
#else
	    return(read_delay_common(node, network, word, rest, 2, d1, d2));
#endif /* SIS */
	}

	else if (strcmp(word, "input_drive") == 0) {
	    if (def == 0) {
	        if ((node = read_input_node(&rest, network, word)) == 0) {
		    return(0);
		}
		d1 = DELAY_DRIVE_RISE;
		d2 = DELAY_DRIVE_FALL;
	    }
	    else {
	        d1 = DELAY_DEFAULT_DRIVE_RISE;
		d2 = DELAY_DEFAULT_DRIVE_FALL;
	    }
	    return(read_delay_common(node, network, word, rest, 2, d1, d2));
	}

	else if (strcmp(word, "output_load") == 0) {
	    if (def == 0) {
	        if ((node = read_output_node(&rest, po_list, word)) == 0) {
		    return(0);
		}
		d1 = DELAY_OUTPUT_LOAD;
	    }
	    else {
	        d1 = DELAY_DEFAULT_OUTPUT_LOAD;
	    }
	    return(read_delay_common(node, network, word, rest, 1, d1, d2));
	}

	else if (strcmp(word, "max_input_load") == 0) {
	    if (def == 0) {
	        if ((node = read_input_node(&rest, network, word)) == 0) {
		    return(0);
		}
		d1 = DELAY_MAX_INPUT_LOAD;
	    }
	    else {
	        d1 = DELAY_DEFAULT_MAX_INPUT_LOAD;
	    }
	    return(read_delay_common(node, network, word, rest, 1, d1, d2));
	}
    }
    return(-1);
}

static void
read_delay_set_parameters(node, br, bf, dr, df, load, max_load, phase)
node_t *node;
double br, bf, dr, df, load, max_load, phase;
{
    delay_set_parameter(node, DELAY_BLOCK_RISE, br);
    delay_set_parameter(node, DELAY_BLOCK_FALL, bf);
    delay_set_parameter(node, DELAY_DRIVE_RISE, dr);
    delay_set_parameter(node, DELAY_DRIVE_FALL, df);
    delay_set_parameter(node, DELAY_INPUT_LOAD, load);
    delay_set_parameter(node, DELAY_MAX_INPUT_LOAD, max_load);
    delay_set_parameter(node, DELAY_PHASE, phase);
}
    
/*
 * If node = NULL, then setting global attribute, otherwise setting attribute
 *   for that specific node.
 *
 * 0 error
 * 1 no error
 */
/*VARARGS7*/
static int
read_delay_common(node, network, word, rest, num, d1, d2)
node_t *node;
network_t *network;
char *word, *rest;
int num;
delay_param_t d1, d2;
{
    double v1, v2;
    char befaft[MAX_WORD], edge[MAX_WORD];
    int i;
#ifdef SIS
    clock_edge_t cedge;
#endif /* SIS */

    if (rest == NIL(char)) {
        read_error("wrong number arguments to %s", word);
        return(0);
    }

    i = sscanf(rest, "%lf %lf %s %s", &v1, &v2, befaft, edge);
    if (num != i) {
#ifdef SIS
        if (num != 4 || i != 2) {
	    read_error("wrong number arguments to %s", word);
	    return(0);
	}
#else
	read_error("wrong number arguments to %s", word);
	return(0);
#endif /* SIS */
    }

#ifdef SIS
    if (i == 4) {
        if (node == NIL(node_t)) {
	    read_error("cannot specify transition for .global_%s", word);
	    return(0);
	}
        if (*edge == 'r') {
	    cedge.transition = RISE_TRANSITION;
	}
	else if (*edge == 'f') {
	    cedge.transition = FALL_TRANSITION;
	}
	else {
	    read_error("transition must be `r' or `f' in %s", word);
	    return(0);
	}
	if ((cedge.clock = clock_get_by_name(network, edge + 2)) == 0) {
	    read_error("clock %s not found in %s", edge + 2, word);
	    return(0);
	}
        if (*befaft == 'b') {
	    delay_set_synch_edge(node, cedge, BEFORE_CLOCK_EDGE);
	}
	else if (*befaft == 'a') {
	    delay_set_synch_edge(node, cedge, AFTER_CLOCK_EDGE);
	}
	else {
	    read_error("synch edge must be `b' or `a' in %s", word);
	    return(0);
	}
    }        
#endif /* SIS */
    if (node == NIL(node_t)) {
        delay_set_default_parameter(network, d1, v1);
    }
    else {
        delay_set_parameter(node, d1, v1);
    }
    if (num > 1) {
        if (node == NIL(node_t)) {
	    delay_set_default_parameter(network, d2, v2);
	}
	else {
	    delay_set_parameter(node, d2, v2);
	}
    }
    return(1);
}

static node_t *
read_input_node(rest, network, errmsg)
char **rest;
network_t *network;
char *errmsg;
{
    node_t *node;
    char *word;

    if ((word = gettoken(NIL(char), rest)) == NIL(char)) {
	read_error("missing.sh name in %s", errmsg);
	return(NIL(node_t));
    }
    node = network_find_node(network, word);
    if (node == NIL(node_t) || node_type(node) != PRIMARY_INPUT) {
	read_error("node '%s' not defined as input node in %s", word, errmsg);
	return(NIL(node_t));
    }
    return(node);
}

static node_t *
read_output_node(rest, po_list, errmsg)
char **rest;
lsList po_list;
char *errmsg;
{
    node_t *node;
    lsGen gen;
    char *word;

    if ((word = gettoken(NIL(char), rest)) == NIL(char)) {
	read_error("missing.sh name in %s", errmsg);
	return(NIL(node_t));
    }
    lsForeachItem(po_list, gen, node) {
	if (strcmp(node->name, word) == 0) {
	    LS_ASSERT(lsFinish(gen));
	    return(node);
	}
    }
    read_error("node '%s' not defined as output node in %s", word, errmsg);
    return(NIL(node_t));
}


#ifdef SIS
/**********  read_delay_cleanup  ***********/

void
read_delay_cleanup(network)
network_t *network;
{
    lsGen gen;
    node_t *node, *fanin;

    /* copy any output information from its fanin to the output node */
    foreach_primary_output(network, gen, node) {
	if (network_is_real_po(network, node)) {
	    delay_copy_terminal_constraints(node);
	    fanin = node_get_fanin(node, 0);
	    if (network_latch_end(fanin) != NIL(node_t)){
		/*
		 * The output required time was stored at the PI representing
		 * a latch output. We should scrap that setting
		 */
		delay_set_parameter(fanin, DELAY_ARRIVAL_RISE, DELAY_NOT_SET);
		delay_set_parameter(fanin, DELAY_ARRIVAL_FALL, DELAY_NOT_SET);
	    }
	}
    }
}


int
read_search_file(filename, models, search_files, reader)
char *filename;
st_table *models, *search_files;
int (*reader)();
{
    char *name;
    FILE *fp;
    int line, status;
    network_t *dummy;
    
    if (filename == NIL(char)) {
        read_error("missing file name for search");
	return(0);
    }
    name = read_filename;
    fp = com_open_file(filename, "r", &read_filename, 1);
    if (fp == NIL(FILE)) {
        FREE(read_filename);
	read_filename = name;
	read_error("search file %s not found", filename);
	return(0);
    }
    if (!st_lookup(search_files, filename, NIL(char *))) {
	st_insert(search_files, util_strsav(filename), NIL(char));
    } else {
	return(1);
    }
    line = read_lineno;
    read_lineno = 1;
    status = (*reader)(fp, models, search_files, &dummy);
    FREE(read_filename);
    if (fp != stdin) {
        (void) fclose(fp);
    }
    read_filename = name;
    read_lineno = line;
    return(status);
}
#endif /* SIS */

/*
 * Creates or finds a modelinfo record.
 * Inserts it into the `models' table if new one is created.
 *
 * Also allocates a po_list, and a patch_list for the entry.
 */
modelinfo *
read_find_or_create_model(name, models)
char *name;
st_table *models;
{
    modelinfo *entry;

    if (st_lookup(models, name, (char **) &entry) == 1) {
	return(entry);
    }
    entry = ALLOC(modelinfo,1);
    entry->network = NIL(network_t);
    entry->po_list = lsCreate();
    entry->library = 0;
    entry->patch_lists = lsCreate();
    entry->latch_order_list = NULL;
    (void) st_insert(models, util_strsav(name), (char *) entry);
    entry->depends_on = 0;    

    return(entry);
}


#ifdef SIS
/*
 * The following routine returns 1 if the user only specified the 
 * .inputs and .outputs hoping that those names would be stored with the STG
 * In this case all the PI nodes have no fanout --- that's what we  will check
 */
static int
read_special_case_for_stg(network)
network_t *network;
{
   lsGen gen;
   node_t *node;
   foreach_node(network, gen, node){
       if (node->type == PRIMARY_INPUT && node_num_fanout(node) > 0){
	   lsFinish(gen);
	   return FALSE;
       }
   }
   return TRUE;
}
static int
read_set_latch_order(network, latch_order_list)
network_t *network;
lsList latch_order_list;
{
    lsGen gen;
    node_t *p;
    latch_t *l;
    graph_t *stg;
    lsList newlatch;
    char *name;
    int i, order;
    array_t *names, *node_array;

    stg = network_stg(network);
    /*
     * For the special case when the STG is specified with the PI and PO 
     * declarations. The input names were stored earlier (as part
     * of the .start_kiss processing we did a network_set_stg(). At that
     * time the PO nodes were not present. Hence we have to do it again...
     * We don't want stg_save_names to print out the error messages
     * again though.
     */
    if (stg != NIL(graph_t)) {
        names = stg_get_names(stg, 0);
	if (names != NIL(array_t)) {
	    for (i = array_n(names); i-- > 0; ){
		name = array_fetch(char *, names, i);
		FREE(name);
	    }
	    (void) array_free(names);
	}
        stg_set_names(stg, NIL(array_t), 0);
        names = stg_get_names(stg, 1);
	if (names != NIL(array_t)) {
	    for (i = array_n(names); i-- > 0; ){
		name = array_fetch(char *, names, i);
		FREE(name);
	    }
	    (void) array_free(names);
	}
        stg_set_names(stg, NIL(array_t), 1);
	if (stg_save_names(network, stg, 0)) {
	    node_array = network_dfs_from_input(network);
	    for (i = 0; i < array_n(node_array); i++) {
		p = array_fetch(node_t *, node_array, i);
		network_delete_node(network, p);
	    }
	    array_free(node_array);
	}
    }

    order = (latch_order_list != 0) ? (lsLength(latch_order_list) != 0) : 0;
    name = network_name(network);

    if (stg == NIL(graph_t) || lsLength(network->nodes) == 0) {
        if (order != 0) {
	    read_error("model `%s': .latch_order unnecessary", name);
	}
	return(1);
    }
    /*
     * stg & network given... Need to handle the special case when only
     * the .inputs and .outputs are specified alongwith a STG
     */
    if (order == 0 && !read_special_case_for_stg(network)){
        read_error("model `%s': .latch_order & .code must be given w/ stg",
	    		name);
	return(0);
    }
	/*
	 * Order the latches
	 */

    if (latch_order_list != 0) {
        newlatch = lsCreate();
   
	lsForeachItem (latch_order_list, gen, p) {
	    l = latch_from_node(p);
	    if (l == NIL(latch_t)) {
	        read_error("model `%s': %s is not the output of a latch", name,
				node_name(p));
		(void) lsFinish(gen);
		return(0);
	    }
	    (void) lsNewEnd(newlatch, (lsGeneric) l, LS_NH);
	}
	(void) lsDestroy(latch_order_list, (void (*)()) 0);
	(void) lsDestroy(network->latch, (void (*)()) 0);
	network->latch = newlatch;
    }
    return(1);
}
#endif /* SIS */

static enum st_retval read_patch_network();

enum patch_retval {
    NO_PATCHED, PATCH_ERROR, PATCH_OK
};

typedef struct {
    st_table *models;			/* data in */
    network_t *master;			/* data in */
    enum patch_retval patch_return;	/* data out */
} st_foreach_data;

static void
patch_free(patch)
patchinfo *patch;
{
    int i;
    char **formals = patch->formals;

    if (formals != NIL(char *)) {
        for (i = array_n(patch->actuals) - 1; i >= 0; i--) {
	    FREE(formals[i]);
	}
	FREE(formals);
    }
    if (patch->actuals != NIL(array_t)) {
        array_free(patch->actuals);
    }
    FREE(patch);
}

int
read_blif_slif(fp, networkp, reader)
FILE *fp;
network_t **networkp;
int (*reader)();
{
    st_table *models = st_init_table(strcmp, st_strhash);
    st_table *search_files = st_init_table(strcmp, st_strhash);
    int status;
    char *name;
    st_generator *stgen;
    modelinfo *entry;
    network_t *network = NIL(network_t);
    st_foreach_data sfd;
#ifdef SIS
    graph_t *stg;
    vertex_t *v;
    int encode_length;
    int i;
#endif /* SIS */

    status = (*reader)(fp, models, search_files, &network);

    /* Search files keeps track of which files we've already seen so we
       don't look at one file more than once. */

    st_foreach_item(search_files, stgen, &name, NIL(char *)) {
        FREE(name);
    }
    st_free_table(search_files);

	/* patch all the holes */

    sfd.master = network;
    sfd.patch_return = PATCH_OK;
    if (network == NIL(network_t) || status != 1) {
    	/* propogate error */

        network = NIL(network_t);
	sfd.master = NIL(network_t);
    }
    else {
	sfd.models = models;
	FREE(read_filename);
        do {
	    sfd.patch_return = NO_PATCHED;
	    st_foreach(models, read_patch_network, (char *) &sfd);
	    if (sfd.patch_return == NO_PATCHED) {
	        read_error("Cyclic model dependency detected, probably involving:");
		st_foreach_item (models, stgen, &name, NIL(char *)) {
		    read_error(name);
		}
		break;
	    }
	} while (sfd.patch_return != PATCH_ERROR && st_count(models) != 0);
    }

    if (sfd.patch_return != PATCH_OK) {
        network = NIL(network_t);
    }
#ifdef SIS
    else if (network != NIL(network_t)) {	

        network_replace_io_fake_names(network);
    }
#endif /* SIS */

	/* delete all the temporary networks not needed anymore */

    st_foreach_item (models, stgen, &name, (char **) &entry) {
    	if (entry->network != network) {
	    network_free(entry->network);
	}
	if (entry->po_list != 0) {
	    (void) lsDestroy(entry->po_list, (void (*)()) 0);
	}
	if (entry->latch_order_list != 0) {
	    (void) lsDestroy(entry->latch_order_list, (void (*)()) 0);
	}
	if (entry->patch_lists != 0) {
	    (void) lsDestroy(entry->patch_lists, patch_free);
	}
        FREE(name);
	FREE(entry);
    }
    st_free_table(models);

#ifdef SIS
    /* Need to fix the stg so that any state labelled "ANY" or "*" */
    /* has an encoding string of all 2's */
    /* First check the stg */

    if (network != NIL(network_t)) {
        stg = network_stg(network);
        if (stg != NIL(graph_t)) {
	    v = stg_get_start(stg);
            name = stg_get_state_encoding(v);
            if (name != NIL(char)) {
	        encode_length = strlen(name);
            } else {
                encode_length = 0;
            }
	    v = stg_get_state_by_name(stg, "ANY");
            if (v != NIL(vertex_t)) {
	        if ((encode_length != 0) &&
                    (stg_get_state_encoding(v) == NIL(char))) {
	            name = ALLOC(char, encode_length+1);
                    for (i = 0; i < encode_length; i++) {
			name[i] = '2';
		    }
		    name[encode_length] = '\0';
		    stg_set_state_encoding(v, name);
		    FREE(name);
		}
	    }
	    v = stg_get_state_by_name(stg, "*");
            if (v != NIL(vertex_t)) {
	        if ((encode_length != 0) &&
                    (stg_get_state_encoding(v) == NIL(char))) {
		    name = ALLOC(char, encode_length+1);
                    for (i = 0; i < encode_length; i++) {
			name[i] = '2';
		    }
		    name[encode_length] = '\0';
		    stg_set_state_encoding(v, name);
		    FREE(name);
	        }
	    }
	}
#ifdef DEBUG
	if (!network_stg_check(network)) {
	    read_error("network does not cover stg");
	    network_free(network);
	    status = 0;
	}
#endif
    }
#endif /* SIS */
    if (network != NIL(network_t)) {
	if (!network_check(network)) {
	    read_error("network failed network_check");
	    network_free(network);
	    status = 0;
	}
    }
/* Correct the naming in the don't care network. The fake primary outputs are renamed to their
 * corresponding primary output names in the care network.
 */
    if (network != NIL(network_t)) {
    if (network->dc_network  != NIL(network_t)) {
        dc_network_check(network, &status);
    }
    }
    *networkp = network;
    return(status);
}

#ifdef SIS
static node_t *
find_or_create_mapped_node(net, map, node)
network_t *net;
st_table *map;
node_t *node;
{
    node_t *found;
    char *name;

    if (st_lookup(map, (char *) node, (char **) &found) == 0) {
	found = node_alloc();
	network_add_node(net, found);
	(void) st_insert(map, (char *) node, (char *) found);

        name = node_long_name(node);
	if (network_find_node(net, name) == NIL(node_t)) {
	    network_change_node_name(net, found, util_strsav(name));
	}
    }
    return(found);
}

static int
is_real_pio(net, node)
network_t *net;
node_t *node;
{
    if (network_is_real_pi(net, node) != 0) {
        return(1);
    }
    return(network_is_real_po(net, node));
}

#endif /* SIS */

/*
 * If the network of `entry' does not depend on anything, then patches
 * that network into all the networks associated with the patch_lists
 * of `entry'.  Also cleans up the outputs of that network.
 * 
 * The network of `entry' is not collapsed before insertion into the 
 * surrounding networks.
 *
 * `entry' is removed from the `models' table after it has been patched in.
 *
 * `entry->network' is either inserted into the library or all the storage
 * associated with `entry->network' is freed depending on `entry->library'.
 */
static enum st_retval
read_patch_network(sname, entry, sfd)
char *sname;
modelinfo *entry;
st_foreach_data *sfd;
{
  network_t *snet;
  lsList po_list;
  library_t *library;
  lib_gate_t *gate;
#ifdef SIS
  lsGen gen, netgen;
  network_t *bnet;
  int inputs, outputs, n, ins, k, num_false, num_controls;
  array_t *bactuals;
  modelinfo *bentry;
  node_t *new_node, *bnode, *bfanin, *snode, *sfanin, *bpi;
  node_t *scontrol, *bcontrol;
  patchinfo *bpatch;
  st_table *controls;
  latch_t *slatch, *blatch;
  node_type_t type;
  st_table *models, *nmap;
  char **bformals, *formal, *bname;
#endif /* SIS */

	/*
	 * snet is the small network being inserted.
	 * bnet is the big network being inserted into.
	 */
	
	/* If null network, look in library--error if not found */

  snet = entry->network;
  if (snet == NIL(network_t)) {
    library = lib_get_library();
    if (library != NIL(library_t)) {
      gate = lib_get_gate(library, sname);
    }
    if (library == NIL(library_t) || gate == NIL(lib_gate_t)) {
      read_error("can't find model `%s' in files or library", sname);
      sfd->patch_return = PATCH_ERROR;
      return(ST_STOP);
    }
  }
  else if (entry->depends_on > 0) {
    return(ST_CONTINUE);
  }

	/* Clean up po_list and check network is not cyclic */

  po_list = entry->po_list;
  if (snet != NIL(network_t)) {
    /* If this is a special case where names are saved on the stg for the
       coming network, don't print warning messages. */
    if (!read_check_io_list(snet, po_list, !read_special_case_for_stg(snet))) {
	sfd->patch_return = PATCH_ERROR;
	return(ST_STOP);
    }
    read_hack_outputs(snet, po_list);
#ifdef SIS
    read_check_control_signals(snet);
#endif /* SIS */
  }
  (void) lsDestroy(po_list, (void (*)()) 0);
  entry->po_list = NULL;

  if (snet != NIL(network_t)) {
#ifdef SIS
    if (read_set_latch_order(snet, entry->latch_order_list) == 0) {
      sfd->patch_return = PATCH_ERROR;
      return(ST_STOP);
    }
    entry->latch_order_list = NULL;
#endif /* SIS */

    if (network_check(snet) == 0) {
      read_error("model `%s': network_check failed", sname);
      sfd->patch_return = PATCH_ERROR;
      return(ST_STOP);
    }

    if (network_is_acyclic(snet) == 0) {
      read_error("network `%s' contains a cycle", sname);
      sfd->patch_return = PATCH_ERROR;
      return(ST_STOP);
    }

#ifdef SIS
    read_delay_cleanup(snet);
#endif

	/* Insert this network into the library */

#ifdef SIS
    if (entry->library != 0) {		/* only for slif (so far) */
      if (slif_add_to_library(snet) == 0) {
        sfd->patch_return = PATCH_ERROR;
	return(ST_STOP);
      }
    }
#endif /* SIS */
  }
  else {
    snet = gate->network;
  }

#ifdef SIS

	/*
	 * Figure out all the false PIs & POs that come from the latches and
	 * clocks.
	 */

  if (lsLength(entry->patch_lists) > 0) {
    num_false = 0;
    controls = st_init_table(st_ptrcmp, st_ptrhash);

    foreach_latch (snet, gen, slatch) {
      num_false++;
      snode = latch_get_control(slatch);
      if (snode != NIL(node_t)) {
	(void) st_insert(controls, (char *) snode, NIL(char));
      }
    }
    num_controls = st_count(controls);

    outputs = network_num_po(snet) - num_false - num_controls;
    inputs = network_num_pi(snet) - num_false;
    models = sfd->models;
  }

	/*
	 * For each network that depends on this network ...
	 */

  lsForeachItem (entry->patch_lists, gen, bpatch) {
    (void) st_lookup(models, bpatch->netname, (char **) &bentry);
    bnet = bentry->network;
    bname = network_name(bnet);
    bactuals = bpatch->actuals;
    bentry->depends_on--;

	    /*
	     * Build mapping between PIs, POs, and clocks of two networks.
	     */

    nmap = st_init_table(st_ptrcmp, st_ptrhash);

    bformals = bpatch->formals;

 	  /*
	   * formal=actual format
	   */

    if (bformals != NIL(char *)) {	

      for (k = array_n(bactuals); --k >= 0; ) {
        bnode = array_fetch(node_t *, bactuals, k);
	formal = bformals[k];
	snode = network_find_node(snet, formal);
	if (snode == NIL(node_t) || is_real_pio(snet, snode) == 0) {
	  read_error("model `%s', \".subckt %s\": formal argument \"%s\" not part of `%s'",
		  	bname, sname, formal, sname);
	  sfd->patch_return = PATCH_ERROR;
	  continue;
	}
	if (node_type(snode) == PRIMARY_OUTPUT) {
	  if (node_function(bnode) != NODE_UNDEFINED) {
	    read_error("model `%s', \".subckt %s\": in \"%s=%s\", \"%s\" is output but \"%s\" already has function",
	    		bname, sname, formal, io_node_name(bnode), formal,
			io_node_name(bnode));
	    sfd->patch_return = PATCH_ERROR;
	  }
	  (void) st_insert(nmap, (char *) snode, NIL(char));
	  snode = node_get_fanin(snode, 0);
	}
	if (st_insert(nmap, (char *) snode, (char *) bnode) != 0) {
	  read_error("model `%s', \".subckt %s\": formal argument \"%s\" specified twice",
	  		bname, sname, formal);
	  sfd->patch_return = PATCH_ERROR;
	}
	FREE(formal);
      }
      FREE(bpatch->formals);
      if (sfd->patch_return == PATCH_ERROR) {
        (void) lsFinish(gen);
	st_free_table(nmap);
	return(ST_STOP);
      }
    }

	/*
	 * just actual arguments	(SLIF)
	 */

    else {

      n = array_n(bactuals);
      ins = bpatch->inputs;
      if (ins == -1) { 			/* # inputs not specified */
        if (n != inputs + outputs) {    /* have wrong total # signals */
	  (void) fprintf(siserr,
		"Warning: wrong number inputs or outputs to `%s' in `%s'\n",
		sname, bname);
	}
      }
      else if (inputs != ins || outputs != (n - ins)) {
        (void) fprintf(siserr, "Warning: wrong number of %s to `%s' in `%s'\n",
      		((inputs == ins) ? "outputs" : "inputs"), sname,
		bname);
      }
      k = 0;
      foreach_primary_input (snet, netgen, snode) {
	if (network_latch_end(snode) == NIL(node_t)){
	  bnode = array_fetch(node_t *, bactuals, k++);
	  (void) st_insert(nmap, (char *) snode, (char *) bnode);
	}
      }
      foreach_primary_output (snet, netgen, snode) {
	if (network_latch_end(snode) == NIL(node_t) &&
		!st_is_member(controls, (char *)snode)){
	  bnode = array_fetch(node_t *, bactuals, k++);
	  snode = node_get_fanin(snode, 0);
	  (void) st_insert(nmap, (char *) snode, (char *) bnode);
	}
      }
	    /* clocks ?? */
    }

	    /*
	     * For each node of the network ...
	     */

    foreach_node (snet, netgen, snode) {
      type = node_type(snode);
      if (type != INTERNAL && is_real_pio(snet, snode) == 0) {
        slatch = latch_from_node(snode);
        if (type == PRIMARY_INPUT || slatch == NIL(latch_t)) {
	  continue;
	}
	sfanin = node_get_fanin(snode, 0);
	bfanin = find_or_create_mapped_node(bnet, nmap, sfanin);
	bnode = network_add_fake_primary_output(bnet, bfanin);

	bpi = find_or_create_mapped_node(bnet, nmap, latch_get_output(slatch));
	network_change_node_type(bnet, bpi, PRIMARY_INPUT);

	network_create_latch(bnet, &blatch, bnode, bpi);
	latch_set_initial_value(blatch, latch_get_initial_value(slatch));
	latch_set_current_value(blatch, latch_get_current_value(slatch));
	latch_set_type(blatch, latch_get_type(slatch));
	latch_set_gate(blatch, latch_get_gate(slatch));
	scontrol = latch_get_control(slatch);
	if (scontrol != NIL(node_t)) {
	  scontrol = node_get_fanin(scontrol, 0);
	  bcontrol = find_or_create_mapped_node(bnet, nmap, scontrol);

	  bnode = network_get_control(bnet, bcontrol);
	  if (bnode == NIL(node_t)) {
	    bnode = network_add_fake_primary_output(bnet, bcontrol);
	  }
	}
	else {
	  bnode = NIL(node_t);
	}
	latch_set_control(blatch, bnode);
	continue;
      }

      switch (type) {
      case PRIMARY_INPUT:
        if (st_is_member(nmap, (char *) snode) == 0) {
	  (void) fprintf(siserr,
	  	"Warning: `%s', \".subckt %s\": formal input \"%s\" not driven\n",
		bname, sname, io_node_name(snode));
	}
	break;
      case PRIMARY_OUTPUT:
	/*
	 * This test is wrong: The PO node was never used as a key to an
	 * entry in the hash table... So the test is bound to fail....
	 */
	/*
        if (st_is_member(nmap, (char *) snode) == 0 &&
			network_is_real_po(snet, snode) != 0) {
	  (void) fprintf(siserr,
	  	"Warning: `%s', \".subckt %s\": formal output \"%s\" does not fanout\n",
		bname, sname, io_node_name(snode));
	}
	*/
	break;
      case INTERNAL:
        bnode = find_or_create_mapped_node(bnet, nmap, snode);
	new_node = node_dup(snode);
	node_replace(bnode, new_node);
	foreach_fanin (snode, n, sfanin) {
	  bfanin = find_or_create_mapped_node(bnet, nmap, sfanin);
	  (void) node_patch_fanin(bnode, sfanin, bfanin);
	}
        node_map_copy(snode, bnode);

	/* Run node_scc on each interface node in the hierarchy.  This is
           because the interface nodes could have the function a*a'.  If this
           is true, then when "print" is run which calls node_make_common_base,
	   the number of fanins is one, but the function has no cubes.  This
           causes an error.  Running node_scc on the interface nodes fixes
           this.  Note that while reading the networks, node_scc is called on
           all the internal nodes, so this is consistent. */
	/* For mapped circuits, this is still a problem, but it always has
           been.  We don't want to change the mapping to fix the problem. */

	if (lib_gate_of(bnode) == NIL(lib_gate_t)) {
	    node_scc(bnode);
	}

      }
    }
    array_free(bactuals);
    bpatch->actuals = NIL(array_t);
    st_free_table(nmap);
  }
  
  if (lsLength(entry->patch_lists) > 0) {
    st_free_table(controls);
  }

#endif /* SIS */

  (void) lsDestroy(entry->patch_lists, patch_free);
  entry->patch_lists = NULL;

  snet = entry->network;
  if (snet != sfd->master) {
    network_free(snet);
  }

  FREE(entry);
  FREE(sname);
  sfd->patch_return = PATCH_OK;
  return(ST_DELETE);
}


#ifdef SIS
static void
node_map_copy(from, to)
node_t *from, *to;
{
    int nin, i;
    node_t *fanin;
    lib_gate_t *gate;
    node_t **actuals;
    char **formals;

    gate = lib_gate_of(from);
    if (gate == NIL(lib_gate_t)) {
      return;
    } else {
      nin = node_num_fanin(to);
      actuals = ALLOC(node_t *, nin);
      formals = ALLOC(char *, nin);
      foreach_fanin(to, i, fanin) {
	actuals[i] = fanin;
	formals[i] = lib_gate_pin_name(gate, i, 1);
      }
      assert(lib_set_gate(to, gate, formals, actuals, nin));
      FREE(actuals);
      FREE(formals);
    }
}
#endif


void
dc_network_check(network, status)
network_t *network;
int *status;
{
    network_t *dc_net;
    int po_cnt;
    node_t *node, *po, *pnode;
    node_t *dcpo;
    node_t *pi, *dcfanin;
    lsGen gen;
    char *name;

    dc_net = network->dc_network;
#ifdef SIS
    foreach_primary_output(network, gen, po){
        if(network_is_real_po(network, po)){
          continue;
        }
        node = po->fanin[0];
        name = io_name(po, 0);
        po_cnt = io_rpo_fanout_count(node, &pnode);
        if (node_function(node) == NODE_PI){
            continue;
        }
        if (po_cnt == 0){
        /* name of the latch is stored in the fanin of po*/
            if (!(dcpo = network_find_node(dc_net, node->name))){
                continue;
            }
            if (node_function(dcpo) == NODE_PO){
                name = util_strsav(po->name);
                network_change_node_name(dc_net, dcpo, name);
                continue;
            }else{
                read_error("fatal: %s is not a primary output in the external don't care network", node->name);
                *status = 0;
                network_free(network);
                return;
            }
        }
        /*name of the latch is stored in a real po*/
        if (!(dcpo = network_find_node(dc_net, pnode->name))){
            continue;
        }
        dcfanin = node_get_fanin(dcpo, 0);
        node = node_literal(dcfanin, 1);
        node->name = util_strsav(po->name);
        network_add_node(dc_net, node);
        (void) network_add_primary_output(dc_net, node);
    }
#endif /* SIS */
    foreach_primary_output(dc_net, gen, dcpo){
	    if (!(network_find_node(network, dcpo->name))){
            read_error("fatal: output %s appears only in the external don't care network.\n",
            dcpo->name);
            *status = 0;
            network_free(network);
            return;
        }
    }
    foreach_primary_input(dc_net, gen, pi){
        if (network_find_node(network, pi->name))
            continue;
        read_error("fatal: input %s appears only in the external don't care  network.\n", pi->name);
        *status = 0;
        network_free(network);
        return;
    }
}
