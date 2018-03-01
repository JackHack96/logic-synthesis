#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
    char *strval;
    node_t *node;
    array_t *array;
} EQN_yystype;
# define YYSTYPE EQN_yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	OPR_OR	257
# define	OPR_AND	258
# define	CONST_ZERO	259
# define	CONST_ONE	260
# define	IDENTIFIER	261
# define	LPAREN	262
# define	OPR_XOR	263
# define	OPR_XNOR	264
# define	OPR_NOT	265
# define	OPR_NOT_POST	266
# define	NAME	267
# define	INORDER	268
# define	OUTORDER	269
# define	PASS	270
# define	ASSIGN	271
# define	SEMI	272
# define	RPAREN	273
# define	END	274


extern YYSTYPE EQN_yylval;

#endif /* not BISON_Y_TAB_H */
