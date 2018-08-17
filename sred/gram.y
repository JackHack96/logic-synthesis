%{

#include "reductio.h"
extern char yytext[];
%}

%start fsm

%token DOT_I DOT_O DOT_S DOT_R DOT_P DOT_E NAME CUBE NUM

%%

fsm : dots 
	{
		type = MEALY;
	}
	table dot_e
	;

dots	: dot
		| dots dot
		;

dot 	: dot_i
		| dot_o
		| dot_r
		| dot_s
		| dot_p
		;

table	: line
		| table line
		;

dot_i	: DOT_I NUM
		{
			nis = atoi (yytext);
		}
		| DOT_I CUBE
		{
			nis = atoi (yytext);
		}
		;

dot_o	: DOT_O NUM
		{
			nos = atoi (yytext);
		}
		| DOT_O CUBE 
		{
			nos = atoi (yytext);
		}
		;

dot_r	: DOT_R NAME
		{
			strcpy (startstate, yytext);
		}
		;

dot_s	: DOT_S NUM
		| DOT_S CUBE
		;

dot_p	: DOT_P NUM
		| DOT_P CUBE
		;

dot_e	: DOT_E
		|
		;

line	: input state next output
		{
			mealy ();
		}
		;

input	: CUBE
		{
			strcpy (lastin, yytext);
		}
		;

output	: CUBE
		{
			strcpy (lastout, yytext);
		}
		;

state	: NAME 
		{
			strcpy (laststate, yytext);
		}
		;

next	: NAME 
		{
			strcpy (lastnext, yytext);
		}
		;

