#ifndef BISON_GRAM_H
# define BISON_GRAM_H

# ifndef YYSTYPE
#  define YYSTYPE int
#  define YYSTYPE_IS_TRIVIAL 1
# endif
# define    DOT_I    257
# define    DOT_O    258
# define    DOT_S    259
# define    DOT_R    260
# define    DOT_P    261
# define    DOT_E    262
# define    NAME    263
# define    CUBE    264
# define    NUM    265


extern YYSTYPE yylval;

#endif /* not BISON_GRAM_H */
