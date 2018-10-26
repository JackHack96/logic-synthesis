
#ifdef SIS

#include "sis.h"
#include "io_int.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#define INCLUDE_MAX	6		/* max include nesting */

static int read_slif_nodelist();
static int read_slif_latch();

static int
read_statement(fp,str,len)
FILE *fp;
char *str;
int len;
{
    int c, last, count;
    char *start = str;

    last = ' ';
    count = 0;
    len -= 3; 	/* safety margin */

    while (count < len && (c = getc(fp)) != EOF) {
	if (c == '#') {
	    while ((c = getc(fp)) != EOF && c != '\n') {
	    }
	    if (c == EOF) {
	        return(EOF);
	    }
	    /* fall into isspace */
	}
	if (isspace(c)) {
	    if (c == '\n') {
	        read_lineno++;
	    }
	    if (last == ' ') {
	        continue;
	    }
	    c = ' ';
	}
	else if (c == ';') {
	    if (count > 0 && *start == '.') {	/* command: throw out ';' */
	        if (last == ' ') {
		    str--;
		}
	    }
	    else {				/* eqn: keep ';' */
	        *str++ = ';';
	    }
	    *str = '\0';
	    return(!EOF);
	}
	else if (c == '(' || c == ')' || c == ',') {
	    if (last != ' ') {
	        *str++ = ' ';  count++;
	    }
	    *str++ = c;  *str++ = ' ';  count += 2;  last = ' ';
	    continue;
	}
	*str++ = c;  count++;  last = c;	
    }
    if (count != 0) {
        read_error("incomplete last line");
    }
    return(EOF);
}

typedef struct {
    FILE *fp;
    int lineno;
    char *name;
} slif_context;

/*
 * Returns:
 *	0 if encountered an error.
 *	1 otherwise
 *
 * If first_model = NIL(network_t *) then first_model points to the
 *   first model in the file.  If no models were encountered, then
 *   first_model points to NIL(network_t).
 */
static int
read_slif_loop(fp, models, search_files, first_model)
FILE *fp;
st_table *models, *search_files;
network_t **first_model;
{
    int in_model, include_depth, have_function, i, exit_status, global;
    char line[MAX_WORD], buf[MAX_WORD];
    slif_context saved_context[INCLUDE_MAX], *save;
    modelinfo *entry;
    char *command, *arg1, *name, *rest;
    network_t *network, *net;
    node_t *node, *po, *n2, *n3;
    lsList po_list;
    lsGen gen;

    in_model = 0;
    include_depth = 0;
    network = NIL(network_t);
    exit_status = 1;

    if (first_model != NIL(network_t *)) {
        *first_model = NIL(network_t);
    }

    while ((i = read_statement(fp, line, sizeof(line))) != EOF || include_depth > 0) {
        if (i == EOF) {

	/* restore context on end of include file */

	    FREE(read_filename);
	    if (fp != stdin) {
	        (void) fclose(fp);
	    }
	    save = &saved_context[--include_depth];
	    fp = save->fp;
	    read_lineno = save->lineno;
	    read_filename = save->name;
	}

	else if (*line == ';' || *line == '\0') {
	    read_error("null statement");
	}
	else if (*line == '.') {
	    command = gettoken(line, NIL(char *)); /* get first word in line */
	    command++;				/* skip past initial '.' */
	    arg1 = gettoken(NIL(char), &rest);	/* next token */

	/* These commands can appear outside of model declaration */

	    if (strcmp(command, "model") == 0) {
	        if (in_model != 0) {
		    read_error(".model encountered within another .model");
		    exit_status = 0;
		    break;
		}
		if (arg1 == NIL(char)) {
		    read_error("missing name in .model construct");
		    exit_status = 0;
		    break;
		}

		entry = read_find_or_create_model(arg1, models);
		
		if (entry->network != NIL(network_t)) {
		    read_error("model %s already defined", arg1);
		    exit_status = 0;
		    break;
		}

		entry->network = network_alloc();
		network_set_name(entry->network, arg1);	   /* auto strsav */

	        in_model = 1;
		network = entry->network;
		po_list = entry->po_list;		    

		if (first_model != NIL(network_t *) && *first_model == NIL(network_t)) {
		    *first_model = network;
		}			
	    }

	    else if (strcmp(command, "include") == 0) {
	        if (include_depth >= INCLUDE_MAX) {
		    read_error("maximum include depth exceeded");
		    exit_status = 0;
		    break;
		}
		if (arg1 == NIL(char)) {
		    read_error("no file name specified after .include");
		    exit_status = 0;
		    break;
		} 

		/*
		 * Save current context, then treat the include file as if it
		 * were a part of this file.
		 */
		save = &saved_context[include_depth++];
		save->fp = fp;
		save->lineno = read_lineno;
		save->name = read_filename;

		fp = com_open_file(arg1, "r", &name, 1);

		if (fp == NIL(FILE)) {
		    FREE(name);
		    read_error("include file %s not found", arg1);
		    exit_status = 0;
		    break;
		}
		read_lineno = 1;
		read_filename = name;
	    }

	    else if (strcmp(command, "search") == 0) {
	        exit_status = read_search_file(arg1, models, search_files,
					       read_slif_loop);
		if (exit_status == 0) {		/* propagate an error */
		    break;
		}
	    }

	    	/* must be inside a model for all following commands */
		
	    else if (in_model == 0) {
	        read_error("Illegal statement outside of .model: %s", line);
		exit_status = 0;
		break;
	    }

	    else {
	      if (!strcmp(command, "attribute") || !strcmp(command, "global_attribute")) {
		if (arg1 == NIL(char)) {
		    read_error("no attribute type specified");
		    exit_status = 0;
		    break;
		}
		global = (strcmp(command, "global_attribute") == 0);
		i = read_delay(network, po_list, arg1, rest, global + 10);
		if (i < 1) {
		    if (i == -1) {
		        read_error("illegal %s: %s", command, rest);
		    }
		    exit_status = 0;
		    break;
		}
	      }

	      else if (strcmp(command, "call") == 0) {
	        if (read_slif_call(fp, network, models, arg1) == 0) {
		    exit_status = 0;
		    break;
		}
		entry->depends_on++;
	      }

	      else if (strcmp(command, "endmodel") == 0) {
	        in_model = 0;		/* not in a model def anymore */
		if (entry->library == 1 && lsLength(po_list) != 1) {
		    read_error("library module can only have 1 primary output");
		    exit_status = 0;
		}
	      }
	      
	      else if (strcmp(command, "inouts") == 0) {
	        read_error("inouts not allowed");
		exit_status = 0;
		break;
	      }

	      else if (strcmp(command, "inputs") == 0) {
	        while (arg1 != NIL(char)) {
		    node = read_find_or_create_node(network, arg1);
		    if (node_function(node) != NODE_UNDEFINED) {
		        read_error("node %s multiply defined", arg1);
			exit_status = 0;
			break;
		    }
		    network_change_node_type(network, node, PRIMARY_INPUT);
		    arg1 = gettoken(NIL(char),NIL(char *));
		}
	      }

	      else if (strcmp(command, "library") == 0) {
	        entry->library = 1;
		continue;
	      }

	      else if (strcmp(command, "net") == 0) {
		node = NIL(node_t);
		have_function = 0;

		while (arg1 != NIL(char)) {
		    n2 = read_slif_find_or_create_node(network, arg1, 0);
		    if (node == NIL(node_t)) {
		        node = n2;
		    }
		    else {
		      if (node_function(n2) != NODE_UNDEFINED) {
		        if (have_function == 1) {
			    (void) strcpy(buf, node_name(node));
			    read_error(".net: %s & %s already have functions",
			    		buf, node_name(n2));
			    exit_status = 0;
			    break;
			}
			have_function = 1;
			n3 = n2; n2 = node; node = n3;
		      }

		/* make n2 (no node func) = node (possibly node func) */

		      n3 = node_literal(node, 1);
		      node_replace(n2, n3);
		    }
		    arg1 = gettoken(NIL(char), NIL(char *));
		}
	      }

	      else if (strcmp(command, "outputs") == 0) {
	        while (arg1 != NIL(char)) {
		    node = read_find_or_create_node(network, arg1);
		    (void) lsNewEnd(po_list, (lsGeneric) node, LS_NH);
		    arg1 = gettoken(NIL(char), NIL(char *));
		}
	      }

	      else {
	        (void) fprintf(siserr, "\"%s\", line %d: %s ignored\n",
			read_filename, read_lineno, command - 1);
	      }
	    }
	}
	else {
	    if (in_model == 0) {
	        read_error("Illegal statement outside of .model: %s", line);
		exit_status = 0;
		break;
	    }
	    name = strchr(line, '@');
	    if (name != NIL(char)) {   /* found a latch */
	        if (read_slif_latch(network, line, name) == 0) {
		    exit_status = 0;
		    break;
		}
		continue;
	    }
	    name = read_filename;
	    i = read_lineno;
	    read_filename = NIL(char);
	    net = read_eqn_string(line);
	    read_filename = name;
	    read_lineno = i;

	    if (net == NIL(network_t)) {
	        read_error("%s", line);	
	        exit_status = 0;
		break;
	    }

	    po = network_get_po(net, 0);
	    node = read_find_or_create_node(network, node_long_name(po));

	    po = node_get_fanin(po, 0);
	    n3 = node_dup(po);
	    node_replace(node, n3);			/* new node = old po */

	    if (node_num_fanin(po) != 0) {		/* not constant */
	        foreach_primary_input (net, gen, n2) {
		    n3 = read_find_or_create_node(network, node_long_name(n2));
		    (void) node_patch_fanin(node, n2, n3);
		}
	    }

	    network_free(net);
	}
    }
    if (exit_status != 0 && in_model != 0) {
        read_error("Missing .endmodel");
	exit_status = 0;
    }

    while (include_depth > 0) {		/* unwind any saved stacks */
	FREE(read_filename);
        if (fp != stdin) {
	    (void) fclose(fp);
	}
	save = &saved_context[--include_depth];
	fp = save->fp;
	read_filename = save->name;
    }
    return(exit_status);
}

network_t *
read_slif(fp)
FILE *fp;
{
    network_t *network;

    if (read_blif_slif(fp, &network, read_slif_loop) != 1) {
        return(NIL(network_t));
    }
    read_cleanup_buffers(network);
    return(network);
}

/*
 * 0 error
 * 1 no error
 */
int
read_slif_call(fp, network, models, name)
FILE *fp;
network_t *network;
st_table *models;
char *name;
{
    char *word, *last, line[MAX_WORD];
    array_t *actuals;
    char *rest;
    int error_stat, inputs, inouts;
    modelinfo *ent;
    patchinfo *patch;
    
	/*
	 * get the last argument before the '(' as the model name.
	 */

    if (name == NIL(char)) {
        read_error("missing model name for call");
	return(0);
    }
    last = name;
    while ((word = gettoken(NIL(char), &rest)) != NIL(char)) {
        if (*word == '(') {
	    name = last;
	    break;
	}
	else {
	    last = word;
	}
    }

    ent = read_find_or_create_model(name, models);	/* entry for called */

    patch = ALLOC(patchinfo, 1);
    patch->actuals = NIL(array_t);
    patch->formals = NIL(char *);
    patch->inputs = 0;
    patch->netname = network_name(network);
    (void) lsNewEnd(ent->patch_lists, (lsGeneric) patch, LS_NH);
    
    actuals = array_alloc(node_t *, 10);
    error_stat = 1;    
    inputs = read_slif_nodelist(network, rest, actuals, 0 /* input */);

    if (read_statement(fp, line, MAX_WORD) == EOF) {
        read_error("ran out of arguments in .call");
	error_stat = 0;
    }
    else {
        inouts = read_slif_nodelist(network, line, actuals, 0 /* anything */);
	if (inouts != 0) {
	    read_error("no inouts allowed");
	    error_stat = 0;
	}
	else if (read_statement(fp, line, MAX_WORD) == EOF) {
	    read_error("ran out of arguments in .call");
	    error_stat = 0;
	}
	else {
	    (void) read_slif_nodelist(network, line, actuals, 1 /* output */);
	}
    }

    if (error_stat == 0) {
        array_free(actuals);
    }
    else {
	patch->actuals = actuals;
	patch->inputs = inputs;
    }
    return(error_stat);
}

static int
read_slif_nodelist(network, line, actuals, flag)
network_t *network;
char *line;
array_t *actuals;
int flag;	/* Describes the configuration depending on context to be */
		/* created for complemented nodes encountered 		  */
{
    node_t *node;
    char *word = strtok(line, " ,");
    int i = 0;

    while (word != NIL(char)) {
        if (*word == ')' || *word == ';') {
	    break;
	}
	node = read_slif_find_or_create_node(network, word, flag);
	array_insert_last(node_t *, actuals, node);
	word = strtok(NIL(char), " ,");
	i++;
    }
    return(i);
}

static int
read_slif_latch(net, line, pos)
network_t *net;
char *line, *pos;
{

    char *word, *rest;
    node_t *input, *output, *clock, *hack_po, *enable, *n1, *n2, *n3, *n4;
    array_t *array;
    int i;
    latch_t *latch;
    sis_clock_t *ck;

    pos++;
    if (*pos == 'T') {
        read_error("T flip flop not supported");
	return(0);
    }
    if (*pos != 'D') {
        read_error("unknown flip flop: %s", pos);
	return(0);
    }
    word = gettoken(line, &rest);
    if (*rest != '=') {

bad_format:
	read_error("bad format for latch");
	return(0);
    }
    output = read_slif_find_or_create_node(net, word, 1);

    do {
        pos++;
    } while (*pos != '\0' && *pos != '(');

    if (pos == '\0') {
        goto bad_format;
    }

    array = array_alloc(node_t *, 0);

    i = read_slif_nodelist(net, ++pos, array, 0);
    if (i < 2 || i > 3) {
	read_error("invalid number of inputs to D flip flop");
	array_free(array);
	return(0);	
    }

    input = array_fetch(node_t *, array, 0);
    clock = array_fetch(node_t *, array, 1);
    if (i == 3) {
        enable = array_fetch(node_t *, array, 2);
    }
    array_free(array);

    if (node_type(output) == PRIMARY_INPUT || output->F != 0) {
        read_error("latch output %s is already defined", node_name(output));
	return(0);
    }

    network_change_node_type(net, output, PRIMARY_INPUT);

    if (i == 3) {		/* enabled latch--fiddle w/ data input */
	n1 = node_literal(enable, 1);
	n2 = node_literal(input, 1);
	n3 = node_and(n1, n2);
	node_free(n1);
	node_free(n2);

	n1 = node_literal(enable, 0);
	n2 = node_literal(output, 1);
	n4 = node_and(n1, n2);
	node_free(n1);
	node_free(n2);

	input = node_or(n3, n4);	/* enable input + enable' output */
	node_free(n3);
	node_free(n4);
	
	network_add_node(net, input);
	read_change_madeup_name(net, input);
    }
    
    hack_po = network_add_fake_primary_output(net, input);
    read_change_madeup_name(net, hack_po);

    n1 = network_get_control(net, clock);
    if (n1 == NIL(node_t)) {
        n1 = network_add_fake_primary_output(net, clock);
	read_change_madeup_name(net, n1);
    }

    if (node_type(clock) == PRIMARY_INPUT) {
        ck = clock_get_by_name(net, node_long_name(clock));
	if (ck == NIL(sis_clock_t)) {
	    ck = clock_create(node_long_name(clock));
	    (void) clock_add_to_network(net, ck);
	}
    }

    network_create_latch(net, &latch, hack_po, output);
    latch_set_initial_value(latch, 0);
    latch_set_current_value(latch, 0);
    latch_set_type(latch, RISING_EDGE);

    latch_set_control(latch, n1);

    return(1);
}


#define DEFAULT_DELAY_ENTRY	"INV 1 999 1 .2 1 .2"


/*
 * 0 error
 * 1 ok
 */
int
slif_add_to_library(network)
network_t *network;
{
    FILE *fp;
    char *name, buf[128];
    node_t *node;
    double input, area;
    lsGen gen;
    int line = read_lineno;
    static char *phasename[] = {"INV", "NONINV", "UNKNOWN"};
    char stempname[20];
    int sfd;
    
    (void) network_collapse(network);
    strcpy(stempname, "/tmp/fileXXXXXX");
    sfd = mkstemp(stempname);
    fp = fdopen(sfd, "w+");
    if (fp == NIL(FILE)) {
        read_error("error opening temporary file");
	return(0);
    }

    area = (network->area_given == 0) ? 1 : network->area;
    (void) fprintf(fp, "GATE\t\"%s\"\t%g\t", network_name(network), area);

    node = node_get_fanin(network_get_po(network, 0), 0);
    write_sop(fp, node, 0, 1);

    foreach_primary_input (network, gen, node) {
        (void) fprintf(fp, "PIN\t%s ", node_long_name(node));
        input = delay_get_parameter(node, DELAY_INPUT_LOAD);
	if (input == DELAY_NOT_SET) {
	    (void) fprintf(fp, "%s\n", DEFAULT_DELAY_ENTRY);
	}
	else {
	    (void) fprintf(fp, "%s %g %g %g %g %g %g\n",
	        phasename[(int) delay_get_parameter(node, DELAY_PHASE)],
	    	input,
		delay_get_parameter(node, DELAY_MAX_INPUT_LOAD),
		delay_get_parameter(node, DELAY_BLOCK_RISE),
		delay_get_parameter(node, DELAY_DRIVE_RISE),
		delay_get_parameter(node, DELAY_BLOCK_FALL),
		delay_get_parameter(node, DELAY_DRIVE_FALL));
	}
    }
    (void) fclose(fp);
    (void) sprintf(buf, "read_library -a %s", name);
    (void) com_execute(&network, buf);
    (void) unlink(name);
    read_lineno = line;
    return(1);
}

#endif /* SIS */
