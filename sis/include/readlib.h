#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
    tree_node_t       *nodeval;
    char              *strval;
    double            realval;
    pin_info_t        *pinval;
    function_t        *functionval;
    latch_info_t      *latchval;
    constraint_info_t *constrval;
} GENLIB_yystype;
# define YYSTYPE GENLIB_yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define    OPR_OR    257
# define    OPR_AND    258
# define    CONST1    259
# define    CONST0    260
# define    IDENTIFIER    261
# define    LPAREN    262
# define    REAL    263
# define    OPR_NOT    264
# define    OPR_NOT_POST    265
# define    GATE    266
# define    PIN    267
# define    SEMI    268
# define    ASSIGN    269
# define    RPAREN    270
# define    LATCH    271
# define    CONTROL    272
# define    CONSTRAINT    273
# define    SEQ    274


extern YYSTYPE GENLIB_yylval;

#endif /* not BISON_Y_TAB_H */
