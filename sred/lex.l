%{

#include "gram.h"
#ifdef getc
#undef getc
#endif
#define getc(file) mygetc(file)
#define tolower(c)	((c) >= 'A' && (c) <= 'Z' ? (c) | 0x20:(c))
%}
%%
\.i	{
	return DOT_I;
}

\.o	{
	return DOT_O;
}

\.s	{
	return DOT_S;
}

\.r	{
	return DOT_R;
}

\.p	{
	return DOT_P;
}

\.e	{
	return DOT_E;
}

[a-zA-Z][a-zA-Z0-9_]*	{
		return NAME;
	}

[01-]+	{
		return CUBE;
	}

[0-9]+	{
		return NUM;
	}

[\t\n ]	{
		/* ignore */
	}


[^ ]	{
	 return yytext[0];
	}

%%
yywrap()
{
	return(1);
}
