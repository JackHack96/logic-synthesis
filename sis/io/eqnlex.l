%{
#include "sis.h"
#include "io_int.h"
#include "read_eqn.h"
#undef input
#undef unput
#ifdef FLEX_SCANNER
#undef YY_INPUT
#define YY_INPUT(buf,result,max) (result = eqn_input(buf, max))
#endif

static char *string;
static int file_mode;
static int nbuffer;
static char buffer[20];
static int last_was_newline;

extern int yyerror(char *);
%}

blank			[ \n\t]

alpha			[A-Za-z]
alnum			[A-Za-z0-9_.,<>$%[\]:~/-]

%%

({alnum}*) { 
			    if (strcmp(yytext, "0") == 0) {
				return CONST_ZERO;
			    } else if(strcmp(yytext, "1") == 0) {
				return CONST_ONE;
			    } else if (strcmp(yytext, "NAME") == 0) {
				return NAME;
			    } else if (strcmp(yytext, "INORDER") == 0) {
				return INORDER;
			    } else if (strcmp(yytext, "OUTORDER") == 0) {
				return OUTORDER;
			    } else if (strcmp(yytext, "END") == 0) {
				return END;
			    } else {
				return IDENTIFIER; 
			    }
			}


\"[^"]+\"		{ 
   			    int i;

			    yytext[yyleng-1] = '\0';
			    for(i = 0; i < yyleng; i++) {
				yytext[i] = yytext[i+1];
			    }
			    yyleng -= 2;
			    return IDENTIFIER; 
			}
"("			{ return LPAREN; }
")"			{ return RPAREN; }
";"			{ return SEMI; }
"="			{ return ASSIGN; }

"!"			{ return OPR_NOT; }
"'"			{ return OPR_NOT_POST; }
"=="			{ return OPR_XNOR; }
"!="			{ return OPR_XOR; }
"^"			{ return OPR_XOR; }
"&"			{ return OPR_AND; }
"|"			{ return OPR_OR; }
"*"			{ return OPR_AND; }
"+"			{ return OPR_OR; }

{blank}+		;
#.*			; 		/* comments */
.			{ yyerror("bad character"); }

%%

#if 0 /* #ifndef FLEX_SCANNER */
static int
input()
{
    register int c;
    register char *prompt;

    if (nbuffer > 0) {
	c = buffer[--nbuffer];
    } else if (file_mode) {
	if (last_was_newline && yyin == stdin) {
	    prompt = com_get_flag("prompt2");
	    if (prompt != NULL) fputs(prompt,stdout);
	}
	c = getc(yyin);
    } else {
	c = *string;
	if (c != 0) string++;
    }

    last_was_newline = (c == '\n');
    if (last_was_newline) read_lineno++;
    if (c == EOF) c = 0;
    return c;
}

static int
unput(c)
int c;
{
    if (nbuffer < 20) buffer[nbuffer++] = c;
    if (c == '\n') read_lineno--;
}
#else

char *eqn_inp_ptr;
char* eqn_inp_lim;

int eqn_input(char* buf, int max_size)
{
    char c;
    int i, n;
	 
    if (! file_mode) {
	if (max_size > (eqn_inp_lim - eqn_inp_ptr))
	    n = eqn_inp_lim - eqn_inp_ptr;
	else
	    n= max_size;
	if (n > 0) {
	    for (i = 0; i < n; i++)
		if (eqn_inp_ptr[i] == '\n') {
		    read_lineno++;
		    i++;
		    break;
		}
	    memcpy (buf, eqn_inp_ptr, i);
	    eqn_inp_ptr += i;
	}
    } else {
	c = getc(yyin);
	if (c == '\n')
	    read_lineno++;
	buf[0] = (c == EOF) ? (n = 0, YY_NULL) : (n = 1, c);
    }
    return n;
}

#endif

int
equation_setup_string(s)
char *s;
{
    read_register_filename(NIL(char));
    file_mode = 0;
    string = s;
#if 1 /* #ifdef FLEX_SCANNER */
    yy_switch_to_buffer(
                 yy_create_buffer( yyin, YY_BUF_SIZE ) );
    yyrestart(yyin); 
    eqn_inp_lim = string + strlen(string);
    eqn_inp_ptr = string;
#endif
    nbuffer = 0;
}

int
equation_setup_file(fp)
FILE *fp;
{
    yyin = fp;
#if 1 /* #ifdef FLEX_SCANNER */
    yy_switch_to_buffer(
                 yy_create_buffer( yyin, YY_BUF_SIZE ) );
    yyrestart(yyin); 
    BEGIN(INITIAL);
#endif
    file_mode = 1;
    nbuffer = 0;
    last_was_newline = 1;
}

#undef yywrap
int
yywrap()
{
    return 1;
}
