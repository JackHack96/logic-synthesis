
%{
/* file @(#)readlib.y	1.2                      */
/* last modified on 6/13/91 at 17:46:40   */
#include <setjmp.h>
#include "genlib_int.h"
#include "config.h"

#undef yywrap 
static int input();
static int unput();
static int yywrap();

FILE *gl_out_file;
#if YYTEXT_POINTER
extern char *yytext;
#else
extern char yytext[];
#endif

static int global_use_nor;
static function_t *global_fct;

extern int library_setup_string(char *);
extern int library_setup_file(FILE *, char *);

%}

%union {
    tree_node_t *nodeval;
    char *strval;
    double realval; 
    pin_info_t *pinval;
    function_t *functionval;
    latch_info_t *latchval;
    constraint_info_t *constrval;
}


%type <nodeval> expr
%type <strval> name pin_name pin_phase
%type <realval> real
%type <pinval> pins pin_info
%type <functionval> function
%type <pinval> clock
%type <constrval> constraint constraints
%type <strval> type
%type <latchval> latch
%left OPR_OR
%left OPR_AND CONST1 CONST0 IDENTIFIER LPAREN REAL
%left OPR_NOT OPR_NOT_POST
%token GATE PIN SEMI ASSIGN RPAREN
%token LATCH CONTROL CONSTRAINT SEQ
%start hack

%%
hack	:	gates
	|	function
		    {global_fct = $1; }
	;

gates	:	/* empty */
	|	gates gate_info
	;

gate_info:	GATE name real function pins
		    { 
			if (! gl_convert_gate_to_blif($2,$3,$4,$5,global_use_nor))
			    yyerror(NIL(char));
		    }
	|	LATCH name real function pins latch clock constraints
		    {
		      if (! gl_convert_latch_to_blif($2,$3,$4,$5,global_use_nor,$6,$7,$8))
			yyerror(NIL(char));
		    }
	;

pins	:	/* empty */
		    { $$ = 0; }
	|	pins pin_info
		    { $2->next = $1; $$ = $2;}
	;

pin_info:	PIN pin_name pin_phase real real real real real real
		    {
			$$ = ALLOC(pin_info_t, 1);
			$$->name = $2;
			$$->phase = $3;
			$$->value[0] = $4; $$->value[1] = $5;
			$$->value[2] = $6; $$->value[3] = $7;
			$$->value[4] = $8; $$->value[5] = $9;
			$$->next = 0;
		    }
	;

pin_phase:	name
		    {
			if (strcmp($1, "INV") != 0) {
			    if (strcmp($1, "NONINV") != 0) {
				if (strcmp($1, "UNKNOWN") != 0) {
				    yyerror("bad pin phase");
				}
			    }
			}
			$$ = $1;
		    }
	;

pin_name:	name
		    { $$ = $1; }
	|	OPR_AND
		    { $$ = util_strsav(yytext); }
	;

function:	name ASSIGN expr SEMI
		    {
			$$ = ALLOC(function_t, 1);
			$$->name = $1;
			$$->tree = $3;
			$$->next = 0;
		    }
	;

expr	:	expr OPR_OR expr
		    { 
			$$ = gl_alloc_node(2);
			$$->phase = 1;
			$$->sons[0] = $1;
			$$->sons[1] = $3;
			$$->type = OR_NODE;
		    }
	|	expr OPR_AND expr
		    {
			$$ = gl_alloc_node(2);
			$$->phase = 1;
			$$->sons[0] = $1;
			$$->sons[1] = $3;
			$$->type = AND_NODE;
		    }
	|	expr expr %prec OPR_AND
		    {
			$$ = gl_alloc_node(2);
			$$->phase = 1;
			$$->sons[0] = $1;
			$$->sons[1] = $2;
			$$->type = AND_NODE;
		    }
	|       LPAREN expr RPAREN
		    { 
			$$ = $2; 
		    }
	|	OPR_NOT expr
		    { 
			$$ = $2; 
			$$->phase = 0; 
		    }
	|	expr OPR_NOT_POST
		    { 
			$$ = $1; 
			$$->phase = 0; 
		    }
	|	name
		    {
			$$ = gl_alloc_leaf($1);
			FREE($1);
			$$->type = LEAF_NODE;	/* hack */
			$$->phase = 1;
		    }
	|	CONST0
		    {
			$$ = gl_alloc_leaf("0");
			$$->phase = 1;
			$$->type = ZERO_NODE;
		    }
	| 	CONST1
		    {
			$$ = gl_alloc_leaf("1");
			$$->phase = 1;
			$$->type = ONE_NODE;
		    }
	;

name	:	IDENTIFIER
		    { $$ = util_strsav(yytext); }
	|
		REAL
		    { $$ = util_strsav(yytext); } 
	;

real	:	REAL
		    { $$ = atof(yytext); }
	;

clock	:	/* empty */
		{ $$ = 0;}
	|	CONTROL name real real real real real real
		{ $$ = ALLOC(pin_info_t, 1);
		  $$->name = $2;
		  $$->phase = util_strsav("UNKNOWN");
		  $$->value[0] = $3;
		  $$->value[1] = $4;
		  $$->value[2] = $5;
		  $$->value[3] = $6;
		  $$->value[4] = $7;
		  $$->value[5] = $8;
		  $$->next = 0;
		}
	;

constraints:	/* empty */
		{ $$ = 0; }
	|	constraints constraint
		{
		  $2->next = $1;
		  $$ = $2;
		}
        ;

constraint:     CONSTRAINT pin_name real real
		{
		  $$ = ALLOC(constraint_info_t, 1);
		  $$->name = $2;
		  $$->setup = $3;
		  $$->hold = $4;
		  $$->next = 0;
		}
	;

latch	:	SEQ name name type
		{ 
		  $$ = ALLOC(latch_info_t, 1);
		  $$->in = $2;
		  $$->out = $3;
		  $$->type = $4;
		}
	;

type	:	IDENTIFIER
		{ 
		  if (strcmp(yytext, "RISING_EDGE") == 0) {
		    $$ = util_strsav("re");
		  } else if (strcmp(yytext, "FALLING_EDGE") == 0) {
		    $$ = util_strsav("fe");
		  } else if (strcmp(yytext, "ACTIVE_HIGH") == 0) {
		    $$ = util_strsav("ah");
		  } else if (strcmp(yytext, "ACTIVE_LOW") == 0) {
		    $$ = util_strsav("al");
		  } else if (strcmp(yytext, "ASYNCH") == 0) {
		    $$ = util_strsav("as");
		  } else {
		    yyerror("latch type can only be re/fe/ah/al/as");
		  }
		}
	;

%%
static jmp_buf jmpbuf;

int
yyerror(errmsg)
char *errmsg;
{
    if (errmsg != 0) read_error(errmsg);
    longjmp(jmpbuf, 1);
}

int
gl_convert_genlib_to_blif(filename, use_nor)
char *filename;
int use_nor;
{
    FILE *fp;

    error_init();
    if ((fp = fopen(filename, "r")) == NULL) {
	perror(filename);
	return 0;
    }
    library_setup_file(fp, filename);
    gl_out_file = stdout;

    global_fct = 0;		/* not used here, just to check for error */
    global_use_nor = use_nor;

    if (setjmp(jmpbuf)) {
	return 0;
    } else {
	(void) yyparse();
	if (global_fct != 0) yyerror("syntax error");
	return 1;
    }
}

int
genlib_parse_library(infp, infilename, outfp, use_nor)
FILE *infp;
char *infilename;
FILE *outfp;
int use_nor;
{
    error_init();
    library_setup_file(infp, infilename);
    gl_out_file = outfp;
    global_fct = 0;		/* not used here, just to check for error */
    global_use_nor = use_nor;

    if (setjmp(jmpbuf)) {
	return 0;
    } else {
	(void) yyparse();
	if (global_fct != 0) yyerror("syntax error");
	return 1;
    }
}

int
gl_convert_expression_to_tree(string, tree, name)
char *string;
tree_node_t **tree;
char **name;
{
    error_init();
    global_fct = 0;
    library_setup_string(string);

    if (setjmp(jmpbuf)) {
	return 0;
    } else {
	(void) yyparse();
	if (global_fct == 0) yyerror("syntax error");
	*tree = global_fct->tree;
	*name = global_fct->name;
	return 1;
    }
}
