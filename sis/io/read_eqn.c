/* A Bison parser, made from read_eqn.y
   by GNU bison 1.35.  */

#define YYBISON 1  /* Identify Bison output.  */

# define    OPR_OR    257
# define    OPR_AND    258
# define    CONST_ZERO    259
# define    CONST_ONE    260
# define    IDENTIFIER    261
# define    LPAREN    262
# define    OPR_XOR    263
# define    OPR_XNOR    264
# define    OPR_NOT    265
# define    OPR_NOT_POST    266
# define    NAME    267
# define    INORDER    268
# define    OUTORDER    269
# define    PASS    270
# define    ASSIGN    271
# define    SEMI    272
# define    RPAREN    273
# define    END    274

#line 10 "read_eqn.y"

#include "sis.h"
#include "../include/io_int.h"
#include <setjmp.h>
#include "config.h"

#if YYTEXT_POINTER
extern char *EQN_yytext;
#else
extern char EQN_yytext[];
#endif

static network_t *global_network;
static lsList    po_list;

int EQN_yyerror();

#if 0 /* #ifndef FLEX_SCANNER */
#undef EQN_yywrap
static int input();
static int unput();
static int EQN_yywrap();
#endif

extern int equation_setup_string(char *);

extern int equation_setup_file(FILE *);

static void
do_assign(name, expr)
        char *name;
        node_t *expr;
{
    char   errmsg[1024];
    node_t *node;

    node = read_find_or_create_node(global_network, name);
    if (node_function(node) != NODE_UNDEFINED) {
        (void) sprintf(errmsg, "Attempt to redefine '%s'\n", name);
        EQN_yyerror(errmsg);   /* never returns */
    }
    FREE(name);
    node_replace(node, expr);
}

static node_t *
do_sexpr_list(list, func)
        array_t *list;
        node_t *(*func)();
{
    int    i;
    node_t *node, *node1, *node2;

    node1 = array_fetch(node_t * , list, 0);
    node  = node_dup(node1);
    node_free(node1);
    for (i = 1; i < array_n(list); i++) {
        node1 = node;
        node2 = array_fetch(node_t * , list, i);
        node  = (*func)(node1, node2);
        node_free(node1);
        node_free(node2);
    }
    array_free(list);
    return node;
}


#line 77 "read_eqn.y"
#ifndef YYSTYPE
typedef union {
    char    *strval;
    node_t  *node;
    array_t *array;
}                EQN_yystype;
# define YYSTYPE EQN_yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
# define YYDEBUG 0
#endif


#define    YYFINAL        70
#define    YYFLAG        -32768
#define    YYNTBASE    21

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 274 ? EQN_yytranslate[x] : 30)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char EQN_yytranslate[] =
                          {
                                  0, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                  2, 2, 2, 2, 2, 2, 1, 3, 4, 5,
                                  6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                  16, 17, 18, 19, 20
                          };

#if YYDEBUG
static const short EQN_yyprhs[] =
{
       0,     0,     1,     3,     7,     9,    12,    16,    17,    21,
      25,    29,    33,    39,    43,    47,    50,    54,    58,    61,
      64,    66,    68,    72,    74,    79,    84,    89,    91,    93,
      97,   101,   103,   105,   108,   110,   111,   114,   115
};
static const short EQN_yyrhs[] =
{
      -1,    22,     0,    22,    20,    18,     0,    25,     0,    23,
      18,     0,    22,    23,    18,     0,     0,    13,    17,    27,
       0,    14,    17,    28,     0,    15,    17,    29,     0,    27,
      17,    24,     0,     8,    17,    27,    25,    19,     0,    24,
       3,    24,     0,    24,     4,    24,     0,    24,    24,     0,
      24,     9,    24,     0,    24,    10,    24,     0,    24,    12,
       0,    11,    24,     0,     5,     0,     6,     0,     8,    24,
      19,     0,    27,     0,     8,     4,    26,    19,     0,     8,
       3,    26,    19,     0,     8,    11,    25,    19,     0,     5,
       0,     6,     0,     8,     5,    19,     0,     8,     6,    19,
       0,    27,     0,    25,     0,    26,    25,     0,     7,     0,
       0,    28,    27,     0,     0,    29,    27,     0
};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short EQN_yyrline[] =
{
       0,    94,    95,    96,    97,   101,   102,   105,   106,   112,
     114,   116,   119,   124,   127,   130,   133,   136,   139,   142,
     145,   148,   151,   154,   164,   167,   170,   173,   176,   179,
     182,   185,   195,   200,   206,   211,   212,   229,   230
};
#endif


#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const EQN_yytname[] =
{
  "$", "error", "$undefined.", "OPR_OR", "OPR_AND", "CONST_ZERO", 
  "CONST_ONE", "IDENTIFIER", "LPAREN", "OPR_XOR", "OPR_XNOR", "OPR_NOT", 
  "OPR_NOT_POST", "NAME", "INORDER", "OUTORDER", "PASS", "ASSIGN", "SEMI", 
  "RPAREN", "END", "program", "prog", "stat", "expr", "sexpr", 
  "sexpr_list", "identifier", "input_list", "output_list", 0
};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short EQN_yyr1[] =
                           {
                                   0, 21, 21, 21, 21, 22, 22, 23, 23, 23,
                                   23, 23, 23, 24, 24, 24, 24, 24, 24, 24,
                                   24, 24, 24, 24, 25, 25, 25, 25, 25, 25,
                                   25, 25, 26, 26, 27, 28, 28, 29, 29
                           };

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short EQN_yyr2[] =
                           {
                                   0, 0, 1, 3, 1, 2, 3, 0, 3, 3,
                                   3, 3, 5, 3, 3, 2, 3, 3, 2, 2,
                                   1, 1, 3, 1, 4, 4, 4, 1, 1, 3,
                                   3, 1, 1, 2, 1, 0, 2, 0, 2
                           };

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short EQN_yydefact[] =
                           {
                                   1, 27, 28, 34, 0, 0, 0, 0, 2, 0,
                                   4, 31, 0, 0, 0, 0, 0, 0, 0, 35,
                                   37, 0, 0, 0, 0, 5, 0, 0, 32, 0,
                                   31, 0, 29, 30, 0, 0, 8, 9, 10, 3,
                                   6, 20, 21, 0, 0, 11, 23, 25, 33, 24,
                                   26, 0, 36, 38, 0, 19, 0, 0, 0, 0,
                                   18, 15, 12, 22, 13, 14, 16, 17, 0, 0,
                                   0
                           };

static const short EQN_yydefgoto[] =
                           {
                                   68, 8, 9, 61, 28, 29, 46, 37, 38
                           };

static const short EQN_yypact[] =
                           {
                                   -4, -32768, -32768, -32768, 56, 2, 3, 6, 70, 4,
                                   -32768, 7, 36, 36, 11, 44, 36, 57, 57, -32768,
                                   -32768, 49, 54, 58, 7, -32768, 109, 107, -32768, 20,
                                   -32768, 63, -32768, -32768, 55, 36, -32768, 57, 57, -32768,
                                   -32768, -32768, -32768, 109, 109, 88, -32768, -32768, -32768, -32768,
                                   -32768, 60, -32768, -32768, 46, -32768, 109, 109, 109, 109,
                                   -32768, 112, -32768, -32768, 97, 112, -5, -5, 75, 80,
                                   -32768
                           };

static const short EQN_yypgoto[] =
                           {
                                   -32768, -32768, 73, -11, 5, 74, 0, -32768, -32768
                           };


#define    YYLAST        124


static const short EQN_yytable[] =
                           {
                                   11, 1, 2, 3, 4, 10, 44, 60, 24, 5,
                                   6, 7, 30, 30, -7, 45, 30, 35, 36, 18,
                                   19, 34, 25, 20, 26, 1, 2, 3, 27, 30,
                                   32, 30, 54, 55, 48, 30, 48, 52, 53, 47,
                                   51, 1, 2, 3, 27, 64, 65, 66, 67, 56,
                                   57, 41, 42, 3, 43, 58, 59, 44, 60, 12,
                                   13, 14, 15, 33, 3, 63, 17, 16, 1, 2,
                                   3, 27, 39, 17, 50, 69, 40, 3, 21, 62,
                                   70, 23, 49, 5, 6, 7, 0, 31, -7, 0,
                                   22, 56, 57, 41, 42, 3, 43, 58, 59, 44,
                                   60, 57, 41, 42, 3, 43, 58, 59, 44, 60,
                                   12, 13, 14, 15, 41, 42, 3, 43, 16, 0,
                                   44, 58, 59, 44, 60
                           };

static const short EQN_yycheck[] =
                           {
                                   0, 5, 6, 7, 8, 0, 11, 12, 8, 13,
                                   14, 15, 12, 13, 18, 26, 16, 17, 18, 17,
                                   17, 16, 18, 17, 17, 5, 6, 7, 8, 29,
                                   19, 31, 43, 44, 29, 35, 31, 37, 38, 19,
                                   35, 5, 6, 7, 8, 56, 57, 58, 59, 3,
                                   4, 5, 6, 7, 8, 9, 10, 11, 12, 3,
                                   4, 5, 6, 19, 7, 19, 17, 11, 5, 6,
                                   7, 8, 18, 17, 19, 0, 18, 7, 8, 19,
                                   0, 8, 19, 13, 14, 15, -1, 13, 18, -1,
                                   20, 3, 4, 5, 6, 7, 8, 9, 10, 11,
                                   12, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                   3, 4, 5, 6, 5, 6, 7, 8, 11, -1,
                                   11, 9, 10, 11, 12
                           };
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/share/bison/bison.simple"

/* Skeleton output parser for bison,

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software
   Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser when
   the %semantic_parser declaration is not specified in the grammar.
   It was written by Richard Stallman by simplifying the hairy parser
   used when %semantic_parser is specified.  */

/* All symbols defined below should begin with EQN_yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if !defined (EQN_yyoverflow) || defined (YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
/* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (EQN_yyoverflow) || defined (YYERROR_VERBOSE) */


#if (!defined (EQN_yyoverflow) \
 && (!defined (__cplusplus) \
 || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union EQN_yyalloc {
    short EQN_yyss;
    YYSTYPE EQN_yyvs;
# if YYLSP_NEEDED
    YYLTYPE EQN_yyls;
# endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAX (sizeof (union EQN_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# if YYLSP_NEEDED
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE) + sizeof (YYLTYPE))	\
      + 2 * YYSTACK_GAP_MAX)
# else
#  define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))                \
      + YYSTACK_GAP_MAX)
# endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
    {					\
      register YYSIZE_T EQN_yyi;		\
      for (EQN_yyi = 0; EQN_yyi < (Count); EQN_yyi++)	\
        (To)[EQN_yyi] = (From)[EQN_yyi];		\
    }					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)                    \
    do                                    \
      {                                    \
    YYSIZE_T EQN_yynewbytes;                        \
    YYCOPY (&EQN_yyptr->Stack, Stack, EQN_yysize);                \
    Stack = &EQN_yyptr->Stack;                        \
    EQN_yynewbytes = EQN_yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAX;    \
    EQN_yyptr += EQN_yynewbytes / sizeof (*EQN_yyptr);                \
      }                                    \
    while (0)

#endif


#if !defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if !defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if !defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if !defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define EQN_yyerrok        (EQN_yyerrstatus = 0)
#define EQN_yyclearin    (EQN_yychar = YYEMPTY)
#define YYEMPTY        -2
#define YYEOF        0
#define YYACCEPT    goto EQN_yyacceptlab
#define YYABORT    goto EQN_yyabortlab
#define YYERROR        goto EQN_yyerrlab1
/* Like YYERROR except do call EQN_yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL        goto EQN_yyerrlab
#define YYRECOVERING()  (!!EQN_yyerrstatus)
#define YYBACKUP(Token, Value)                    \
do                                \
  if (EQN_yychar == YYEMPTY && EQN_yylen == 1)                \
    {                                \
      EQN_yychar = (Token);                        \
      EQN_yylval = (Value);                        \
      EQN_yychar1 = YYTRANSLATE (EQN_yychar);                \
      YYPOPSTACK;                        \
      goto EQN_yybackup;                        \
    }                                \
  else                                \
    {                                \
      EQN_yyerror ("syntax error: cannot back up");            \
      YYERROR;                            \
    }                                \
while (0)

#define YYTERROR    1
#define YYERRCODE    256


/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)        \
   Current.last_line   = Rhs[N].last_line;    \
   Current.last_column = Rhs[N].last_column;
#endif


/* YYLEX -- calling `EQN_yylex' with the right arguments.  */

#if YYPURE
# if YYLSP_NEEDED
#  ifdef YYLEX_PARAM
#   define YYLEX		EQN_yylex (&EQN_yylval, &EQN_yylloc, YYLEX_PARAM)
#  else
#   define YYLEX		EQN_yylex (&EQN_yylval, &EQN_yylloc)
#  endif
# else /* !YYLSP_NEEDED */
#  ifdef YYLEX_PARAM
#   define YYLEX		EQN_yylex (&EQN_yylval, YYLEX_PARAM)
#  else
#   define YYLEX		EQN_yylex (&EQN_yylval)
#  endif
# endif /* !YYLSP_NEEDED */
#else /* !YYPURE */
# define YYLEX            EQN_yylex ()
#endif /* !YYPURE */


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (EQN_yydebug)					\
    YYFPRINTF Args;				\
} while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int EQN_yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef    YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

# ifndef EQN_yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define EQN_yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
EQN_yystrlen (const char *EQN_yystr)
#   else
EQN_yystrlen (EQN_yystr)
     const char *EQN_yystr;
#   endif
{
  register const char *EQN_yys = EQN_yystr;

  while (*EQN_yys++ != '\0')
    continue;

  return EQN_yys - EQN_yystr - 1;
}
#  endif
# endif

# ifndef EQN_yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define EQN_yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
EQN_yystpcpy (char *EQN_yydest, const char *EQN_yysrc)
#   else
EQN_yystpcpy (EQN_yydest, EQN_yysrc)
     char *EQN_yydest;
     const char *EQN_yysrc;
#   endif
{
  register char *EQN_yyd = EQN_yydest;
  register const char *EQN_yys = EQN_yysrc;

  while ((*EQN_yyd++ = *EQN_yys++) != '\0')
    continue;

  return EQN_yyd - 1;
}
#  endif
# endif
#endif

#line 315 "/usr/share/bison/bison.simple"


/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into EQN_yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
#  define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL
# else
#  define YYPARSE_PARAM_ARG YYPARSE_PARAM
#  define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
# endif
#else /* !YYPARSE_PARAM */
# define YYPARSE_PARAM_ARG
# define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
# ifdef YYPARSE_PARAM
int EQN_yyparse (void *);
# else

int EQN_yyparse(void);

# endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES            \
/* The lookahead symbol.  */                \
int EQN_yychar;                        \
                            \
/* The semantic value of the lookahead symbol. */    \
YYSTYPE EQN_yylval;                        \
                            \
/* Number of parse errors so far.  */            \
int EQN_yynerrs;

#if YYLSP_NEEDED
# define YY_DECL_VARIABLES			\
YY_DECL_NON_LSP_VARIABLES			\
                        \
/* Location data for the lookahead symbol.  */	\
YYLTYPE EQN_yylloc;
#else
# define YY_DECL_VARIABLES            \
YY_DECL_NON_LSP_VARIABLES
#endif


/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif  /* !YYPURE */

int
EQN_yyparse(YYPARSE_PARAM_ARG)
YYPARSE_PARAM_DECL
{
    /* If reentrant, generate the variables here. */
#if YYPURE
    YY_DECL_VARIABLES
#endif  /* !YYPURE */

    register int EQN_yystate;
    register int EQN_yyn;
    int          EQN_yyresult;
    /* Number of tokens to shift before error messages enabled.  */
    int          EQN_yyerrstatus;
    /* Lookahead token as an internal (translated) token number.  */
    int          EQN_yychar1   = 0;

    /* Three stacks and their tools:
       `EQN_yyss': related to states,
       `EQN_yyvs': related to semantic values,
       `EQN_yyls': related to locations.

       Refer to the stacks thru separate pointers, to allow EQN_yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack. */
    short            EQN_yyssa[YYINITDEPTH];
    short            *EQN_yyss = EQN_yyssa;
    register short   *EQN_yyssp;

    /* The semantic value stack.  */
    YYSTYPE EQN_yyvsa[YYINITDEPTH];
    YYSTYPE *EQN_yyvs          = EQN_yyvsa;
    register YYSTYPE *EQN_yyvsp;

#if YYLSP_NEEDED
    /* The location stack.  */
    YYLTYPE EQN_yylsa[YYINITDEPTH];
    YYLTYPE *EQN_yyls = EQN_yylsa;
    YYLTYPE *EQN_yylsp;
#endif

#if YYLSP_NEEDED
# define YYPOPSTACK   (EQN_yyvsp--, EQN_yyssp--, EQN_yylsp--)
#else
# define YYPOPSTACK   (EQN_yyvsp--, EQN_yyssp--)
#endif

    YYSIZE_T EQN_yystacksize   = YYINITDEPTH;


    /* The variables used to return semantic value and location from the
       action routines.  */
    YYSTYPE EQN_yyval;
#if YYLSP_NEEDED
    YYLTYPE EQN_yyloc;
#endif

    /* When reducing, the number of symbols on the RHS of the reduced
       rule. */
    int EQN_yylen;

    YYDPRINTF ((stderr, "Starting parse\n"));

    EQN_yystate     = 0;
    EQN_yyerrstatus = 0;
    EQN_yynerrs     = 0;
    EQN_yychar      = YYEMPTY;        /* Cause a token to be read.  */

    /* Initialize stack pointers.
       Waste one element of value and location stack
       so that they stay on the same level as the state stack.
       The wasted elements are never initialized.  */

    EQN_yyssp = EQN_yyss;
    EQN_yyvsp = EQN_yyvs;
#if YYLSP_NEEDED
    EQN_yylsp = EQN_yyls;
#endif
    goto EQN_yysetstate;

/*------------------------------------------------------------.
| EQN_yynewstate -- Push a new state, which is found in EQN_yystate.  |
`------------------------------------------------------------*/
    EQN_yynewstate:
    /* In all cases, when you get here, the value and location stacks
       have just been pushed. so pushing a state here evens the stacks.
       */
    EQN_yyssp++;

    EQN_yysetstate:
    *EQN_yyssp = EQN_yystate;

    if (EQN_yyssp >= EQN_yyss + EQN_yystacksize - 1) {
        /* Get the current used size of the three stacks, in elements.  */
        YYSIZE_T EQN_yysize = EQN_yyssp - EQN_yyss + 1;

#ifdef EQN_yyoverflow
        {
      /* Give user a chance to reallocate the stack. Use copies of
         these so that the &'s don't force the real ones into
         memory.  */
      YYSTYPE *EQN_yyvs1 = EQN_yyvs;
      short *EQN_yyss1 = EQN_yyss;

      /* Each stack pointer address is followed by the size of the
         data in use in that stack, in bytes.  */
# if YYLSP_NEEDED
      YYLTYPE *EQN_yyls1 = EQN_yyls;
      /* This used to be a conditional around just the two extra args,
         but that might be undefined if EQN_yyoverflow is a macro.  */
      EQN_yyoverflow ("parser stack overflow",
              &EQN_yyss1, EQN_yysize * sizeof (*EQN_yyssp),
              &EQN_yyvs1, EQN_yysize * sizeof (*EQN_yyvsp),
              &EQN_yyls1, EQN_yysize * sizeof (*EQN_yylsp),
              &EQN_yystacksize);
      EQN_yyls = EQN_yyls1;
# else
      EQN_yyoverflow ("parser stack overflow",
              &EQN_yyss1, EQN_yysize * sizeof (*EQN_yyssp),
              &EQN_yyvs1, EQN_yysize * sizeof (*EQN_yyvsp),
              &EQN_yystacksize);
# endif
      EQN_yyss = EQN_yyss1;
      EQN_yyvs = EQN_yyvs1;
        }
#else /* no EQN_yyoverflow */
# ifndef YYSTACK_RELOCATE
        goto EQN_yyoverflowlab;
# else
        /* Extend the stack our own way.  */
        if (EQN_yystacksize >= YYMAXDEPTH)
            goto EQN_yyoverflowlab;
        EQN_yystacksize *= 2;
        if (EQN_yystacksize > YYMAXDEPTH)
            EQN_yystacksize = YYMAXDEPTH;

        {
            short             *EQN_yyss1 = EQN_yyss;
            union EQN_yyalloc *EQN_yyptr =
                                      (union EQN_yyalloc *) YYSTACK_ALLOC(YYSTACK_BYTES (EQN_yystacksize));
            if (!EQN_yyptr)
                goto EQN_yyoverflowlab;
            YYSTACK_RELOCATE (EQN_yyss);
            YYSTACK_RELOCATE (EQN_yyvs);
# if YYLSP_NEEDED
            YYSTACK_RELOCATE (EQN_yyls);
# endif
# undef YYSTACK_RELOCATE
            if (EQN_yyss1 != EQN_yyssa)
                YYSTACK_FREE (EQN_yyss1);
        }
# endif
#endif /* no EQN_yyoverflow */

        EQN_yyssp = EQN_yyss + EQN_yysize - 1;
        EQN_yyvsp = EQN_yyvs + EQN_yysize - 1;
#if YYLSP_NEEDED
        EQN_yylsp = EQN_yyls + EQN_yysize - 1;
#endif

        YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                (unsigned long int) EQN_yystacksize));

        if (EQN_yyssp >= EQN_yyss + EQN_yystacksize - 1)
            YYABORT;
    }

    YYDPRINTF ((stderr, "Entering state %d\n", EQN_yystate));

    goto EQN_yybackup;


/*-----------.
| EQN_yybackup.  |
`-----------*/
    EQN_yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* EQN_yyresume: */

    /* First try to decide what to do without reference to lookahead token.  */

    EQN_yyn = EQN_yypact[EQN_yystate];
    if (EQN_yyn == YYFLAG)
        goto EQN_yydefault;

    /* Not known => get a lookahead token if don't already have one.  */

    /* EQN_yychar is either YYEMPTY or YYEOF
       or a valid token in external form.  */

    if (EQN_yychar == YYEMPTY) {
        YYDPRINTF ((stderr, "Reading a token: "));
        EQN_yychar = YYLEX;
    }

    /* Convert token to internal form (in EQN_yychar1) for indexing tables with */

    if (EQN_yychar <= 0)        /* This means end of input. */
    {
        EQN_yychar1 = 0;
        EQN_yychar  = YYEOF;        /* Don't call YYLEX any more */

        YYDPRINTF ((stderr, "Now at end of input.\n"));
    } else {
        EQN_yychar1 = YYTRANSLATE (EQN_yychar);

#if YYDEBUG
        /* We have to keep this `#if YYDEBUG', since we use variables
       which are defined only if `YYDEBUG' is set.  */
         if (EQN_yydebug)
       {
         YYFPRINTF (stderr, "Next token is %d (%s",
                EQN_yychar, EQN_yytname[EQN_yychar1]);
         /* Give the individual parser a way to print the precise
            meaning of a token, for further debugging info.  */
# ifdef YYPRINT
         YYPRINT (stderr, EQN_yychar, EQN_yylval);
# endif
         YYFPRINTF (stderr, ")\n");
       }
#endif
    }

    EQN_yyn += EQN_yychar1;
    if (EQN_yyn < 0 || EQN_yyn > YYLAST || EQN_yycheck[EQN_yyn] != EQN_yychar1)
        goto EQN_yydefault;

    EQN_yyn = EQN_yytable[EQN_yyn];

    /* EQN_yyn is what to do for this token type in this state.
       Negative => reduce, -EQN_yyn is rule number.
       Positive => shift, EQN_yyn is new state.
         New state is final state => don't bother to shift,
         just return success.
       0, or most negative number => error.  */

    if (EQN_yyn < 0) {
        if (EQN_yyn == YYFLAG)
            goto EQN_yyerrlab;
        EQN_yyn = -EQN_yyn;
        goto EQN_yyreduce;
    } else if (EQN_yyn == 0)
        goto EQN_yyerrlab;

    if (EQN_yyn == YYFINAL)
        YYACCEPT;

    /* Shift the lookahead token.  */
    YYDPRINTF ((stderr, "Shifting token %d (%s), ",
            EQN_yychar, EQN_yytname[EQN_yychar1]));

    /* Discard the token being shifted unless it is eof.  */
    if (EQN_yychar != YYEOF)
        EQN_yychar = YYEMPTY;

    *++EQN_yyvsp = EQN_yylval;
#if YYLSP_NEEDED
    *++EQN_yylsp = EQN_yylloc;
#endif

    /* Count tokens shifted since error; after three, turn off error
       status.  */
    if (EQN_yyerrstatus)
        EQN_yyerrstatus--;

    EQN_yystate = EQN_yyn;
    goto EQN_yynewstate;


/*-----------------------------------------------------------.
| EQN_yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
    EQN_yydefault:
    EQN_yyn = EQN_yydefact[EQN_yystate];
    if (EQN_yyn == 0)
        goto EQN_yyerrlab;
    goto EQN_yyreduce;


/*-----------------------------.
| EQN_yyreduce -- Do a reduction.  |
`-----------------------------*/
    EQN_yyreduce:
    /* EQN_yyn is the number of a rule to reduce with.  */
    EQN_yylen = EQN_yyr2[EQN_yyn];

    /* If YYLEN is nonzero, implement the default value of the action:
       `$$ = $1'.

       Otherwise, the following line sets YYVAL to the semantic value of
       the lookahead token.  This behavior is undocumented and Bison
       users should not rely upon it.  Assigning to YYVAL
       unconditionally makes the parser a bit smaller, and it avoids a
       GCC warning that YYVAL may be used uninitialized.  */
    EQN_yyval = EQN_yyvsp[1 - EQN_yylen];

#if YYLSP_NEEDED
    /* Similarly for the default location.  Let the user run additional
       commands if for instance locations are ranges.  */
    EQN_yyloc = EQN_yylsp[1-EQN_yylen];
    YYLLOC_DEFAULT (EQN_yyloc, (EQN_yylsp - EQN_yylen), EQN_yylen);
#endif

#if YYDEBUG
    /* We have to keep this `#if YYDEBUG', since we use variables which
       are defined only if `YYDEBUG' is set.  */
    if (EQN_yydebug)
      {
        int EQN_yyi;

        YYFPRINTF (stderr, "Reducing via rule %d (line %d), ",
           EQN_yyn, EQN_yyrline[EQN_yyn]);

        /* Print the symbols being reduced, and their result.  */
        for (EQN_yyi = EQN_yyprhs[EQN_yyn]; EQN_yyrhs[EQN_yyi] > 0; EQN_yyi++)
      YYFPRINTF (stderr, "%s ", EQN_yytname[EQN_yyrhs[EQN_yyi]]);
        YYFPRINTF (stderr, " -> %s\n", EQN_yytname[EQN_yyr1[EQN_yyn]]);
      }
#endif

    switch (EQN_yyn) {

        case 4:
#line 98 "read_eqn.y"
        { do_assign(util_strsav("SILLY"), EQN_yyvsp[0].node); }
            break;
        case 8:
#line 107 "read_eqn.y"
        {
            network_set_name(global_network, EQN_yyvsp[0].strval);
            FREE(EQN_yyvsp[0].strval);
        }
            break;
        case 11:
#line 117 "read_eqn.y"
        { do_assign(EQN_yyvsp[-2].strval, EQN_yyvsp[0].node); }
            break;
        case 12:
#line 120 "read_eqn.y"
        { do_assign(EQN_yyvsp[-2].strval, EQN_yyvsp[-1].node); }
            break;
        case 13:
#line 125 "read_eqn.y"
        {
            EQN_yyval.node = node_or(EQN_yyvsp[-2].node, EQN_yyvsp[0].node);
            node_free(EQN_yyvsp[-2].node);
            node_free(EQN_yyvsp[0].node);
        }
            break;
        case 14:
#line 128 "read_eqn.y"
        {
            EQN_yyval.node = node_and(EQN_yyvsp[-2].node, EQN_yyvsp[0].node);
            node_free(EQN_yyvsp[-2].node);
            node_free(EQN_yyvsp[0].node);
        }
            break;
        case 15:
#line 131 "read_eqn.y"
        {
            EQN_yyval.node = node_and(EQN_yyvsp[-1].node, EQN_yyvsp[0].node);
            node_free(EQN_yyvsp[-1].node);
            node_free(EQN_yyvsp[0].node);
        }
            break;
        case 16:
#line 134 "read_eqn.y"
        {
            EQN_yyval.node = node_xor(EQN_yyvsp[-2].node, EQN_yyvsp[0].node);
            node_free(EQN_yyvsp[-2].node);
            node_free(EQN_yyvsp[0].node);
        }
            break;
        case 17:
#line 137 "read_eqn.y"
        {
            EQN_yyval.node = node_xnor(EQN_yyvsp[-2].node, EQN_yyvsp[0].node);
            node_free(EQN_yyvsp[-2].node);
            node_free(EQN_yyvsp[0].node);
        }
            break;
        case 18:
#line 140 "read_eqn.y"
        {
            EQN_yyval.node = node_not(EQN_yyvsp[-1].node);
            node_free(EQN_yyvsp[-1].node);
        }
            break;
        case 19:
#line 143 "read_eqn.y"
        {
            EQN_yyval.node = node_not(EQN_yyvsp[0].node);
            node_free(EQN_yyvsp[0].node);
        }
            break;
        case 20:
#line 146 "read_eqn.y"
        { EQN_yyval.node = node_constant(0); }
            break;
        case 21:
#line 149 "read_eqn.y"
        { EQN_yyval.node = node_constant(1); }
            break;
        case 22:
#line 152 "read_eqn.y"
        { EQN_yyval.node = EQN_yyvsp[-1].node; }
            break;
        case 23:
#line 155 "read_eqn.y"
        {
            node_t *node;
            node = read_find_or_create_node(global_network, EQN_yyvsp[0].strval);
            EQN_yyval.node = node_literal(node, 1);
            FREE(EQN_yyvsp[0].strval);
        }
            break;
        case 24:
#line 165 "read_eqn.y"
        { EQN_yyval.node = do_sexpr_list(EQN_yyvsp[-1].array, node_and); }
            break;
        case 25:
#line 168 "read_eqn.y"
        { EQN_yyval.node = do_sexpr_list(EQN_yyvsp[-1].array, node_or); }
            break;
        case 26:
#line 171 "read_eqn.y"
        {
            EQN_yyval.node = node_not(EQN_yyvsp[-1].node);
            node_free(EQN_yyvsp[-1].node);
        }
            break;
        case 27:
#line 174 "read_eqn.y"
        { EQN_yyval.node = node_constant(0); }
            break;
        case 28:
#line 177 "read_eqn.y"
        { EQN_yyval.node = node_constant(1); }
            break;
        case 29:
#line 180 "read_eqn.y"
        { EQN_yyval.node = node_constant(0); }
            break;
        case 30:
#line 183 "read_eqn.y"
        { EQN_yyval.node = node_constant(1); }
            break;
        case 31:
#line 186 "read_eqn.y"
        {
            node_t *node;
            node = read_find_or_create_node(global_network, EQN_yyvsp[0].strval);
            EQN_yyval.node = node_literal(node, 1);
            FREE(EQN_yyvsp[0].strval);
        }
            break;
        case 32:
#line 196 "read_eqn.y"
        {
            EQN_yyval.array = array_alloc(node_t * , 10);
            array_insert_last(node_t * , EQN_yyval.array, EQN_yyvsp[0].node);
        }
            break;
        case 33:
#line 201 "read_eqn.y"
        {
            array_insert_last(node_t * , EQN_yyvsp[-1].array, EQN_yyvsp[0].node);
        }
            break;
        case 34:
#line 207 "read_eqn.y"
        { EQN_yyval.strval = util_strsav(EQN_yytext); }
            break;
        case 36:
#line 213 "read_eqn.y"
        {
            node_t *node;
            char   errmsg[1024];
            node = read_find_or_create_node(global_network, EQN_yyvsp[0].strval);
            if (node_function(node) != NODE_UNDEFINED) {
                (void) sprintf(errmsg,
                               "Attempt to redefine '%s'\n", EQN_yyvsp[0].strval);
                EQN_yyerror(errmsg);   /* never returns */
            }
            network_change_node_type(global_network,
                                     node, PRIMARY_INPUT);
            FREE(EQN_yyvsp[0].strval);
        }
            break;
        case 38:
#line 231 "read_eqn.y"
        {
            node_t *node;
            node = read_find_or_create_node(global_network, EQN_yyvsp[0].strval);
            LS_ASSERT(lsNewEnd(po_list, (lsGeneric) node, LS_NH));
            FREE(EQN_yyvsp[0].strval);
        }
            break;
    }

#line 705 "/usr/share/bison/bison.simple"


    EQN_yyvsp -= EQN_yylen;
    EQN_yyssp -= EQN_yylen;
#if YYLSP_NEEDED
    EQN_yylsp -= EQN_yylen;
#endif

#if YYDEBUG
    if (EQN_yydebug)
      {
        short *EQN_yyssp1 = EQN_yyss - 1;
        YYFPRINTF (stderr, "state stack now");
        while (EQN_yyssp1 != EQN_yyssp)
      YYFPRINTF (stderr, " %d", *++EQN_yyssp1);
        YYFPRINTF (stderr, "\n");
      }
#endif

    *++EQN_yyvsp = EQN_yyval;
#if YYLSP_NEEDED
    *++EQN_yylsp = EQN_yyloc;
#endif

    /* Now `shift' the result of the reduction.  Determine what state
       that goes to, based on the state we popped back to and the rule
       number reduced by.  */

    EQN_yyn = EQN_yyr1[EQN_yyn];

    EQN_yystate     = EQN_yypgoto[EQN_yyn - YYNTBASE] + *EQN_yyssp;
    if (EQN_yystate >= 0 && EQN_yystate <= YYLAST && EQN_yycheck[EQN_yystate] == *EQN_yyssp)
        EQN_yystate = EQN_yytable[EQN_yystate];
    else
        EQN_yystate = EQN_yydefgoto[EQN_yyn - YYNTBASE];

    goto EQN_yynewstate;


/*------------------------------------.
| EQN_yyerrlab -- here on detecting error |
`------------------------------------*/
    EQN_yyerrlab:
    /* If not already recovering from an error, report this error.  */
    if (!EQN_yyerrstatus) {
        ++EQN_yynerrs;

#ifdef YYERROR_VERBOSE
        EQN_yyn = EQN_yypact[EQN_yystate];

        if (EQN_yyn > YYFLAG && EQN_yyn < YYLAST)
      {
        YYSIZE_T EQN_yysize = 0;
        char *EQN_yymsg;
        int EQN_yyx, EQN_yycount;

        EQN_yycount = 0;
        /* Start YYX at -YYN if negative to avoid negative indexes in
           YYCHECK.  */
        for (EQN_yyx = EQN_yyn < 0 ? -EQN_yyn : 0;
             EQN_yyx < (int) (sizeof (EQN_yytname) / sizeof (char *)); EQN_yyx++)
          if (EQN_yycheck[EQN_yyx + EQN_yyn] == EQN_yyx)
            EQN_yysize += EQN_yystrlen (EQN_yytname[EQN_yyx]) + 15, EQN_yycount++;
        EQN_yysize += EQN_yystrlen ("parse error, unexpected ") + 1;
        EQN_yysize += EQN_yystrlen (EQN_yytname[YYTRANSLATE (EQN_yychar)]);
        EQN_yymsg = (char *) YYSTACK_ALLOC (EQN_yysize);
        if (EQN_yymsg != 0)
          {
            char *EQN_yyp = EQN_yystpcpy (EQN_yymsg, "parse error, unexpected ");
            EQN_yyp = EQN_yystpcpy (EQN_yyp, EQN_yytname[YYTRANSLATE (EQN_yychar)]);

            if (EQN_yycount < 5)
          {
            EQN_yycount = 0;
            for (EQN_yyx = EQN_yyn < 0 ? -EQN_yyn : 0;
                 EQN_yyx < (int) (sizeof (EQN_yytname) / sizeof (char *));
                 EQN_yyx++)
              if (EQN_yycheck[EQN_yyx + EQN_yyn] == EQN_yyx)
                {
              const char *EQN_yyq = ! EQN_yycount ? ", expecting " : " or ";
              EQN_yyp = EQN_yystpcpy (EQN_yyp, EQN_yyq);
              EQN_yyp = EQN_yystpcpy (EQN_yyp, EQN_yytname[EQN_yyx]);
              EQN_yycount++;
                }
          }
            EQN_yyerror (EQN_yymsg);
            YYSTACK_FREE (EQN_yymsg);
          }
        else
          EQN_yyerror ("parse error; also virtual memory exhausted");
      }
        else
#endif /* defined (YYERROR_VERBOSE) */
        EQN_yyerror("parse error");
    }
    goto EQN_yyerrlab1;


/*--------------------------------------------------.
| EQN_yyerrlab1 -- error raised explicitly by an action |
`--------------------------------------------------*/
    EQN_yyerrlab1:
    if (EQN_yyerrstatus == 3) {
        /* If just tried and failed to reuse lookahead token after an
       error, discard it.  */

        /* return failure if at end of input */
        if (EQN_yychar == YYEOF)
            YYABORT;
        YYDPRINTF ((stderr, "Discarding token %d (%s).\n",
                EQN_yychar, EQN_yytname[EQN_yychar1]));
        EQN_yychar = YYEMPTY;
    }

    /* Else will try to reuse lookahead token after shifting the error
       token.  */

    EQN_yyerrstatus = 3;        /* Each real token shifted decrements this */

    goto EQN_yyerrhandle;


/*-------------------------------------------------------------------.
| EQN_yyerrdefault -- current state does not do anything special for the |
| error token.                                                       |
`-------------------------------------------------------------------*/
    EQN_yyerrdefault:
#if 0
    /* This is wrong; only states that explicitly want error tokens
       should shift them.  */

    /* If its default is to accept any token, ok.  Otherwise pop it.  */
    EQN_yyn = EQN_yydefact[EQN_yystate];
    if (EQN_yyn)
      goto EQN_yydefault;
#endif


/*---------------------------------------------------------------.
| EQN_yyerrpop -- pop the current state because it cannot handle the |
| error token                                                    |
`---------------------------------------------------------------*/
    EQN_yyerrpop:
    if (EQN_yyssp == EQN_yyss)
        YYABORT;
    EQN_yyvsp--;
    EQN_yystate = *--EQN_yyssp;
#if YYLSP_NEEDED
    EQN_yylsp--;
#endif

#if YYDEBUG
    if (EQN_yydebug)
      {
        short *EQN_yyssp1 = EQN_yyss - 1;
        YYFPRINTF (stderr, "Error: state stack now");
        while (EQN_yyssp1 != EQN_yyssp)
      YYFPRINTF (stderr, " %d", *++EQN_yyssp1);
        YYFPRINTF (stderr, "\n");
      }
#endif

/*--------------.
| EQN_yyerrhandle.  |
`--------------*/
    EQN_yyerrhandle:
    EQN_yyn = EQN_yypact[EQN_yystate];
    if (EQN_yyn == YYFLAG)
        goto EQN_yyerrdefault;

    EQN_yyn += YYTERROR;
    if (EQN_yyn < 0 || EQN_yyn > YYLAST || EQN_yycheck[EQN_yyn] != YYTERROR)
        goto EQN_yyerrdefault;

    EQN_yyn = EQN_yytable[EQN_yyn];
    if (EQN_yyn < 0) {
        if (EQN_yyn == YYFLAG)
            goto EQN_yyerrpop;
        EQN_yyn = -EQN_yyn;
        goto EQN_yyreduce;
    } else if (EQN_yyn == 0)
        goto EQN_yyerrpop;

    if (EQN_yyn == YYFINAL)
        YYACCEPT;

    YYDPRINTF ((stderr, "Shifting error token, "));

    *++EQN_yyvsp = EQN_yylval;
#if YYLSP_NEEDED
    *++EQN_yylsp = EQN_yylloc;
#endif

    EQN_yystate = EQN_yyn;
    goto EQN_yynewstate;


/*-------------------------------------.
| EQN_yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
    EQN_yyacceptlab:
    EQN_yyresult = 0;
    goto EQN_yyreturn;

/*-----------------------------------.
| EQN_yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
    EQN_yyabortlab:
    EQN_yyresult = 1;
    goto EQN_yyreturn;

/*---------------------------------------------.
| EQN_yyoverflowab -- parser overflow comes here.  |
`---------------------------------------------*/
    EQN_yyoverflowlab:
    EQN_yyerror("parser stack overflow");
    EQN_yyresult = 2;
    /* Fall through.  */

    EQN_yyreturn:
#ifndef EQN_yyoverflow
    if (EQN_yyss != EQN_yyssa)
        YYSTACK_FREE (EQN_yyss);
#endif
    return EQN_yyresult;
}

#line 239 "read_eqn.y"


static jmp_buf jmpbuf;

int
EQN_yyerror(errmsg)
        char *errmsg;
{
    read_error(errmsg);
    longjmp(jmpbuf, 1);
}


network_t *
read_eqn(fp)
        FILE *fp;
{
    error_init();

    if (setjmp(jmpbuf)) {
        /* syntax error -- return from EQN_yyerror() */
        LS_ASSERT(lsDestroy(po_list, (void (*)()) 0));
        network_free(global_network);
        return 0;

    } else {
        global_network = network_alloc();
        read_filename_to_netname(global_network, read_filename);
        po_list = lsCreate();
        equation_setup_file(fp);
        (void) EQN_yyparse();

        if (!read_check_io_list(global_network, po_list)) {
            network_free(global_network);
            return 0;
        }
        read_hack_outputs(global_network, po_list);
        LS_ASSERT(lsDestroy(po_list, (void (*)()) 0));

        if (!network_is_acyclic(global_network)) {
            network_free(global_network);
            return 0;
        }

        return global_network;
    }
}

network_t *
read_eqn_string(s)
        char *s;
{
    error_init();

    if (setjmp(jmpbuf)) {
        /* syntax error -- return from EQN_yyerror() */
        LS_ASSERT(lsDestroy(po_list, (void (*)()) 0));
        network_free(global_network);
        return 0;

    } else {
        global_network = network_alloc();
        po_list        = lsCreate();
        equation_setup_string(s);
        (void) EQN_yyparse();

        if (!read_check_io_list(global_network, po_list)) {
            network_free(global_network);
            return 0;
        }
        read_hack_outputs(global_network, po_list);
        LS_ASSERT(lsDestroy(po_list, (void (*)()) 0));

        if (!network_is_acyclic(global_network)) {
            network_free(global_network);
            return 0;
        }

        return global_network;
    }
}
