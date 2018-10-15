%{
/* file @(#)readliblex.l	1.1                      */
/* last modified on 5/29/91 at 12:35:32   */
#include "genlib_int.h"
#include "readlib.h"
#undef input
#undef unput
#ifdef FLEX_SCANNER
#undef YY_INPUT
#define YY_INPUT(buf,result,max) (result = genlib_input(buf, max))
#endif

static char *file_string;
static int file_mode;
static int nbuffer;
static char buffer[20];

/*alnum			[-A-Za-z0-9_.<>$%^&|{}[\]:,?/@]*/
%}

blank			[ \n\t]
alnum			[-A-Za-z0-9_]
mant1			([0-9]+\.?[0-9]*)
mant2			([0-9]*\.[0-9]+)
exp			([eE][-+]?[0-9]+)
float			([-+]?({mant1}|{mant2}){exp}?)

%%

\"[^"]+\"		{ 
   			    int i;
			    yytext[yyleng-1] = '\0';
			    for(i = 0; i < yyleng; i++) yytext[i] = yytext[i+1];
			    yyleng -= 2;
			    return IDENTIFIER; 
			}

"("			{ return LPAREN; }
")"			{ return RPAREN; }
";"			{ return SEMI; }
"="			{ return ASSIGN; }
"!"			{ return OPR_NOT; }
"'"			{ return OPR_NOT_POST; }
"+"			{ return OPR_OR; }
"*"			{ return OPR_AND; }

GATE			{ return GATE; }
PIN			{ return PIN; }
LATCH			{ return LATCH; }
CONTROL                 { return CONTROL; }
CONSTRAINT              { return CONSTRAINT; }
SEQ			{ return SEQ; }
"CONST0"		{ return CONST0; }
"CONST1"		{ return CONST1; }
{float}{blank}		{ return REAL;}
{alnum}+		{ return IDENTIFIER; }
{blank}+		;
#.*			; 		/* comments */

.			{ yyerror("bad character"); }

%%

#if 0 /* #ifndef FLEX_SCANNER */
static int
input()
{
    register int c;

    if (nbuffer > 0) {
	c = buffer[--nbuffer];
    } else {
	if (file_mode) {
	    c = getc(yyin);
	} else {
	    c = *file_string;
	    if (c != 0) file_string++;
	}
    }
    if (c == '\n') read_lineno++;
    if (c == EOF) c = 0;
    return c;
}


static int
unput(c)
int c;
{
    if (nbuffer < 20) buffer[nbuffer++] = c;
    if (c == '\n') read_lineno--;
    return c;
}
#else

char *genlib_inp_ptr;
char *genlib_inp_lim;

int genlib_input(char* buf, int max_size)
{
    char c;
    int i, n;

    if (! file_mode) {
	if (max_size > (genlib_inp_lim - genlib_inp_ptr))
	    n = genlib_inp_lim - genlib_inp_ptr;
	else 
	    n = max_size;	
	if (n > 0) {
	    for (i = 0; i < n; i++)
		if (buf[i] == '\n') {
		    read_lineno++;
		    i++;
		    break;
		}
	    memcpy (buf, genlib_inp_ptr, i);
	    genlib_inp_ptr += i;
	}
    } else {
	c = getc(yyin);
	if (c == '\n') read_lineno++;
	buf[0] = (c == EOF) ? (n = 0, YY_NULL) : (n = 1, c);
    }
    return n;
}
#endif

int
library_setup_file(fp, filename)
FILE *fp;
char *filename;
{
    yyin = fp;
#if 1 /* #ifdef FLEX_SCANNER */
    yy_switch_to_buffer( yy_create_buffer( yyin, YY_BUF_SIZE ) );
    yyrestart(yyin); 
    BEGIN(INITIAL);
#endif
    file_mode = 1;
    nbuffer = 0;
    read_register_filename(filename);
}

int
library_setup_string(string)
char *string;
{
    file_string = string;
    file_mode = 0;
#if 1 /* #ifdef FLEX_SCANNER */
    yy_switch_to_buffer(
                 yy_create_buffer( yyin, YY_BUF_SIZE ) );
    yyrestart(yyin); 
    genlib_inp_lim = string + strlen(string);
    genlib_inp_ptr = string;
#endif
    nbuffer = 0;
    read_register_filename(NIL(char));
}

#undef yywrap 
int
yywrap()
{
    return 1;
}
