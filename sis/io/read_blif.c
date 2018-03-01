/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/io/read_blif.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:28 $
 *
 */
#include "sis.h"
#include "io_int.h"

/*
 *  bug list: 
 *	- blif cannot describe a single gate which feeds multiple outputs
 */

static int read_node_list();
static int read_cover();
static int read_blif_gate();
extern int read_line();

#ifdef SIS
static int read_blif_subckt();

static int read_blif_latch();
static int read_blif_clock();
static int read_blif_clock_event();
static int read_blif_code();
#endif

/* ### hack, reach in and grab this from the node package */
extern void node_replace_internal();

static int print_stuff = 0;	/* debugging */

/*
 * Mini input processor for blif files. (Also could be used by read_kiss)
 * Reads the next newline or EOF terminated string from `fp' into `line'.
 *
 * Returns EOF if nothing was placed in `line' and EOF has been reached.
 * Blank lines are eliminated and not returned in `line'.
 * '\' followed immediately by a newline is treated as a single space.
 * Comments begin with a '#' at any position on a line and end with newline.
 *
 * In `line' each word is seperated by exactly one space, and there are no
 * initial or terminal spaces.  This is the necessary condition for
 * gettoken() to work.
 */

io_do_prompt2 (fp)
FILE *fp;
{
    char *prompt2;

    if (fp == stdin) {
	prompt2 = com_get_flag ("prompt2");
	if (prompt2 != NULL) fputs (prompt2,stdout);
    }
}

int
read_line(fp, linep, lenp)
FILE *fp;
char **linep;
int *lenp;
{
    int count, c;
    char *line;
    int len;

    count = 0;
    len = *lenp; 
    line = *linep;
    if (read_lineno == 0) io_do_prompt2(fp);

    while ((c = getc(fp)) != EOF) {
        if (count >= len) {
	    len += MAX_WORD;
	    *linep = REALLOC(char, *linep, len);
	    line = &(*linep)[count];
	    *lenp = len;
	}
        if (c == '\\') {
	    if ((c = getc(fp)) == '\n') {
	        read_lineno++;
		io_do_prompt2 (fp);
		c = ' ';
	    }
	}
	else if (c == '#') {
	    while ((c = getc(fp)) != EOF && c != '\n') {
	    }
	    if (c == EOF) {
	        return(EOF);
	    }
	}
	if (c == '\n' && count != 0) {	/* got a non empty line */
	    read_lineno++;
	    io_do_prompt2 (fp);
	    if (line[-1] == ' ') {
	        line--;
	    }
	    *line = '\0';
	    if (print_stuff != 0) {
	        (void) printf("%s\n", *linep);
	    }
	    break;
	}
	if (isspace(c)) {
	    if (c == '\n') {
	        read_lineno++;
		io_do_prompt2 (fp);
	    }
	    if (count == 0 || line[-1] == ' ') {
	        continue;
	    }
	    c = ' ';
	}
	*line++ = c;
	count++;
    }
    return((count == 0) ? EOF : !EOF);
}

/*
 * Special variable just used by lib_read_blif() when reading in libraries.
 */
static int just_the_first = 0;

/* needed when returning from a .exdc */
char exdc_ret[128];

/*
 *  read_blif: read a blif description of a network.  
 *
 *  returns:
 *	-1  if end-of-file reached before a network has been read (not EOF !)
 *	 0  if an error occured during reading
 *	 1  if no errors
 *
 *  Always check error_string() for any error/warning messages
 *
 *  We return immediately on any fatal error with error_string() containing
 *  a somewhat useful error message.
 */
static int 
read_blif_loop(fp, models, search_files, first_network)
FILE *fp;
st_table *models, *search_files;
network_t **first_network;
{
    int c, i, nlist, nin, nout, nterm, got_something, line_len;
    char *line, *word, *rest, *got_model_name;
    node_t *output, *node, **list;
    lsList po_list;
    network_t *network;
    int error_status, in_model, fake_model_names;
    modelinfo *entry;
#ifdef SIS
    double cycle_time;
    graph_t *stg;
    lsList latch_order_list;
#endif /* SIS */

    /* Initialize the network */
    got_something = 0;

    if (first_network != NIL(network_t *)) {
        *first_network = NIL(network_t);
    }

    network = NIL(network_t);

#ifdef SIS
    latch_order_list = NULL;
#endif /* SIS */

    /*
     * We record the primary outputs on a special list because of the
     * difficulties of mapping blif into our network structure which includes
     * an extra 'PO' node.
     */
    error_status = 1;
    in_model = 0;
    got_model_name = NIL(char);
    exdc_ret[0] = '\0';
    fake_model_names = 0;

    line_len = 0;
    line = NIL(char);

    while (read_line(fp, &line, &line_len) != EOF) {
loop:
        word = gettoken(line, &rest);
	if (*word == '.') {
	    word++;

		/*
		 * These can appear outside of model definition
		 */
	    if (strcmp(word, "model") == 0 || strcmp(word, "circuit") == 0) {
		got_model_name = gettoken(NIL(char), NIL(char *));
		if (network != NIL(network_t) && just_the_first != 0) {
		    strcpy(exdc_ret, got_model_name);
		    break;
		}
		in_model = 1;
		goto got_model;

	    }
#ifdef SIS
	    else if (strcmp(word, "search") == 0) {
	        word = gettoken(NIL(char), NIL(char *));
		error_status = read_search_file(word, models, search_files,
                                                read_blif_loop);
		if (error_status != 1) {	/* propagate an error */
		    break;
		}
		continue;
	    }
#endif /* SIS */
		/*
		 * Everything down here has to be inside a model.  If there
		 * isn't a model, create one.  Illegal to have two unnamed
		 * models in one file.
		 */
	    if (strcmp(word, "exdc") == 0) {
	        if (in_model == 0) {
		    read_error("external don't care w/o associated network");
		    error_status = 0;
		    break;
		    }
		i = read_blif_first(fp, &network->dc_network);
		if (i != 1) {
		    read_error("bad/missing.sh external don't care network");
		    network->dc_network = NIL(network_t);
		    error_status = 0;
		    break;
		}
		if (exdc_ret[0] == '\0') {
		    continue;
		}
		got_model_name = exdc_ret;
		in_model = 1;
		goto got_model;
	    }		
	    
	    if (in_model == 0) {
got_model:        if (got_model_name == NIL(char)) {
		    if (fake_model_names != 0) {
		        read_error("too many unnamed models");
		        error_status = 0;
			break;
		    }
		    fake_model_names++;
		    got_model_name = strrchr(read_filename, '/');
		    if (got_model_name == NIL(char)) {
		        got_model_name = read_filename;
		    }
		    else {
		        got_model_name++;
		    }
		}
		entry = read_find_or_create_model(got_model_name, models);

		if (entry->network != NIL(network_t)) {
		    read_error("model %s already defined", got_model_name);
		    error_status = 0;
		    break;
		}

		entry->network = network_alloc();
		network_set_name(entry->network, got_model_name);

		got_something = 1;
		network = entry->network;
		po_list = entry->po_list;
		if (first_network != NIL(network_t *)) {
		    if (*first_network == NIL(network_t)) {
		        *first_network = network;
		    }
		}
		if (in_model == 1) {
		    continue;
		}
		in_model = 1;

	    }
	    if (strcmp(word, "names") == 0 || strcmp(word, "cover") == 0) {
		if (strcmp(word, "cover") == 0) {
		    if (sscanf(rest, "%d %d %d", &nin, &nout, &nterm) != 3) {
			read_error(".cover requires nin, nout, nterm");
			error_status = 0;
			break;
		    }
		    /* gettoken must be called to move the pointer to */
		    /* the input stream up to the node names */
		    for (i = 0; i<=2; i++) {
			word = gettoken(NIL(char), NIL(char *));
		    }
		    if (nout != 1) {
			read_error("only single-output .cover allowed");
			error_status = 0;
			break;
		    }
		}
		got_something = 1;
		nlist = read_node_list(network, &list);
		if (nlist == 0) {
		    read_error("Too few names following .names");
		    error_status = 0;
		    break;
		}
		nlist--;
		output = list[nlist];
		if (node_function(output) != NODE_UNDEFINED) {
		    read_error("fatal: output '%s' is multiply defined",
			    node_name(output));
		    error_status = 0;
		    break;
		}

		i = read_cover(fp, output, list, nlist, &line, &line_len);
		if (i == EOF) {
		    break;
		}
		else if (i == 0) {
		    error_status = 0;
		    break;
		}
		goto loop;    /* don't want to read in the next line of file */

	    }
	    else if (strcmp(word, "gate") == 0) {	/* library model */
		got_something = 1;
		if (read_blif_gate(network, 0) == 0) {
		    error_status = 0;
		    break;
		}

	    }
#ifdef SIS
	    else if (strcmp(word, "subckt") == 0) {
	        got_something = 1;
		if (read_blif_subckt(network, models) == 0) {
		    error_status = 0;
		    break;
		}
		entry->depends_on++;
	    }
#endif /* SIS */
	    else if (strcmp(word, "inputs") == 0) {
		got_something = 1;
		while ((word = gettoken(NIL(char), NIL(char *))) != 0) {
		    node = read_find_or_create_node(network, word);
		    if (node_function(node) != NODE_UNDEFINED) {
			read_error("input '%s' is multiply defined", word);
			error_status = 0;
			break;
		    }
		    network_change_node_type(network, node, PRIMARY_INPUT);
		}

	    }
	    else if (strcmp(word, "outputs") == 0) {
		got_something = 1;
		while ((word = gettoken(NIL(char), NIL(char *))) != 0) {
		    node = read_find_or_create_node(network, word);
		    (void) lsNewEnd(po_list, (lsGeneric) node, LS_NH);
		}

#ifdef SIS

	    } else if (strcmp(word, "latch") == 0) {
		got_something = 1;
		if (read_blif_latch(network) == 0) {
		    error_status = 0;
		    break;
		}

	    } else if (strcmp(word, "mlatch") == 0) {
		got_something = 1;
		if (read_blif_gate(network, 1) == 0) {
		    error_status = 0;
		    break;
		}

	    } else if (strcmp(word, "start_kiss") == 0) {
		got_something = 1;

		if (read_kiss(fp, &stg) == 0) {
		    error_status = 0;
		    break;
		}
		network_set_stg(network, stg);

	    } else if (strcmp(word, "latch_order") == 0) {
	        if (entry->latch_order_list == NULL) {
		    entry->latch_order_list = lsCreate();
		}
		latch_order_list = entry->latch_order_list;
	        while ((word = gettoken(NIL(char), NIL(char *))) != 0) {
		    node = read_find_or_create_node(network, word);
		    (void) lsNewEnd(latch_order_list, (lsGeneric) node, LS_NH);
		}

	    } else if (strcmp(word, "code") == 0) {
	        if (read_blif_code(network) == 0) {
		    error_status = 0;
		    break;
		}

	    } else if (strcmp(word, "cycle") == 0) {
	        if (sscanf(rest, "%lf", &cycle_time) != 1) {
		    read_error(".cycle requires single real argument");
		    error_status = 0;
		    break;
		}
		clock_set_cycletime(network, cycle_time);
		
	    } else if (strcmp(word, "clock") == 0 ||
		       strcmp(word, "clocks") == 0) {
	        if (read_blif_clock(network) == 0) {
		    error_status = 0;
		    break;
		}

	    } else if (strcmp(word, "clock_event") == 0) {
	        if (read_blif_clock_event(network) == 0) {
		    error_status = 0;
		    break;
		}

#endif /* SIS */
	    } else if (strcmp(word, "end") == 0) {
	        in_model = 0;
		if (just_the_first != 0) {
		    break;
		}

	    }
	    else {
		c = read_delay(network, po_list, word, rest, 00);
		if (c == -1) {
		    /* echo line with unknown keyword */
		    word--;
		    goto huh;
		}
		else if (c == 0) {
		    /* error parsing delay-related keyword */
		    error_status = 0;
		    break;
		}
	    }
	}
	else {
huh:
	    (void) fprintf(siserr, "\"%s\", line %d: %s %s\n", read_filename,
	    		read_lineno, word, rest);
	}
    }

    if (got_something == 0) {
        error_status = -1;
    }
    FREE(line);
    return(error_status);
}

/*
 * The exported routine for everyone's use.
 */
int
read_blif(fp, networkp)
FILE *fp;
network_t **networkp;
{
    int status;

    just_the_first = 0;
    status = read_blif_slif(fp, networkp, read_blif_loop);
    if (status != 0 && *networkp != NIL(network_t)) {
        read_cleanup_buffers(*networkp);
    }
    return status;
}

/*
 * Just read the first .model defined in the given file.  Used by
 * lib_read_blif().
 */
int
read_blif_first(fp, networkp)
FILE *fp;
network_t **networkp;
{
    int i;
    
    just_the_first = 1;
    i = read_blif_slif(fp, networkp, read_blif_loop);
    just_the_first = 0;
    return i;
}

/*
 * Accumulate nodes from the input stream.
 *
 * Nodes are inserted into `network' & accumulated onto dynamically allocated
 * `list'.  `list' should be freed after use.
 * Returns the number of nodes processed.
 */
static int
read_node_list(network, list)
network_t *network;
node_t ***list;
{
    int nlist, list_max;
    char *word;
    node_t *node;

    list_max = 10;
    *list = ALLOC(node_t *, list_max);
    nlist = 0;
    while ((word = gettoken(NIL(char), NIL(char *))) != NIL(char)) {
        node = read_find_or_create_node(network, word);
	if (nlist >= list_max) {
	    list_max *= 2;
	    *list = REALLOC(node_t *, *list, list_max);
	}
	(*list)[nlist++] = node;
    }
    return(nlist);
}

/*
 * Reads in a single output cover.  Currently either the ON-set or the OFF-set
 * may be specified; DC-set is not supported.  Reads until it finds a line
 * beginning with ".", which means it has read one line too many.  Uses the
 * `linep' and `lenp' variables to return the extra line and its length.
 */
static int 
read_cover(fp, node, fanin, ninput, linep, lenp)
FILE *fp;
node_t *node;
node_t **fanin;
int ninput;
char **linep;		/* reads one line too many. returns it here */
int *lenp;
{
    register int i, c;
    register pset pdest;
    pset_family f;
    char *word, *rest;
    int is_normal, is_compl;

    is_compl = 0;			/* records some table has "0" output */
    is_normal = 0;			/* records some table has "1" output */

    f = sf_new(8, ninput * 2);

    while ((c = read_line(fp, linep, lenp)) != EOF) {
	if (**linep == '.') {		/* end table on a command */
	    break;			/* command passed back in line_p */
	}
        word = gettoken(*linep, &rest);
	if (word == NIL(char)) {
	    goto bad_char;
	}
	if (ninput > 0) {
	    if (strlen(word) != ninput || rest == NIL(char)) {
	        goto bad_char;
	    }
	    pdest = set_new(ninput * 2);
	    for (i = 0; i < ninput; i++) {
		switch (word[i]) {
		case '0':
		    set_insert(pdest, 2 * i);
		    break;
		case '1':
		    set_insert(pdest, 2 * i + 1);
		    break;
		case '2':
		case '-':
		    set_insert(pdest, 2 * i);
		    set_insert(pdest, 2 * i + 1);
		    break;
		default:
		    goto bad_char;
		}
	    }
	    if (strcmp(rest, "1") == 0) {
		if (is_compl) {
		    read_error("must give F or R, but not both");
		    return(0);
		}
		f = sf_addset(f, pdest);
		set_free(pdest);
		is_normal = 1;
	    } else if (strcmp(rest, "0") == 0) {
		if (is_normal) {
		    read_error("must give F or R, but not both");
		    return(0);
		}
		f = sf_addset(f, pdest);
		set_free(pdest);
		is_compl = 1;
	    } else {
		goto bad_char;
	    }

	} else {
	    if (strcmp(word, "1") == 0) {
		f->count = 1;		/* the '1' function */ 
	    } else if (strcmp(word, "0") == 0) {
				        /* the '0' function */
	    } else {
		goto bad_char;
	    }
	}
    }

    node_replace_internal(node, fanin, ninput, f);
    if (is_compl) {
	node_replace(node, node_not(node));
    }
    node_scc(node);		/* make it scc-minimal, dup_free, etc. */
    return((c == EOF) ? EOF : 1);

bad_char:
    read_error("bad character in PLA table");
    return(0);
}


/*
 * Accumulate space seperated strings of characters (words) from the input
 * stream.
 *
 * Elements of `wordlist' point into the tokenized input string; the elements
 * should not be freed.  `wordlist' should be freed after use.
 * Returns the number of words read.
 */
static int
read_wordlist(wordlist)
char ***wordlist;
{
    int i = 0;
    int list_max = 10;
    char *word;

    *wordlist = ALLOC(char *, list_max);
    while ((word = gettoken(NIL(char), NIL(char *))) != NIL(char)) {
    	if (i == list_max) {
	    list_max *= 2;
	    *wordlist = REALLOC(char *, *wordlist, list_max);
	}
	(*wordlist)[i++] = word;
    }
    return(i);
}

#ifdef SIS
/*
 * Make `network' depend on something.
 *
 * formats for .subckt:
 *
 * .subckt bozo formal1=actual1 formal2=actual2 ...
 */
static int
read_blif_subckt(network, models)
network_t *network;
st_table *models;
{
    char *name, *actual, **word_list;
    modelinfo *ent;
    patchinfo *patch;
    node_t *node;
    int nwords, i;
    
    name = gettoken(NIL(char), NIL(char *));
    if (name == NIL(char)) {
        read_error("missing name in subckt");
	return(0);
    }
    if (strchr(name, '=') != NIL(char)) {
        (void) fprintf(siserr, "\"%s\", line %d: calling subckt %s  (contains `=')\n",
		read_filename, read_lineno, name);
    }
    ent = read_find_or_create_model(name, models);

    nwords = read_wordlist(&word_list);

    patch = ALLOC(patchinfo, 1);
    patch->actuals = array_alloc(node_t *, 10);
    patch->formals = ALLOC(char *, nwords);
    patch->inputs = -1;
    patch->netname = network_name(network);

    for (i = 0; i < nwords; i++) {
	actual = strchr(word_list[i], '=');
	if (actual == NIL(char) || (*actual++ = '\0', *actual == '\0')) {
	    read_error("bad format for formal=actual in .subckt: %s",
	    		word_list[i]);
	    FREE(word_list);
	    return(0);
	}
	node = read_find_or_create_node(network, actual);
	array_insert_last(node_t *, patch->actuals, node);
    }
    for (i = 0; i < nwords; i++) {
        patch->formals[i] = util_strsav(word_list[i]);
    }        
    FREE(word_list); 
    (void) lsNewEnd(ent->patch_lists, (lsGeneric) patch, LS_NH);
    return(1);
}

#endif /* SIS */

/*
 * Reads in a ".gate" or ".mlatch".  `is_mlatch' == 0 if reading in a ".gate".
 */
static int
read_blif_gate(network, is_mlatch)
network_t *network;
int is_mlatch;
{
    char *name, *actual, *word;
    array_t *actual_array;
    char **word_list;
    node_t *node, *tmp_node, **actual_data;
    library_t *library;
    lib_gate_t *gate;
    int i, nwords;
    static char *gm[] = {".gate", ".mlatch"};
#ifdef SIS
    node_t *control;
    latch_t *latch;
    int value;
#endif /* SIS */

    /* get the current library */
    library = lib_get_library();
    if (library == NIL(library_t)) {
        read_error("cannot process %s without a library", gm[is_mlatch]);
	return(0);
    }

    /* read the gate name */
    name = gettoken(NIL(char), NIL(char *));
    if (name == NIL(char)) {
    	read_error("missing.sh gate name in %s", gm[is_mlatch]);
	return(0);
    }
    gate = lib_get_gate(library, name);
    if (gate == NIL(lib_gate_t)) {
	read_error("cannot find gate '%s' in the library", name);
	return(0);
    }

    nwords = read_wordlist(&word_list);

#ifdef SIS
    if (is_mlatch != 0) {
        nwords--;
	value = *word_list[nwords] - '0';
	if (value < 0 || value > 9) {
	    read_error("warning: no latch value specified (undefined assumed)");
	    value = 3;
	}
	else if (value > 3) {
	    read_error("latch value can only be 0, 1, 2, or 3");
	    FREE(word_list);
	    return(0);
	}
	else {
	    nwords--;
	}
	if (strcmp(word_list[nwords], "NIL") == 0) {
	    control = NIL(node_t);
	}
	else {
	    control = read_find_or_create_node(network, word_list[nwords]);
	}
    }
#endif /* SIS */

    actual_array = array_alloc(node_t *, 10);

    for (i = 0; i < nwords; i++) {
        word = word_list[i];
        actual = strchr(word, '=');
	if (actual == NIL(char) || (*actual++ = '\0', *actual == '\0')) {
	    read_error("bad format for formal=actual in %s: %s",
	    		gm[is_mlatch], word_list[i]);
	    FREE(word_list);
	    return(0);
	}
	node = read_find_or_create_node(network, actual);
	array_insert_last(node_t *, actual_array, node);
    }

    actual_data = array_data(node_t *, actual_array);
    array_free(actual_array);

    nwords--;
    node = actual_data[nwords];	    /* ### output pin name not checked ! */

#ifdef SIS
    if (is_mlatch != 0) {
        if (node_type(node) == PRIMARY_INPUT || node->F != 0) {
	    read_error("node %s already defined", node_name(node));
	    FREE(word_list);
	    FREE(actual_data);
	    return(0);
	}
	else {
	    network_change_node_type(network, node, PRIMARY_INPUT);
	}
    }	
#endif /* SIS */

    if (lib_set_gate(node, gate, word_list, actual_data, nwords) == 0) {
	read_error("error loading gate '%s' (have the correct library?)",
		name);
	FREE(word_list);
	FREE(actual_data);
	return(0);
    }
    FREE(word_list);

#ifdef SIS
    /*
     * HACK:: To handle the case of nodes added when the input to an
     * mlatch is a primary input. The name of the internal node added
     * may conflict with the name of some node yet to be considered 
     */
    if (is_mlatch) {
	tmp_node = latch_get_input(latch_from_node(node));
	tmp_node = node_get_fanin(tmp_node, 0);
	assert(tmp_node->type == INTERNAL);
	read_change_madeup_name(network, tmp_node);
    }
#endif /* SIS */


#ifdef SIS
    if (is_mlatch != 0) {
        latch = latch_from_node(node);
        read_change_madeup_name(network, latch_get_input(latch));
	if (control != NIL(node_t)) {
	    node = network_get_control(network, control);
	    if (node == NIL(node_t)) {
	        node = network_add_fake_primary_output(network, control);
		read_change_madeup_name(network, node);
	    }
	    control = node;
	}
	latch_set_control(latch, control);
	latch_set_initial_value(latch, value);
	latch_set_current_value(latch, value);
    }
#endif /* SIS */

    FREE(actual_data);
    return(1);
}

#ifdef SIS
/*
 * An unspecified latch value is set to 3.
 * Allow input of 0, 1, 2, or 3 for latch value, 2=DONT_CARE, 3=UNSPECIFIED
 */
static int
read_blif_latch(network)
network_t *network;
{
    int n_params, value;
    char **word_list;
    node_t *input, *output, *hack_po, *control, *n;
    latch_t *latch;
    latch_synch_t type;

    n_params = read_wordlist(&word_list);

    if (n_params < 2) {
        read_error("latch input and output must be specified");
	FREE(word_list);
	return(0);
    }
    if (n_params > 5) {
        read_error("too many parameters for .latch");
	FREE(word_list);
	return(0);
    }

    input = read_find_or_create_node(network, word_list[0]);
    output = read_find_or_create_node(network, word_list[1]);

    if ((n_params & 01) == 0) {			/* 2 or 4 */
        read_error("warning: no latch value specified (undefined assumed)");
	value = 3;
	type = UNKNOWN;
	control = NIL(node_t);
    }
    else {					/* 3 or 5 */
        value = *word_list[n_params - 1] - '0';
	if (value < 0 || value > 3) {
	    read_error("latch value can only be 0, 1, 2, or 3");
	    FREE(word_list);
	    return(0);
	}
	if (n_params == 3) {
	    type = UNKNOWN;
	    control = NIL(node_t);
	}
    }
    if (n_params >= 4) {
        if (strcmp(word_list[3], "NIL") == 0) {
	    control = NIL(node_t);
	}
	else {
	    control = read_find_or_create_node(network, word_list[3]);
	}
	if (strcmp(word_list[2], "fe") == 0) 	  type = FALLING_EDGE;
	else if (strcmp(word_list[2], "re") == 0) type = RISING_EDGE;
	else if (strcmp(word_list[2], "ah") == 0) type = ACTIVE_HIGH;
	else if (strcmp(word_list[2], "al") == 0) type = ACTIVE_LOW;
	else if (strcmp(word_list[2], "as") == 0) type = ASYNCH;
	else {
	    read_error("latch type can only be re/fe/ah/al/as");
	    FREE(word_list);
	    return(0);
	}
    }
    FREE(word_list);

    if (node_type(output) == PRIMARY_INPUT || output->F != 0) {
        read_error("latch output %s is already defined", node_name(output));
	return(0);
    }
    network_change_node_type(network, output, PRIMARY_INPUT);

    hack_po = network_add_fake_primary_output(network, input);
    read_change_madeup_name(network, hack_po);

    network_create_latch(network, &latch, hack_po, output);
    latch_set_initial_value(latch, value);
    latch_set_current_value(latch, value);
    latch_set_type(latch, type);

    if (control != NIL(node_t)) {
        n = network_get_control(network, control);
        if (n == NIL(node_t)) {
	    n = network_add_fake_primary_output(network, control);
	    read_change_madeup_name(network, n);
	}
	control = n;
    }
    latch_set_control(latch, control);

    return(1);
}

static int
read_blif_clock(network)
network_t *network;
{
    int i, nlist;
    node_t *node;
    char **word_list;
    sis_clock_t *clock;

    nlist = read_wordlist(&word_list);
    for (i = 0; i < nlist; i++) {
        node = read_find_or_create_node(network, word_list[i]);
	if (node_function(node) != NODE_UNDEFINED) {
	    read_error("clock %s is multiply defined", node_name(node));
	    FREE(word_list);
	    return(0);
	}
	network_change_node_type(network, node, PRIMARY_INPUT);
	clock = clock_create(node_long_name(node));
	if (!clock_add_to_network(network, clock)) {
	    read_error("clock %s is already part of the network",
	    		clock_name(clock));
	    clock_free(clock);
	    FREE(word_list);
	    return(0);
	}
    }
    FREE(word_list);
    return (1);
}

static int
read_blif_clock_event(network)
network_t *network;
{
    double nominal, min, max;
    int first = 1;
    char *word, *rest;
    clock_edge_t first_edge, edge;

    word = gettoken(NIL(char), &rest);
    if (sscanf(word, "%lf", &nominal) != 1) {
        read_error(".clock_event must have event time as first argument");
	return(0);
    }
	
	/*	parse clock events	*/

    while ((word = gettoken(rest, &rest)) != NIL(char)) {
        if (*word == '(') {		/* min and max specified */
	    word++;
	    if (sscanf(rest, "%lf %lf", &min, &max) != 2) {
	        read_error("min and max values incorrectly specified for clock edge %s",
			word);
		return(0);
	    }
	    rest = strchr(rest, ')');
	    if (rest == NIL(char)) {
	        read_error("warning: missing parentheses in event specifcation for clock edge %s",
			word);
		return(0);
	    }
	    (void) gettoken(rest, &rest);
	}
	else {				/* min and max not specified */
	    min = max = 0;
	}
	if (*word == 'r') {
	    edge.transition = RISE_TRANSITION;
	}
	else if (*word == 'f') {
	    edge.transition = FALL_TRANSITION;
	}
	else {
	    read_error("transition must be r or f in .clock_event %s", word);
	    return(0);
	}

	if ((edge.clock = clock_get_by_name(network, word + 2)) == 0) {
	    read_error("clock %s not found in clock_list", word + 2);
	    return(0);
	}
	(void)clock_set_parameter(edge, CLOCK_NOMINAL_POSITION, nominal);
	(void)clock_set_parameter(edge, CLOCK_LOWER_RANGE, min);
	(void)clock_set_parameter(edge, CLOCK_UPPER_RANGE, max);

	if (first) {
	    first = 0;
	    first_edge.transition = edge.transition;
	    first_edge.clock = edge.clock;
	}
	else {
	    (void) clock_add_dependency(first_edge, edge);
	}
    }
    return(1);
}

static int
read_blif_code(network)
network_t *network;
{
    int nlist;
    char **word_list;
    graph_t *stg;
    vertex_t *v;

    nlist = read_wordlist(&word_list);
    if (nlist != 2) {
        read_error(".code should have exactly 2 arguments");
	FREE(word_list);
	return(0);
    }
    if ((stg = network_stg(network)) == NIL(graph_t)) {
        read_error("stg not present, cannot set code");
	FREE(word_list);
	return(0);
    }
    if ((v = stg_get_state_by_name(stg, word_list[0])) == NIL(vertex_t)) {
        read_error("state %s not present, code cannot be set", word_list[0]);
	FREE(word_list);
	return(0);
    }
    stg_set_state_encoding(v, word_list[1]);
    FREE(word_list);
    return(1);
}

#endif /* SIS */

