
/* -------------------------------------------------------------------------- *\
   astg_read.c -- read STG from text description.
\* ---------------------------------------------------------------------------*/

#ifdef SIS
#include "astg_int.h"
#include "astg_core.h"


char *astg_make_name (sig_name,trans_type,copy_n)
char *sig_name;
astg_trans_enum trans_type;
int copy_n;
{
    char tname[ASTG_NAME_LEN], copy_name[ASTG_NAME_LEN];

    strcpy (tname,sig_name);
    if (trans_type == ASTG_POS_X)
	strcat (tname,"+");
    else if (trans_type == ASTG_NEG_X)
	strcat (tname,"-");
    else if (trans_type == ASTG_TOGGLE_X)
	strcat (tname,"~");
    else if (trans_type == ASTG_HATCH_X)
	strcat (tname,"*");
    else if (trans_type != ASTG_DUMMY_X)
	fail("bad trans type");

    if (copy_n > 0) {
	sprintf(copy_name,"/%d",copy_n);
	strcat (tname, copy_name);
    }
    return util_strsav (tname);
}

static int parse_name (source,signame,type,copy_n)
io_t *source;		/*u buffer contains vertex name		*/
char *signame;		/*o signal name				*/
astg_trans_enum *type;	/*o transition type for ASTG_TRANS	*/
int *copy_n;		/*o copy number for ASTG_TRANS		*/
{
    char *t, *s;

    s = source->buffer;
    signame[0] = '\0';
    if ((t = strrchr(s,'+')) != NULL) {
	*type = ASTG_POS_X;
	strncat (signame,s, (t-s));
    }
    else if ((t = strrchr(s,'-')) != NULL) {
	*type = ASTG_NEG_X;
	strncat (signame,s, (t-s));
    }
    else if ((t = strrchr(s,'~')) != NULL) {
	*type = ASTG_TOGGLE_X;
	strncat (signame,s, (t-s));
    }
    else if ((t = strrchr(s,'*')) != NULL) {
	*type = ASTG_HATCH_X;
	strncat (signame,s, (t-s));
    }
    else {
	*type = ASTG_DUMMY_X;
	if ((t = strchr(s,'/')) != NULL) {
	    strncat (signame,s, (t-s));
	} else {
	    strcpy (signame,s);
	}
    }

    *copy_n = 0;
    if ((t=strrchr(s,'/')) != NULL && sscanf(t+1,"%d",copy_n) != 1) {
	return io_error (source,"unrecognizable copy number");
    }
    return 0;
}

astg_trans *find_trans_by_name (source,stg,create)
io_t *source;
astg_graph *stg;
astg_bool create;
{
    astg_trans_enum trans_type;
    int copy_n;
    char name[ASTG_NAME_LEN];
    astg_trans *t = NULL;

    if (parse_name(source,name,&trans_type,&copy_n) == 0) {
	t = astg_find_trans (stg,name,trans_type,copy_n,create);
	if (t == NULL) {
	    if (create)
		io_error (source,"no such signal");
	    else
		io_error(source,"no such transition");
	}
    }
    return t;
}

/* --------------------- Simple input parsing utilities --------------------- */

void io_open (source,stream,s)
io_t *source;
FILE *stream;
char *s;
{
    if (s != NULL) {
	source->from_file = ASTG_FALSE;
	source->s = source->p = s;
	source->inbuf = NULL;
    } else {
	source->from_file = ASTG_TRUE;
	source->stream = (stream == NULL) ? stdin : stream;
	source->in_len = 180;
	source->inbuf = ALLOC (char,source->in_len);
	source->p = source->s = source->inbuf;
	*source->s = '\0';
    }
    source->line_n = 0;
    source->buflen = 180;
    source->buffer = ALLOC (char,source->buflen);
    source->save_one = '\0';
    source->errflag = 0;
}

int io_close (source)
io_t *source;
{
    int status = source->errflag;
    FREE (source->inbuf);
    FREE (source->buffer);
    return status;
}

static int io_dump_string (buffer, offset)
char *buffer;
int offset;
{
    char *p = buffer;
    char buff[5], *s;
    int c;
    int rel_posn = 0, new_offset = 0;

    while ((c=(*(p++))) != '\0') {
        if (isprint(c)) {
            buff[0] = c; buff[1] = '\0';
            s = buff;
        } else if (c == '\n') {
            s = "";
        } else if (c == '\t') {
            s = "\\t";
        } else if (c == EOF) {
            s = "<EOF>";
        } else {
            sprintf(buff,"\\%3.3o",c);
            s = buff;
        }

        rel_posn += strlen(s);
        if (--offset == 0) new_offset = rel_posn;
        fputs(s,stdout);
    }

    fputs("\n",stdout);
    return new_offset;
}

void io_msg (source,warning,message)
io_t *source;
astg_bool warning;
char *message;
{
    int offset;
    int n_spaces, n_dashes;

    fputs(warning?"\nWarning: ":"\nError: ",stdout);
    puts (message);

    printf("\n%4d: ",source->line_n);
    offset = (source->p - source->s);
    if (source->save_one != '\0') offset--;
    n_spaces = 6 + io_dump_string (source->s,offset);

    n_dashes = strlen(source->buffer);
    n_spaces -= n_dashes;

    while (n_spaces--) fputc(' ',stdout);
    while (n_dashes--) fputc('^',stdout);
    fputs("\n",stdout);
}

void io_warn (source,message)
io_t *source;
char *message;
{
    if (source->errflag == 0) {
	io_msg (source,ASTG_TRUE,message);
    }
}

int io_error (source,message)
io_t *source;
char *message;
{
    if (source->errflag == 0) {
	source->errflag = -1;
	io_msg (source,ASTG_FALSE,message);
	source->save_one = EOF;
    }
    return -1;
}


int io_get2 (source)
io_t *source;
{
    int c = source->save_one;

    if (c != '\0') {
	if (c != EOF) source->save_one = '\0';
    }
    else if ((c=(*source->p)) == '\0') {
	if (source->from_file) {
	    if (fgets (source->inbuf,source->in_len,source->stream) == NULL) {
		c = source->save_one = EOF;
	    }
	    else {
		source->line_n++;
		source->p = source->inbuf;
		c = *(source->p++);
	    }
	}
	else {
	    c = EOF;
	}
    }
    else {
	source->p++;
    }
    dbg(3,msg("io_get %c (%d)\n",c,c));
    return c;
}


int io_get (source)
io_t *source;
{
    int c = io_get2 (source);

    if (c == '#') {
	do {
	    c = io_get2 (source);
	} while (c != EOF && c != '\n');
	io_unget (c,source);
	c = ' ';
    }
    return c;
}


static int io_getc (source)
io_t *source;
{
    int c;
    do {
	c = io_get (source);
	if (c == EOF) io_error(source,"unexpected end-of-file");
    } while (c != EOF && c != '\n' && isspace(c));
    return c;
}

static void io_unget (c,source)
int c;
io_t *source;
{
    if (source->save_one != EOF) {
	source->save_one = c;
    }
}

static int io_peek (source)
io_t *source;
{
    int c = io_getc (source);
    io_unget (c,source);
    return c;
}

int io_token (source)
io_t *source;
{
    int   c, start_char, end_char, depth;
    char *p;
    static char separators[] = {",>)}?"};
    int len = source->buflen;

    do {
	c = io_get(source);
	if (c == '\n') break;
    } while (isspace(c));

    p = source->buffer;

    if (strchr("{}<,>",c)) {
	*(p++) = c;
    }
    else if (isprint(c)) {
	depth = 1; start_char = c;
	if (c == '(')
	    end_char = ')';
	else if (c == '"')
	    end_char = '"';
	else
	    end_char = EOF;

	while (c != EOF) {
	    if (len == 1) {
		printf("Warning: token truncated.\n");
	    } else if (len > 0) {
		*(p++) = c;
	    }
	    len--;
	    if (depth == 0) break;
	    c = io_get(source);
	    if (end_char == EOF) {
		if (isspace(c) || strchr(separators,c) != NULL) {
		    io_unget (c,source);
		    break;
		}
	    } else if (c == end_char) {
		depth--;
	    } else if (c == start_char) {
		depth++;
	    }
	} /* end while */
    }
    else {
	*(p++) = c;
    }

    *p = '\0';
    c = *source->buffer;
    return c;
}

static astg_bool io_maybe (source,target_char)
io_t *source;
int target_char;
{
    int next_c = io_getc(source);
    if (next_c != target_char) io_unget (next_c,source);
    return (next_c == target_char);
}

static int io_mustbe (source,token)
io_t *source;
char *token;
{
    char msgbuf[80];

    io_token (source);
    if (strcmp(source->buffer,token)) {
	strcpy(msgbuf,"expecting '");
	if (!strcmp("\n",token)) {
	    strcat(msgbuf,"end of line");
	}
	else {
	    strcat(msgbuf,token);
	}
	strcat(msgbuf,"' here");
	io_error (source,msgbuf);
    }
    return io_status(source);
}

/* ------------------------- Parser for STG --------------------------------- */

static int is_place_name (stg,vname)
astg_graph *stg;
char *vname;
{
    int is_place, c;
    int n;
    char *p;

    if ((p=strchr(vname,'/')) != NULL) {
	is_place = ASTG_FALSE;
    }
    else if (astg_find_named_signal (stg,vname) != NULL) {
	is_place = ASTG_FALSE;
    }
    else {
	p = strrchr (vname,'/');
	n = ((p == NULL) ? strlen(vname) : (p-vname)) - 1;
	c = vname[n];
	is_place = (strchr("+-*~",c) == NULL);
    }
    return is_place;
}

static int read_point_list (source,e)
io_t *source;
astg_edge *e;
{
    float xp;

    if (io_maybe(source,'(')) {
	if (e->spline_points == NULL) {
	    e->spline_points = array_alloc (float,0);
	}
	do {
	    io_token (source);
	    if (sscanf(source->buffer,"%f",&xp) == 1) {
		array_insert_last (float,e->spline_points,xp);
	    } else {
		io_error (source,"bad coordinate");
	    }
	} while (io_maybe(source,','));

	io_mustbe (source,")");
    }
    return io_status(source);
}

static void read_edge_guard (source,se)
io_t *source;
astg_edge *se;
{
    if (io_maybe(source,'?')) {
	io_token (source);
	if (!astg_set_guard (se,source->buffer,source)) {
	    io_error (source,"unrecognizable guard");
	}
    }
}

static int read_place_fanout (source,stg,startp)
io_t *source;
astg_graph *stg;
astg_place *startp;
{
    astg_trans *t;

    dbg(2,msg("read place fanout\n"));
    io_token (source);

    if ((t=find_trans_by_name(source,stg,ASTG_TRUE)) != NULL) {
	astg_edge *e = astg_find_edge (startp,t,ASTG_FALSE);
	if (e != NULL) {
	    io_warn (source,"Repeated edge to this transition is ignored.");
	} else {
	    e = astg_new_edge (startp,t);
	    read_edge_guard (source,e);
	    read_point_list (source,e);
	}
    }
    else {
	io_error (source,"unrecognized signal transition name");
    }
    return io_status(source);
}

static int read_trans_fanout (source,stg,startt)
io_t *source;
astg_graph *stg;
astg_trans *startt;
{
    int c = io_token(source);
    int status = 0;
    astg_place *p;
    astg_trans *t;
    astg_edge *e;

    dbg(2,msg("read trans fanout\n"));

    if (isalpha(c)) {
	if (is_place_name(stg,source->buffer)) {
	    /* This is an explicit place. */
	    p = astg_find_place (stg,source->buffer,ASTG_TRUE);
	    e = astg_find_edge (startt,p,ASTG_FALSE);
	    if (e != NULL) {
		io_warn (source,"Repeated edge to this place is ignored.");
	    }
	    else {
		e = astg_new_edge (startt,p);
		read_point_list (source,e);
	    }
	}
	else {
	    /* This has an implied place; insert a place automatically. */
	    t = find_trans_by_name (source,stg,ASTG_TRUE);
	    if (t != NULL) {
		p = astg_find_place (stg,NULL,ASTG_TRUE);
		astg_new_edge (startt,p);
		astg_new_edge (p,t);
	    }
	    /* How to do edges for this? */
	}
    }
    else {
	status = io_error (source,"bad fanout place");
    }
    return status;
}

void astg_read_marking (source,stg)
io_t *source;
astg_graph *stg;
{
    astg_trans *t1, *t2, *t;
    astg_generator tgen, pgen;
    astg_place *place, *meant_p;

    stg->has_marking = ASTG_TRUE;
    io_mustbe (source,"{");

    while (io_status(source) == 0 && !io_maybe(source,'}')) {
	if (io_maybe(source,'<')) {
	    io_token (source);
	    t1 = find_trans_by_name (source,stg,ASTG_FALSE);
	    io_mustbe (source,",");
	    io_token (source);
	    t2 = find_trans_by_name (source,stg,ASTG_FALSE);
	    io_mustbe (source,">");
	    if (t1 != NULL && t2 != NULL) {
		meant_p = NULL;
		astg_foreach_output_place (t1,pgen,place) {
		    astg_foreach_output_trans (place,tgen,t) {
			if (t == t2) meant_p = place;
		    }
		}
		if (meant_p == NULL) {
		    io_error (source,"couldn't find this edge");
		} else {
		    meant_p->type.place.initial_token = ASTG_TRUE;
		}
	    }
	}
	else {
	    io_token (source);
	    meant_p = astg_find_place (stg,source->buffer,ASTG_FALSE);
	    if (meant_p == NULL) {
		io_error (source,"no place with this name");
	    } else {
		meant_p->type.place.initial_token = ASTG_TRUE;
	    }
	}
    }

    if (io_status(source) == 0) stg->has_marking = ASTG_TRUE;
    stg->change_count++;
}

static void read_delay (source,delay)
io_t *source;
float *delay;
{
    if (isdigit(io_peek(source))) {
	io_token (source);
	if (sscanf(source->buffer,"%f",delay) != 1) {
	    io_error (source,"invalid transition delay value");
	}
    }
}

static int read_posn (source,xp,yp)
io_t *source;
float *xp, *yp;
{
    if (io_maybe(source,'(')) {
	io_token (source);
        if (sscanf(source->buffer," %f",xp) != 1) {
	    io_error (source,"bad x coordinate");
	}
        io_mustbe(source,",");
        io_token (source);
        if (sscanf(source->buffer," %f",yp) != 1) {
	    io_error (source,"bad y coordinate");
	}
        io_mustbe(source,")");
    }
    return io_status(source);
}

static void read_vertex (source,stg)
io_t *source;
astg_graph *stg;
{
    astg_place *p;
    astg_trans *t;

    dbg(2,msg("enter read_vertex\n"));
    io_token (source);

    if (is_place_name (stg,source->buffer)) {
	if ((p=astg_find_place (stg,source->buffer,ASTG_TRUE)) != NULL) {
	    p->type.place.user_named = ASTG_TRUE;
	    read_posn (source,&p->x,&p->y);
	    while (io_status(source) == 0 && !io_maybe(source,'\n')) {
		if (read_place_fanout (source,stg,p)) break;
	    }
	}
    }
    else {
	if ((t=find_trans_by_name (source,stg,ASTG_TRUE)) != NULL) {
	    read_delay (source,&t->type.trans.delay);
	    read_posn (source,&t->x,&t->y);
	    while (io_status(source) == 0 && !io_maybe(source,'\n')) {
		if (read_trans_fanout (source,stg,t)) break;
	    }
	}
    }

    dbg(2,msg("end read_vertex\n"));
}

static void read_graph_check (source,stg)
io_t *source;
astg_graph *stg;
{
    astg_generator gen, gen2;
    astg_signal *s;
    astg_place *p, *fan_place;
    astg_trans *t, *new_t;

    if (io_status(source) != 0) return;

    /* --- check that all signals were used? */

    /* Expand hatch transitions into toggles equivalent. */

    astg_foreach_trans (stg,gen,t) {
	if (astg_trans_type(t) == ASTG_HATCH_X) {
	    s = astg_new_sig (stg,ASTG_DUMMY_SIG);
	    p = astg_new_place (stg,NULL,NULL);

	    new_t = astg_find_trans (stg,astg_signal_name(s),ASTG_DUMMY_X,1,ASTG_TRUE);
	    astg_new_edge (new_t,p);
	    astg_foreach_input_place (t,gen2,fan_place) {
		astg_new_edge (fan_place,new_t);
	    }

	    new_t = astg_find_trans (stg,astg_signal_name(s),ASTG_DUMMY_X,2,ASTG_TRUE);
	    astg_new_edge (p,new_t);
	    astg_foreach_output_place (t,gen2,fan_place) {
		astg_new_edge (new_t,fan_place);
	    }

	    /* Use same copy number to generate a toggle transition. */
	    new_t = astg_find_trans (stg,astg_signal_name(astg_trans_sig(t)),
			ASTG_TOGGLE_X,t->type.trans.copy_n,ASTG_TRUE);
	    astg_new_edge (p,new_t);
	    astg_new_edge (new_t,p);

	    astg_delete_trans (t);
	}
    }

    astg_foreach_trans (stg,gen,t) {
	if (astg_in_degree(t) == 0) {
	    printf("warning: '%s' has no in edges\n",astg_trans_name(t));
	}
	if (astg_out_degree(t) == 0) {
	    printf("warning: '%s' has no out edges\n",astg_trans_name(t));
	}
    }

    astg_foreach_place (stg,gen,p) {
	astg_make_place_name (stg,p);
	if (astg_in_degree(p) == 0) {
	    printf("warning: '%s' has no in edges\n",astg_place_name(p));
	}
	if (astg_out_degree(p) == 0) {
	    printf("warning: '%s' has no out edges\n",astg_place_name(p));
	}
    }
}

static void read_graph (source,stg)
io_t *source;
astg_graph *stg;
{
    int c;

    while (io_status(source) == 0) {
	c = io_peek (source);
	if (isalpha(c)) {
	    read_vertex (source,stg);
	} else if (c == '.') {
	    break;
	} else if (c == '\n') {
	    io_getc (source);
	} else {
	    io_error (source,"bad vertex description");
	}
    }

    read_graph_check (source,stg);
}

static void read_signals (source,stg,sig_type)
io_t *source;
astg_graph *stg;
astg_signal_enum sig_type;
{
    while (io_status(source) == 0 && io_token(source) != '\n') {
	if (isalpha(source->buffer[0]) &&
		strchr(source->buffer,'/') == NULL &&
		strchr(source->buffer,'-') == NULL &&
		strchr(source->buffer,'+') == NULL) {
	    astg_find_signal (stg,source->buffer,sig_type,ASTG_TRUE);
	}
	else {
	    io_error (source,"invalid signal name");
	}
    }
}

static void read_note (source,stg)
io_t *source;
astg_graph *stg;
{
    int c;
    char *p;

    p = source->buffer;
    while ((c=io_get(source)) != '\n' && c != EOF) {
	*(p++) = c;
    }
    *p = '\0';
    lsNewEnd (stg->comments,util_strsav(source->buffer),LS_NH);
}

static void read_stg (source,stg)
io_t *source;
astg_graph *stg;
{
    int c;

    while ((c=io_token(source)) != EOF) {

	if (!strcmp(source->buffer,".end")) {
	    /* No newline required for this. */
	    break;
	}
	else if (!strcmp(source->buffer,".model") ||
		 !strcmp(source->buffer,".name")) {
	    if (io_token (source) == '\n') {
		io_error(source,"no model name specified");
	    } else {
		FREE (stg->name);
		stg->name = util_strsav(source->buffer);
		io_mustbe (source,"\n");
	    }
	}
	else if (!strcmp(source->buffer,".note")) {
	    read_note (source,stg);
	}
	else if (!strcmp(source->buffer,".graph")) {
	    io_mustbe (source,"\n");
	    read_graph (source,stg);
	}
	else if (!strcmp(source->buffer,".inputs")) {
	    read_signals (source,stg,ASTG_INPUT_SIG);
	}
	else if (!strcmp(source->buffer,".internal")) {
	    read_signals (source,stg,ASTG_INTERNAL_SIG);
	}
	else if (!strcmp(source->buffer,".outputs")) {
	    read_signals (source,stg,ASTG_OUTPUT_SIG);
	}
	else if (!strcmp(source->buffer,".dummy")) {
	    read_signals (source,stg,ASTG_DUMMY_SIG);
	}
	else if (!strcmp(source->buffer,".marking")) {
	    astg_read_marking (source,stg);
	}
	else if (c == '.') {
	    io_error (source,"unrecognized keyword");
	}
	else if (c != '\n') {
	    io_error (source,"what is this supposed to be?");
	}

    } /* end while */
}

extern astg_graph *astg_read (fin,fname)
FILE *fin;		/*i stream to read STG from		*/
char *fname;		/*i name of stream to use in messages	*/
{
    /*	Read an STG description from the opened stream.  The filename
	argument is used for error messages and to remember where the STG
	description came from. */

    io_t source;
    astg_graph *stg;

    io_open (&source,fin,(char *)NULL);

    stg = astg_new (fname);
    stg->filename = util_strsav (fname);

    read_stg (&source,stg);
    io_close (&source);		/* Caller must close fin */

    if (io_status(&source) == 0) {
	stg->file_count = astg_change_count (stg);
	dbg(2,write_blif(stdout,stg->guards,ASTG_FALSE,ASTG_FALSE));
    }
    else {
	astg_delete (stg);
	stg = NULL;
    }

    return stg;
}

/* ---------------------------- Write STG ----------------------------------- */

void astg_write_marking (stg,fout)
astg_graph *stg;
FILE *fout;
{
    int sep_char, n_token = 0;
    astg_generator pgen;
    astg_place *p;

    if (stg->has_marking) {
	sep_char = '{';
	astg_foreach_place (stg,pgen,p) {
	    if (p->type.place.initial_token) {
		n_token++;
		fputc(sep_char,fout); sep_char = ' ';
		fputs(astg_place_name(p),fout);
	    }
	}
	fputs ((n_token==0)?"{ }\n":"}\n",fout);
    }
}

static char *trim_float (value,buffer)
double value;
char *buffer;
{
    char *p;
    sprintf(buffer,"%f",value);
    p = buffer + strlen(buffer) - 1;
    while (p > buffer && *p == '0') p--;
    if (p > buffer && *p == '.') p--;
    *(p+1) = '\0';
    return buffer;
}

int astg_write (stg,hide_places,stream)
astg_graph *stg;
astg_bool hide_places;
FILE *stream;
{
    astg_signal *sig_p;
    astg_edge *se;
    astg_trans *t;
    astg_place *p;
    astg_generator tgen, egen, pgen, sgen;
    lsGen cgen;
    lsGeneric data;
    int n_internal, n_dummy;
    char fbuf1[20], fbuf2[20];

    fprintf(stream,".model %s\n",stg->name);
    cgen = lsStart (stg->comments);
    while (lsNext(cgen,&data,LS_NH) == LS_OK) {
	fprintf(stream,".note%s\n",(char *) data);
    }
    lsFinish (cgen);

    n_dummy = n_internal = 0;
    fputs(".inputs",stream);
    astg_foreach_signal (stg,sgen,sig_p) {
	if (sig_p->sig_type == ASTG_INPUT_SIG) {
	   fprintf(stream," %s",sig_p->name);
	}
	if (sig_p->sig_type == ASTG_INTERNAL_SIG) n_internal++;
	if (sig_p->sig_type == ASTG_DUMMY_SIG) n_dummy++;
    }
    fputs("\n",stream);

    fputs(".outputs",stream);
    astg_foreach_signal (stg,sgen,sig_p) {
	if (sig_p->sig_type == ASTG_OUTPUT_SIG) {
	   fprintf(stream," %s",sig_p->name);
	}
    }
    fputs("\n",stream);

    if (n_internal != 0) {
	fputs(".internal",stream);
	astg_foreach_signal (stg,sgen,sig_p) {
	    if (sig_p->sig_type == ASTG_INTERNAL_SIG) {
	       fprintf(stream," %s",sig_p->name);
	    }
	}
	fputs("\n",stream);
    }

    if (n_dummy != 0) {
	fputs(".dummy",stream);
	astg_foreach_signal (stg,sgen,sig_p) {
	    if (sig_p->sig_type == ASTG_DUMMY_SIG) {
	       fprintf(stream," %s",sig_p->name);
	    }
	}
	fputs("\n",stream);
    }

    fputs(".graph\n",stream);
    astg_foreach_place (stg,pgen,p) {
	if (hide_places && astg_boring_place(p)) continue;
	fputs (astg_v_name(p),stream);
	if (p->y != 0 || p->x != 0) {
	    fprintf(stream," (%s,%s)",trim_float(p->x,fbuf1),trim_float(p->y,fbuf2));
	}
	astg_foreach_out_edge (p,egen,se) {
	    t = astg_head (se);
	    fputs(" ",stream);
	    fputs(astg_trans_name(t),stream);
	    if (se->guard_eqn != NULL) {
		fprintf(stream," ?%s",se->guard_eqn);
	    }
	}
	fputs("\n",stream);
    }

    astg_foreach_trans (stg,tgen,t) {
	fputs (astg_trans_name(t),stream);
	if (t->type.trans.delay != 0) {
	    fprintf(stream," %s",trim_float(t->type.trans.delay,fbuf1));
	}
	if (t->y != 0 || t->x != 0) {
	    fprintf(stream," (%s,%s)",trim_float(t->x,fbuf1),trim_float(t->y,fbuf2));
	}
	astg_foreach_output_place (t,pgen,p) {
	    if (hide_places && astg_boring_place(p)) {
		fprintf(stream," %s",astg_v_name(p->out_edges->head));
	    }
	    else {
		fprintf(stream," %s",astg_v_name(p));
	    }
	}
	fputs("\n",stream);
    }

    if (stg->has_marking) {
	fputs(".marking ",stream);
	astg_write_marking (stg,stream);
    }

    fputs(".end\n",stream);
}
#endif /* SIS */
