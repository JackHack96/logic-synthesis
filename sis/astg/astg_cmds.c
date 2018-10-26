
/* -------------------------------------------------------------------------- *\
   astg_cmds.c -- SIS command interface to basic ASTG commands.
\* ---------------------------------------------------------------------------*/

#ifdef SIS
#include "astg_int.h"
#include "astg_core.h"

int astg_debug_flag = 0;		/* global debugging flag	*/


extern char *astg_dup (old_astg)
char *old_astg;
{
    /*	Duplicate the network astg slot value. */
    return (char*) astg_duplicate ((astg_graph*) old_astg, NULL);
}

extern void astg_free (old_astg)
char *old_astg;
{
    /*	Free all structures for the given network astg slot. */
    astg_delete ((astg_graph*) old_astg);
}

extern astg_graph *astg_current (network)
network_t *network;		/*i network to find astg info for	*/
{
    /*	Return the ASTG corresponding to this network, or NULL if either
	the network pointer is NULL or it has no ASTG. */

    return (network==NULL) ? NULL : (astg_graph*)(network->astg);
}

extern void astg_set_current (network_p,stg,reset)
network_t **network_p;
astg_graph *stg;
astg_bool reset;
{
    /*	Whenever the astg is changed, call astg_set_current with the indirect
	network pointer, and set reset to ASTG_TRUE if the previous network
	contents should be destroyed, or ASTG_FALSE if the network does
	correspond with the astg you are setting.  This handles all special
	cases such as a nonexistent network, or if the current astg equals
	the new one, etc.  The one special case is if stg is NULL and reset
	is ASTG_FALSE, then any previous stg is not freed first.  This is
	useful if you need to do some command which will destroy the network
	and you want to preserve the astg across the call: set it to NULL,
	then set it back. */

    if (*network_p != NULL) {		/* Destroy old stuff. */
	if (reset) {
	    /* Destroy the previous network. */
	    if ((*network_p)->astg == (astg_t*)stg) {
		/* Don't destroy the astg we are trying to install. */
		(*network_p)->astg = NULL;
	    }
	    network_free (*network_p);
	    *network_p = NULL;
	}
	else if ((*network_p)->astg != NULL) {
	    /* Destroy previous astg if necessary. */
	    if ((*network_p)->astg != (astg_t*)stg) {
		if (stg != NULL) astg_free ((*network_p)->astg);
		(*network_p)->astg = NULL;
	    }
	}
    }

    if (stg != NULL) {			/* Install the new astg. */
	if (reset) {
	    astg_do_daemons (stg,NIL(astg_graph),ASTG_DAEMON_INVALID);
	}
	if (*network_p == NULL) {
	    *network_p = network_alloc();
	}
	(*network_p)->astg = (astg_t*) stg;
    }
}

extern void astg_usage (usage,cmd)
char **usage;
char  *cmd;
{
    /*	Print a set of usage strings, replacing the first %s with cmd.  A
	newline is automatically added to each line in usage; you can put
	more for special formatting.  The last element in the usage array
	must be a NULL pointer. */

    char **p;

    for (p=usage; *p != NULL; p++) {
	printf(*p,cmd);
	fputs ("\n",stdout);
    }
}

static char *flow_usage[] = {

    "usage: %s [-x] [-l <latch-type>] [-o <outfile>] [-q]",
    "    -x   bypass one-token SM check before flow",
    "    -l   use specified latch type, default=as",
    "    -o   save BLIF description in <outfile>, use '-' for stdout",
    "    -q   turn off verbose option of flow",

NULL };

static int com_astg_flow (network_p,argc, argv)
network_t **network_p;
int    argc;
char **argv;
{
    int c;
    char *outfile;
    astg_bool del_out = ASTG_TRUE;
    FILE *fout;
    astg_graph *stg, *curr_stg;
    char *latch_type = "as";
    astg_retval result = ASTG_OK;
    astg_bool full_checks = ASTG_TRUE;
    astg_bool verbose = ASTG_TRUE;
    char stempname[20];
    int sfd;

    outfile = NULL;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"xo:l:q")) != EOF) {
      switch (c) {
	case 'x': full_checks = ASTG_FALSE; break;
	case 'l': latch_type = util_optarg; break;
	case 'o': outfile = util_strsav(util_optarg);
		  del_out = ASTG_FALSE; break;
	case 'q': verbose = ASTG_FALSE; break;
	default : result = ASTG_BAD_OPTION; break;
      }
    }

    stg = curr_stg = astg_current (*network_p);

    if (stg == NULL || util_optind != argc) {
	result = ASTG_BAD_OPTION;
    }
    else if (result == ASTG_OK) {
	if (outfile == NULL) {
	    strcpy(stempname, "/tmp/fileXXXXXX");
	    sfd = mkstemp(stempname);
	    fout = fdopen(sfd, "w+");
	} else if (!strcmp(outfile,"-")) {
	    fout = stdout;
	} else {
	    fout = com_open_file (outfile,"w+",NULL,ASTG_FALSE);
	}
	if (fout != NULL) {
	    if (!astg_is_free_choice_net(stg)) {
		printf("Note: %s is not a free-choice net.\n",astg_name(stg));
		if (!stg->has_marking) {
		    printf("  You must specify an initial marking first.\n");
		    result = ASTG_ERROR;
		}
	    } else if (full_checks) {
		if (!astg_one_sm_token (stg)) result = ASTG_ERROR;
	    }
	    if (result == ASTG_OK && astg_token_flow (stg,verbose) == ASTG_OK) {
		astg_to_blif (fout,stg,latch_type);
		if (del_out) {
		    rewind (fout);
		    /* Clear astg, read new network, restore astg. */
		    astg_set_current(network_p,NIL(astg_graph),ASTG_FALSE);
		    network_free (*network_p);  *network_p = NULL;
		    error_init();
		    read_register_filename ("astg_flow");
		    if (read_blif (fout,network_p) == 0) {
			fprintf(siserr,"%s",error_string());
			result = ASTG_ERROR;
		    }
		    astg_set_current(network_p,curr_stg,ASTG_FALSE);
		}
	    }
	    if (fout != stdout) fclose (fout);
	    if (del_out) unlink (outfile);
	    if (outfile) FREE (outfile);
	}
    }

    if (result == ASTG_BAD_OPTION) {
	astg_usage (flow_usage,argv[0]);
    }

    return (result != ASTG_OK);
}

static char *read_usage[] = {

    "usage: %s [<stg_file>]",
    "    Reads from stdin if no file name is specified.",

NULL };

static int com_astg_read (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    int c;
    char *infile;
    char alt_infile[80];
    astg_bool from_file = ASTG_FALSE;
    FILE *fin;
    astg_graph *stg;
    astg_retval result = ASTG_OK;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"")) != EOF) {
	switch (c) {
	    default : result = ASTG_BAD_OPTION; break;
	}
    }

    if (util_optind < argc) {
	infile = argv[util_optind++];
	strcpy (alt_infile,infile);
	fin = com_open_file (alt_infile,"r",NULL,ASTG_FALSE);
	if (fin == NULL) {
	    printf("Failed to open input file '%s'\n",infile);
	    result = ASTG_ERROR;
	} else {
	    from_file = ASTG_TRUE;
	}
    }
    else {
	strcpy (alt_infile,"<stdin>");
	fin = stdin;
	from_file = ASTG_FALSE;
    }

    if (util_optind != argc) {
	result = ASTG_BAD_OPTION;
    }
    else if (result == ASTG_OK) {
	stg = astg_read (fin,alt_infile);
	if (from_file) fclose (fin);
	if (stg == NULL) result = ASTG_ERROR;
    }

    if (result == ASTG_BAD_OPTION) {
	astg_usage (read_usage,argv[0]);
    }
    else if (result == ASTG_OK) {
	astg_set_current (network_p,stg,ASTG_TRUE);
    }
    return (result != ASTG_OK);
}

static char *print_usage[] = {

    "usage: %s [-p] [<outfile>]",
    "    -p  print all places even with 1 in edge and out edge",

NULL };

static int com_astg_write (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    astg_retval result = ASTG_OK;
    astg_bool hide_places = ASTG_TRUE;
    char *outfile = NULL;
    FILE *fout;
    int c;
    astg_graph *curr_stg;


    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"p")) != EOF) {
	switch (c) {
	    case 'p': hide_places = ASTG_FALSE; break;
	    default : result = ASTG_BAD_OPTION; break;
	}
    }
    if (util_optind < argc) {
	outfile = argv[util_optind++];
    }
    curr_stg = astg_current (*network_p);

    if (result == ASTG_BAD_OPTION || util_optind != argc || curr_stg == NULL) {
	astg_usage (print_usage,argv[0]);
	return 1;
    }

    if (outfile != NULL) {
	fout = com_open_file (outfile,"w",NULL,ASTG_TRUE);
	if (fout == NULL) return 1;
        astg_write (curr_stg,hide_places,fout);
	fclose (fout);
    }
    else {
        astg_write (curr_stg,hide_places,stdout);
    }
    return (result != ASTG_OK);
}


static char *cycle_usage[] = {

    "usage: %s [-la] [-t <trans_name>] [<cycle index>]",
    "    select simple cycles in the STG (default = all)",
    "    -l  select longest delay",
    "    -a  append to existing set",
    "    -t  optionally only through specified trans",
    "    -c  count total number of simple cycles",

NULL };

static int astg_count_cycles (stg,data)
astg_graph *stg;
void *data;
{
    /*	Return 1 to count number of simple cycles in the net. */
    /* ARGSUSED */
    return 1;
}

static int com_astg_cycle (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    int c;
    astg_retval result = ASTG_OK;
    astg_graph *stg;
    io_t source;
    char *tname = NULL;
    astg_trans *thru = NULL;
    astg_bool longest = ASTG_FALSE;
    astg_bool append = ASTG_FALSE;
    astg_bool count_em = ASTG_FALSE;
    int n_cycle, cycle_n = 0;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"t:lac")) != EOF) {
        switch (c) {
	    case 'a': append = ASTG_TRUE; break;
	    case 'l': longest = ASTG_TRUE; break;
	    case 't': tname = util_optarg; break;
	    case 'c': count_em = ASTG_TRUE; break;
	    default : result = ASTG_BAD_OPTION; break;
	}
    }

    if ((stg=astg_current (*network_p)) == NULL) result = ASTG_BAD_OPTION;

    if (result == ASTG_OK && tname != NULL) {
	io_open (&source,NIL(FILE),tname);
	io_token (&source);
	thru = find_trans_by_name (&source,stg,ASTG_FALSE);
	io_close (&source);
	if (thru == NULL) result = ASTG_BAD_OPTION;
    }

    if (util_optind != argc) {
	cycle_n = atoi (argv[util_optind++]);
    }

    if (result == ASTG_BAD_OPTION) {
	astg_usage (cycle_usage,argv[0]);
    } else if (count_em) {
	n_cycle = astg_simple_cycles (stg,NULL,astg_count_cycles,NULL,ASTG_ALL);
	printf("Total number of simple cycles: %d\n",n_cycle);
    } else {
	astg_select_cycle (stg,thru,cycle_n,longest,append);
    }
    return (result != ASTG_OK);
}


static char *lockgraph_usage[] = {

    "usage: %s [-l]",
    "    print lock graph for an STG",
    "    -l	try to form one lock class first",

NULL };

static int print_stg_comp (varray,n,extra)
array_t  *varray;
int n;
void *extra;		/*  ARGSUSED */
{
    int i;
    astg_vertex *v;

    printf("  %d)",n);
    for (i=0; i < array_n(varray); i++) {
        v = array_fetch (astg_vertex *,varray,i);
        printf(" %s",astg_v_name(v));
    }
    fputs("\n",stdout);
}

static int com_astg_lockgraph (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    int c;
    astg_retval result = ASTG_OK;
    int n_comp;
    astg_graph *stg;
    astg_graph *curr_stg;
    astg_bool do_lock = ASTG_FALSE;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"l")) != EOF) {
        switch (c) {
	  case 'l': do_lock = ASTG_TRUE; break;
	  default : result = ASTG_BAD_OPTION; break;
	}
    }

    curr_stg = astg_current (*network_p);

    if (result == ASTG_BAD_OPTION || curr_stg == NULL) {
	result = ASTG_BAD_OPTION;
	astg_usage (lockgraph_usage,argv[0]);
    }
    else {
	stg = curr_stg;
	if (! astg_state_coding (stg,do_lock)) {
		return 1;
	}
	n_comp = astg_lock_graph_components (stg,ASTG_NO_CALLBACK,ASTG_NO_DATA);
	dbg(1,msg("lock graph has %d componen%s\n",n_comp,n_comp==1?"t":"ts"));
	if (n_comp != 1) {
	    printf("\nwarning: STG may have complementary sets:\n");
	    astg_lock_graph_components (stg,print_stg_comp,ASTG_NO_DATA);
	    printf("This could cause duplicate state codings.\n");
	}
	dbg(1,msg("\n"));
	if (do_lock) astg_set_current (network_p,stg,ASTG_TRUE);
    }
    return (result != ASTG_OK);
}

static char *persist_usage[] = {

    "usage: %s [-p]",
    "    -p    just print nonpersistent transitions (don't modify STG)",
    "    Otherwise, add persistency constraints to the STG.",

NULL };

static int com_astg_persist (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    int c;
    astg_retval result = ASTG_OK;
    astg_graph *stg;
    astg_bool modify = ASTG_TRUE;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"p")) != EOF) {
        switch (c) {
	    case 'p': modify = ASTG_FALSE; break;
            default : result = ASTG_BAD_OPTION; break;
        }
    }

    stg = astg_current (*network_p);

    if (result == ASTG_BAD_OPTION || stg == NULL) {
	result = ASTG_BAD_OPTION;
        astg_usage (persist_usage,argv[0]);
    }
    else {
	make_persistent (stg,modify);
	if (!modify) {
	    astg_sel_show (stg);
	} else {
	    astg_set_current (network_p,stg,ASTG_TRUE);
	}
    }

    return (result != ASTG_OK);
}

static char *red_usage[] = {

    "usage: %s [-p]",
    "    -p  print redundant edges instead of deleting them.",

NULL };

static int com_astg_irred (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    int c;
    astg_retval result = ASTG_OK;
    astg_bool modify = ASTG_TRUE;
    astg_graph *curr_stg;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"p")) != EOF) {
        switch (c) {
	    case 'p': modify = ASTG_FALSE; break;
            default : result = ASTG_BAD_OPTION; break;
        }
    }

    curr_stg = astg_current (*network_p);

    if (result == ASTG_BAD_OPTION || curr_stg == NULL) {
	result = ASTG_BAD_OPTION;
	astg_usage (red_usage,argv[0]);
    }
    else {
	astg_irred (curr_stg,modify);
	if (modify) astg_set_current (network_p,curr_stg,ASTG_TRUE);
    }
    return (result != ASTG_OK);
}


static char *current_usage[] = {

    "usage: %s [-d #]",
    "    -d  set debug output (0=no debug output)",
    "    display information about the current stg",

NULL };

static int com_astg_current (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    int c;
    astg_retval result = ASTG_OK;
    int n_comp1, n_comp2;
    astg_graph *curr_stg;
    lsGen cgen;
    lsGeneric data;
    long change_count;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"d:")) != EOF) {
	switch (c) {
	    case 'd': astg_debug_flag = atoi(util_optarg); break;
	    default : result = ASTG_BAD_OPTION; break;
	}
    }

    curr_stg = astg_current (*network_p);

    if (argc > util_optind || result == ASTG_BAD_OPTION) {
	result = ASTG_BAD_OPTION;
	astg_usage (current_usage,argv[0]);
    }
    else if (curr_stg == NULL) {
	fprintf(sisout,"No current ASTG.\n");
    }
    else {
	change_count = astg_change_count (curr_stg);
	printf("%s\n", astg_name(curr_stg));
	/* Print all comments for this graph. */
	cgen = lsStart (curr_stg->comments);
	while (lsNext(cgen,&data,LS_NH) == LS_OK) {
	    printf("  %s\n",(char *) data);
	}
	lsFinish (cgen);
	printf("\tFile: %s", curr_stg->filename);
	if (curr_stg->file_count != change_count) {
	    fputs(" (modified)",stdout);
	}
	fputs("\n",stdout);
	printf("\tPure: %c  Place-simple: %c\n",
		astg_is_pure(curr_stg)?'Y':'N', astg_is_place_simple(curr_stg)?'Y':'N');
	n_comp1 = astg_connected_comp (curr_stg,ASTG_NO_CALLBACK,ASTG_NO_DATA,ASTG_ALL);
	n_comp2 = astg_strong_comp    (curr_stg,ASTG_NO_CALLBACK,ASTG_NO_DATA,ASTG_ALL);
	printf("\tConnected: %c  Strongly Connected: %c\n",
		(n_comp1==1?'Y':'N'), (n_comp2==1?'Y':'N'));
	printf("\tFree Choice: %c  Marked Graph: %c  State Machine: %c\n",
		astg_is_free_choice_net(curr_stg)?'Y':'N',
		astg_is_marked_graph(curr_stg)?'Y':'N',
		astg_is_state_machine(curr_stg)?'Y':'N');
	if (curr_stg->sm_comp != NULL) {
	    printf("\tSM components: %d\n",array_n(curr_stg->sm_comp));
	}
	if (curr_stg->mg_comp != NULL) {
	    printf("\tMG components: %d\n",array_n(curr_stg->mg_comp));
	}
    }

    return (result != ASTG_OK);
}

static char *marking_usage[] = {

    "usage: %s [-s] [new_marking]",
    "    print or set initial marking for the current STG",
    "    -s  specify new marking using signal values",
    "        e.g. a 1 b 0",
    "    otherwise new marking in format: {place1 place2 ...}",
    "    where place is either place name or <t1,t2> transition pair.",

NULL };

static int com_astg_marking (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    astg_retval result = ASTG_OK;
    astg_graph *stg;
    astg_place *p;
    io_t source;
    int c;
    astg_graph *curr_stg;
    astg_generator pgen;
    astg_retval status;
    int value;
    astg_scode state_code;
    st_table *sig_values;
    char *sig_name;
    astg_bool by_state_code = ASTG_FALSE;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"s")) != EOF) {
	switch (c) {
	    case 's': by_state_code = ASTG_TRUE; break;
	    default : result = ASTG_BAD_OPTION; break;
	}
    }

    stg = curr_stg = astg_current (*network_p);

    if (stg == NULL || result != ASTG_OK) {
	result = ASTG_BAD_OPTION;
    }
    else if (util_optind == argc) {
	if (stg->has_marking) {
	    if (by_state_code) {
		status = astg_initial_state (stg,&state_code);
		if (status == ASTG_OK || status == ASTG_NOT_USC) {
		    printf("%X\n",state_code);
		}
	    }
	    else {
		astg_write_marking (stg,stdout);
	    }
	}
	else {
	    printf("%s does not have an initial marking set.\n",
		astg_name(curr_stg));
	}
    }
    else if (util_optind == argc-1 || by_state_code) {
	if (by_state_code) {
	    sig_values = st_init_table (strcmp,st_strhash);
	    while (util_optind < argc) {
		sig_name = argv[util_optind++];
		if (util_optind < argc && sscanf(argv[util_optind++],"%d",&value) == 1) {
		    st_insert (sig_values,sig_name,(char *)value);
		}
		else {
		    printf("Bad value for signal '%s' ignored.\n",
			sig_name);
		}
	    }
	    astg_set_marking_by_name (stg,sig_values);
	    st_free_table (sig_values);
	}
	else {
	    astg_foreach_place (stg,pgen,p) {
		p->type.place.initial_token = ASTG_FALSE;
	    }
	    io_open (&source,NIL(FILE),argv[util_optind++]);
	    astg_read_marking (&source,stg);
	    if (io_status (&source) != 0) result = ASTG_ERROR;
	}
	astg_set_current (network_p,stg,ASTG_TRUE);
    }
    else {
	result = ASTG_BAD_OPTION;
    }

    if (result == ASTG_BAD_OPTION) astg_usage (marking_usage,argv[0]);
    return (result != ASTG_OK);
}

static int astg_print_component_pieces (vertices,n,fdata)
array_t *vertices;
int n;
void *fdata;	/*  ARGSUSED */
{
    /*	Callback for astg_strong_comp, to print parts of a component
	to the user. */

    astg_vertex *v;
    int i;

    printf("\n %d)",n);
    for (i=array_n(vertices); i--; ) {
	v = array_fetch (astg_vertex *, vertices, i);
	if (astg_v_type(v) == ASTG_TRANS) {
	    printf(" %s",astg_trans_name(v));
	} else {
	    printf(" %s",astg_place_name(v));
	}
    }
    printf("\n");
    return 1;
}

static void select_comp (stg,comp,comp_name,comp_n)
astg_graph *stg;
array_t *comp;
char *comp_name;
int comp_n;
{
    astg_vertex *v;
    int n, n_scc;
    astg_generator gen;
    static char set_name[90];

    sprintf(set_name,"%s component %d",comp_name,comp_n);
    astg_sel_new (stg,set_name,ASTG_FALSE);
    astg_foreach_vertex (stg,gen,v) v->subset = ASTG_FALSE;

    for (n=array_n(comp); n--; ) {
	v = array_fetch (astg_vertex *,comp,n);
	v->subset = ASTG_TRUE;
	astg_sel_vertex (v,ASTG_TRUE);
    }

    astg_sel_show (stg);
    n_scc = astg_strong_comp (stg,ASTG_NO_CALLBACK,ASTG_NO_DATA,ASTG_SUBSET);
    if (n_scc != 1) {
	printf(
	"warning: this %s component has %d strongly connected components.\n",
	comp_name,n_scc);
	astg_strong_comp (stg,astg_print_component_pieces,
				ASTG_NO_DATA,ASTG_SUBSET);
    }
}


static char *mgc_usage[] = {

    "usage: %s [<n1> ...]",
    "    find marked graph (MG) components.  If no arguments are given",
    "    then any vertices which are not covered by the MG compoents",
    "    are printed, otherwise the components with the given numbers are",
    "    printed.",

NULL };

static int com_astg_mgc (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    astg_retval result = ASTG_OK;
    astg_graph *stg;
    int c, n, status;
    array_t *comp;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"")) != EOF) {
	switch (c) {
	    default : result = ASTG_BAD_OPTION; break;
	}
    }

    stg = astg_current (*network_p);

    if (result == ASTG_BAD_OPTION || stg == NULL) {
	astg_usage (mgc_usage,argv[0]);
	return 1;
    }

    status = get_mg_comp (stg,ASTG_TRUE);
    dbg(1,msg("%d marked graph (MG) components.\n", array_n(stg->mg_comp)));

    if (util_optind == argc) {
	if (status == ASTG_NOT_COVER) astg_sel_show (stg);
    }
    else {
	for (; util_optind < argc; util_optind++) {
	    n = atoi (argv[util_optind]);
	    if (n > 0 && n <= array_n(stg->mg_comp)) {
		comp = array_fetch (array_t *,stg->mg_comp,n-1);
		select_comp (stg,comp,"MG",n);
	    }
	    else {
		printf("There are only %d MG components.\n",
			array_n(stg->mg_comp));
	    }
	}
    }

    return (result != ASTG_OK);
}

static char *smc_usage[] = {

    "usage: %s [<n1> ...]",
    "    find state machine (SM) components.  If no arguments are given",
    "    then any vertices which are not covered by the SM compoents",
    "    are printed, otherwise the components with the given numbers are",
    "    printed.",

NULL };

static int com_astg_smc (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    astg_retval result = ASTG_OK;
    astg_graph *stg;
    int c, n, status;
    array_t *comp;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"")) != EOF) {
	switch (c) {
	    default : result = ASTG_BAD_OPTION; break;
	}
    }

    stg = astg_current (*network_p);

    if (result == ASTG_BAD_OPTION || stg == NULL) {
	astg_usage (smc_usage,argv[0]);
	return 1;
    }

    status = get_sm_comp (stg,ASTG_TRUE);
    dbg(1,msg("%d state machine (SM) components.\n", array_n(stg->sm_comp)));

    if (util_optind == argc) {
	if (status == ASTG_NOT_COVER) astg_sel_show (stg);
    }
    else {
	for (; util_optind < argc; util_optind++) {
	    n = atoi (argv[util_optind]);
	    if (n > 0 && n <= array_n(stg->sm_comp)) {
		comp = array_fetch (array_t *,stg->sm_comp,n-1);
		select_comp (stg,comp,"SM",n);
	    }
	    else {
		printf("There are only %d SM components.\n",
			array_n(stg->sm_comp));
	    }
	}
    }

    return (result != ASTG_OK);
}


static char *contract_usage[] = {

    "usage: %s [-f] <output_signal>",
    "    -f  keep contracted nets Free Choice",
    "    generate the contracted net for the specified noninput signal",

NULL };

static int com_astg_contract (network_p,argc,argv)
network_t **network_p;
int argc;
char **argv;
{
    astg_retval result = ASTG_OK;
    astg_signal *sig_p;
    astg_graph *cstg = NIL(astg_graph);
    astg_bool keep_fc = ASTG_FALSE;
    astg_graph *curr_stg;
    int c;

    util_getopt_reset ();
    while ((c=util_getopt(argc,argv,"f")) != EOF) {
      switch (c) {
      case 'f': 
	keep_fc = ASTG_TRUE;
	break;
      default: 
	result = ASTG_BAD_OPTION; 
	break;
      }
    }

    curr_stg = astg_current (*network_p);

    if (result == ASTG_BAD_OPTION || util_optind != argc - 1 || curr_stg == NULL) {
	result = ASTG_BAD_OPTION;
	astg_usage (contract_usage,argv[0]);
    }
    else {
      if (! astg_state_coding (curr_stg,ASTG_TRUE)) {
		return 1;
	  }
      sig_p = astg_find_named_signal (curr_stg,argv[util_optind]);
      if (sig_p == NULL) {
	printf("STG %s does not have signal '%s'.\n",
	       astg_name(curr_stg), argv[util_optind]);
      } else if (sig_p->sig_type == ASTG_INPUT_SIG) {
	printf("'%s' is an input signal.\n",argv[util_optind]);
      } else {
	cstg = astg_contract (curr_stg,sig_p,keep_fc);
	astg_irred (cstg,ASTG_TRUE);
	/* Set current stg to the contracted net. */
	astg_set_current (network_p,cstg,ASTG_TRUE);
      }
    }

    return (result != ASTG_OK);
}

typedef struct astg_cmd_rec {
    char	 *cmd_name;
    int		(*cmd)();
    astg_bool	  modifies;
}
astg_cmd_rec;


static astg_cmd_rec basic_commands[] = {
    { "read_astg",	com_astg_read,		ASTG_TRUE  },
    { "_astg_flow",	com_astg_flow,		ASTG_TRUE  },
    { "astg_current",	com_astg_current,	ASTG_FALSE },
    { "astg_persist",	com_astg_persist,	ASTG_TRUE  },
    { "astg_lockgraph",	com_astg_lockgraph,	ASTG_TRUE  },
    { "_astg_cycle",	com_astg_cycle,		ASTG_FALSE },
    { "write_astg",	com_astg_write,		ASTG_FALSE },
    { "astg_marking",	com_astg_marking,	ASTG_TRUE  },
    { "_astg_irred",	com_astg_irred,		ASTG_TRUE  },
    { "astg_contract",	com_astg_contract,	ASTG_TRUE  },
    { "_astg_smc",	com_astg_smc,		ASTG_FALSE },
    { "_astg_mgc",	com_astg_mgc,		ASTG_FALSE }
};

void astg_basic_cmds (do_init)
astg_bool do_init;	/*i ASTG_TRUE=do initialize, ASTG_FALSE=do end.	*/
{
    astg_cmd_rec *p;
    int i;

    if (do_init) {
	i = sizeof(basic_commands) / sizeof(basic_commands[0]);
	for (p=basic_commands; i--; p++) {
	    com_add_command (p->cmd_name, p->cmd, p->modifies);
	}
    }
    else {
	astg_discard_daemons ();
    }
}
#endif /* SIS */
