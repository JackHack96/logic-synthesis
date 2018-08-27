/* A Bison parser, made from readlib.y
   by GNU bison 1.35.  */

#define YYBISON 1 /* Identify Bison output.  */

#define OPR_OR 257
#define OPR_AND 258
#define CONST1 259
#define CONST0 260
#define IDENTIFIER 261
#define LPAREN 262
#define REAL 263
#define OPR_NOT 264
#define OPR_NOT_POST 265
#define GATE 266
#define PIN 267
#define SEMI 268
#define ASSIGN 269
#define RPAREN 270
#define LATCH 271
#define CONTROL 272
#define CONSTRAINT 273
#define SEQ 274

#line 10 "readlib.y"

/* file @(#)readlib.y	1.2                      */
/* last modified on 6/13/91 at 17:46:40   */
#include "genlib_int.h"
#include "config.h"
#include <setjmp.h>

#undef GENLIB_yywrap

static int input();

static int unput();

static int GENLIB_yywrap();

FILE *gl_out_file;
#if YYTEXT_POINTER
extern char *GENLIB_yytext;
#else
extern char GENLIB_yytext[];
#endif

static int global_use_nor;
static function_t *global_fct;

extern int library_setup_string(char *);

extern int library_setup_file(FILE *, char *);

#line 37 "readlib.y"
#ifndef YYSTYPE
typedef union {
  tree_node_t *nodeval;
  char *strval;
  double realval;
  pin_info_t *pinval;
  function_t *functionval;
  latch_info_t *latchval;
  constraint_info_t *constrval;
} GENLIB_yystype;
#define YYSTYPE GENLIB_yystype
#define YYSTYPE_IS_TRIVIAL 1
#endif
#ifndef YYDEBUG
#define YYDEBUG 0
#endif

#define YYFINAL 71
#define YYFLAG -32768
#define YYNTBASE 21

/* YYTRANSLATE(YYLEX) -- Bison token number corresponding to YYLEX. */
#define YYTRANSLATE(x) ((unsigned)(x) <= 274 ? GENLIB_yytranslate[x] : 37)

/* YYTRANSLATE[YYLEX] -- Bison token number corresponding to YYLEX. */
static const char GENLIB_yytranslate[] = {
    0,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 2, 2, 2, 1, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

#if YYDEBUG
static const short GENLIB_yyprhs[] = {
    0,  0,  2,  4,  5,  8,  14, 23, 24, 27, 37, 39, 41, 43, 48,  52, 56,
    59, 63, 66, 69, 71, 73, 75, 77, 79, 81, 82, 91, 92, 95, 100, 105};
static const short GENLIB_yyrhs[] = {
    22, 0,  28, 0,  0, 22, 23, 0,  12, 30, 31, 28, 24, 0,  17, 30, 31, 28,
    24, 35, 32, 33, 0, 0,  24, 25, 0,  13, 27, 26, 31, 31, 31, 31, 31, 31,
    0,  30, 0,  30, 0, 4,  0,  30, 15, 29, 14, 0,  29, 3,  29, 0,  29, 4,
    29, 0,  29, 29, 0, 8,  29, 16, 0,  10, 29, 0,  29, 11, 0,  30, 0,  6,
    0,  5,  0,  7,  0, 9,  0,  9,  0,  0,  18, 30, 31, 31, 31, 31, 31, 31,
    0,  0,  33, 34, 0, 19, 27, 31, 31, 0,  20, 30, 30, 36, 0,  7,  0};

#endif

#if YYDEBUG
/* YYRLINE[YYN] -- source line where rule number YYN was defined. */
static const short GENLIB_yyrline[] = {
    0,   65,  66,  70,  71,  74,  79,  86,  88,  92,  104,
    117, 119, 123, 132, 140, 148, 156, 160, 165, 170, 177,
    183, 191, 193, 198, 202, 204, 218, 220, 227, 237, 246};
#endif

#if (YYDEBUG) || defined YYERROR_VERBOSE

/* YYTNAME[TOKEN_NUM] -- String name of the token TOKEN_NUM. */
static const char *const GENLIB_yytname[] = {"$",           "error",
                                             "$undefined.", "OPR_OR",
                                             "OPR_AND",     "CONST1",
                                             "CONST0",      "IDENTIFIER",
                                             "LPAREN",      "REAL",
                                             "OPR_NOT",     "OPR_NOT_POST",
                                             "GATE",        "PIN",
                                             "SEMI",        "ASSIGN",
                                             "RPAREN",      "LATCH",
                                             "CONTROL",     "CONSTRAINT",
                                             "SEQ",         "hack",
                                             "gates",       "gate_info",
                                             "pins",        "pin_info",
                                             "pin_phase",   "pin_name",
                                             "function",    "expr",
                                             "name",        "real",
                                             "clock",       "constraints",
                                             "constraint",  "latch",
                                             "type",        0};
#endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives. */
static const short GENLIB_yyr1[] = {0,  21, 21, 22, 22, 23, 23, 24, 24, 25, 26,
                                    27, 27, 28, 29, 29, 29, 29, 29, 29, 29, 29,
                                    29, 30, 30, 31, 32, 32, 33, 33, 34, 35, 36};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN. */
static const short GENLIB_yyr2[] = {0, 1, 1, 0, 2, 5, 8, 0, 2, 9, 1,
                                    1, 1, 4, 3, 3, 2, 3, 2, 2, 1, 1,
                                    1, 1, 1, 1, 0, 8, 0, 2, 4, 4, 1};

/* YYDEFACT[S] -- default rule to reduce with in state S when YYTABLE
   doesn't specify something else to do.  Zero means the default is an
   error. */
static const short GENLIB_yydefact[] = {
    3,  23, 24, 1,  2,  0,  0, 0,  4,  0,  0,  0, 22, 21, 0,  0,  0,  20,
    25, 0,  0,  0,  18, 0,  0, 19, 13, 16, 7,  7, 17, 14, 15, 5,  0,  0,
    8,  0,  26, 12, 0,  11, 0, 0,  28, 0,  10, 0, 0,  6,  0,  32, 31, 0,
    0,  29, 0,  0,  0,  0,  0, 0,  0,  0,  30, 0, 0,  9,  27, 0,  0,  0};

static const short GENLIB_yydefgoto[] = {69, 3,  8,  33, 36, 45, 40, 4,
                                         27, 17, 19, 44, 49, 55, 38, 52};

static const short GENLIB_yypact[] = {
    -5,     -32768, -32768, -3,     -32768, 8,      -5,     -5,     -32768,
    94,     -4,     -4,     -32768, -32768, 94,     94,     76,     -32768,
    -32768, -5,     -5,     62,     -32768, 94,     94,     -32768, -32768,
    7,      -32768, -32768, -32768, 87,     7,      -1,     -10,    4,
    -32768, -5,     6,      -32768, -5,     -32768, -5,     -5,     -32768,
    -4,     -32768, 15,     -4,     10,     -4,     -32768, -32768, -4,
    4,      -32768, -4,     -4,     -4,     -4,     -4,     -4,     -4,
    -4,     -32768, -4,     -4,     -32768, -32768, 25,     26,     -32768};

static const short GENLIB_yypgoto[] = {
    -32768, -32768, -32768, 1,      -32768, -32768, -23,    0,
    -8,     21,     -11,    -32768, -32768, -32768, -32768, -32768};

#define YYLAST 104

static const short GENLIB_yytable[] = {
    20, 16, 1,  35, 2,  18, 21, 22, 39, 6,  37, 1,  35, 2,  7,  31, 32, 15,
    25, 28, 29, 5,  51, 9,  43, 70, 71, 10, 11, 54, 34, 58, 0,  0,  50, 0,
    0,  53, 0,  56, 5,  5,  57, 0,  0,  59, 60, 61, 62, 63, 64, 65, 66, 0,
    67, 68, 41, 0,  42, 0,  0,  46, 0,  47, 48, 23, 24, 12, 13, 1,  14, 2,
    15, 25, 0,  41, 0,  0,  30, 23, 24, 12, 13, 1,  14, 2,  15, 25, 0,  0,
    26, 24, 12, 13, 1,  14, 2,  15, 25, 12, 13, 1,  14, 2,  15};

static const short GENLIB_yycheck[] = {
    11, 9,  7,  13, 9,  9,  14, 15, 4,  12, 20, 7,  13, 9,  17, 23, 24, 10,
    11, 19, 20, 0,  7,  15, 18, 0,  0,  6,  7,  19, 29, 54, -1, -1, 45, -1,
    -1, 48, -1, 50, 19, 20, 53, -1, -1, 56, 57, 58, 59, 60, 61, 62, 63, -1,
    65, 66, 35, -1, 37, -1, -1, 40, -1, 42, 43, 3,  4,  5,  6,  7,  8,  9,
    10, 11, -1, 54, -1, -1, 16, 3,  4,  5,  6,  7,  8,  9,  10, 11, -1, -1,
    14, 4,  5,  6,  7,  8,  9,  10, 11, 5,  6,  7,  8,  9,  10};
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

/* All symbols defined below should begin with GENLIB_yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

#if !defined(GENLIB_yyoverflow) || defined(YYERROR_VERBOSE)

/* The parser invokes alloca or malloc; define the necessary symbols.  */

#if YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#ifndef YYSTACK_USE_ALLOCA
#if defined(alloca) || defined(_ALLOCA_H)
#define YYSTACK_ALLOC alloca
#else
#ifdef __GNUC__
#define YYSTACK_ALLOC __builtin_alloca
#endif
#endif
#endif
#endif

#ifdef YYSTACK_ALLOC
/* Pacify GCC's `empty if-body' warning. */
#define YYSTACK_FREE(Ptr)                                                      \
  do { /* empty */                                                             \
    ;                                                                          \
  } while (0)
#else
#if defined(__STDC__) || defined(__cplusplus)
#include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#define YYSIZE_T size_t
#endif
#define YYSTACK_ALLOC malloc
#define YYSTACK_FREE free
#endif
#endif /* ! defined (GENLIB_yyoverflow) || defined (YYERROR_VERBOSE) */

#if (!defined(GENLIB_yyoverflow) &&                                            \
     (!defined(__cplusplus) || (YYLTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union GENLIB_yyalloc {
  short GENLIB_yyss;
  YYSTYPE GENLIB_yyvs;
#if YYLSP_NEEDED
  YYLTYPE GENLIB_yyls;
#endif
};

/* The size of the maximum gap between one aligned stack and the next.  */
#define YYSTACK_GAP_MAX (sizeof(union GENLIB_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
#if YYLSP_NEEDED
#define YYSTACK_BYTES(N)                                                       \
  ((N) * (sizeof(short) + sizeof(YYSTYPE) + sizeof(YYLTYPE)) +                 \
   2 * YYSTACK_GAP_MAX)
#else
#define YYSTACK_BYTES(N)                                                       \
  ((N) * (sizeof(short) + sizeof(YYSTYPE)) + YYSTACK_GAP_MAX)
#endif

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
#ifndef YYCOPY
#if 1 < __GNUC__
#define YYCOPY(To, From, Count)                                                \
  __builtin_memcpy(To, From, (Count) * sizeof(*(From)))
#else
#define YYCOPY(To, From, Count)                                                \
  do {                                                                         \
    register YYSIZE_T GENLIB_yyi;                                              \
    for (GENLIB_yyi = 0; GENLIB_yyi < (Count); GENLIB_yyi++)                   \
      (To)[GENLIB_yyi] = (From)[GENLIB_yyi];                                   \
  } while (0)
#endif
#endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
#define YYSTACK_RELOCATE(Stack)                                                \
  do {                                                                         \
    YYSIZE_T GENLIB_yynewbytes;                                                \
    YYCOPY(&GENLIB_yyptr->Stack, Stack, GENLIB_yysize);                        \
    Stack = &GENLIB_yyptr->Stack;                                              \
    GENLIB_yynewbytes = GENLIB_yystacksize * sizeof(*Stack) + YYSTACK_GAP_MAX; \
    GENLIB_yyptr += GENLIB_yynewbytes / sizeof(*GENLIB_yyptr);                 \
  } while (0)

#endif

#if !defined(YYSIZE_T) && defined(__SIZE_TYPE__)
#define YYSIZE_T __SIZE_TYPE__
#endif
#if !defined(YYSIZE_T) && defined(size_t)
#define YYSIZE_T size_t
#endif
#if !defined(YYSIZE_T)
#if defined(__STDC__) || defined(__cplusplus)
#include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#define YYSIZE_T size_t
#endif
#endif
#if !defined(YYSIZE_T)
#define YYSIZE_T unsigned int
#endif

#define GENLIB_yyerrok (GENLIB_yyerrstatus = 0)
#define GENLIB_yyclearin (GENLIB_yychar = YYEMPTY)
#define YYEMPTY -2
#define YYEOF 0
#define YYACCEPT goto GENLIB_yyacceptlab
#define YYABORT goto GENLIB_yyabortlab
#define YYERROR goto GENLIB_yyerrlab1
/* Like YYERROR except do call GENLIB_yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL goto GENLIB_yyerrlab
#define YYRECOVERING() (!!GENLIB_yyerrstatus)
#define YYBACKUP(Token, Value)                                                 \
  do                                                                           \
    if (GENLIB_yychar == YYEMPTY && GENLIB_yylen == 1) {                       \
      GENLIB_yychar = (Token);                                                 \
      GENLIB_yylval = (Value);                                                 \
      GENLIB_yychar1 = YYTRANSLATE(GENLIB_yychar);                             \
      YYPOPSTACK;                                                              \
      goto GENLIB_yybackup;                                                    \
    } else {                                                                   \
      GENLIB_yyerror("syntax error: cannot back up");                          \
      YYERROR;                                                                 \
    }                                                                          \
  while (0)

#define YYTERROR 1
#define YYERRCODE 256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).

   When YYLLOC_DEFAULT is run, CURRENT is set the location of the
   first token.  By default, to implement support for ranges, extend
   its range to the last symbol.  */

#ifndef YYLLOC_DEFAULT
#define YYLLOC_DEFAULT(Current, Rhs, N)                                        \
  Current.last_line = Rhs[N].last_line;                                        \
  Current.last_column = Rhs[N].last_column;
#endif

/* YYLEX -- calling `GENLIB_yylex' with the right arguments.  */

#if YYPURE
#if YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX GENLIB_yylex(&GENLIB_yylval, &GENLIB_yylloc, YYLEX_PARAM)
#else
#define YYLEX GENLIB_yylex(&GENLIB_yylval, &GENLIB_yylloc)
#endif
#else /* !YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX GENLIB_yylex(&GENLIB_yylval, YYLEX_PARAM)
#else
#define YYLEX GENLIB_yylex(&GENLIB_yylval)
#endif
#endif /* !YYLSP_NEEDED */
#else  /* !YYPURE */
#define YYLEX GENLIB_yylex()
#endif /* !YYPURE */

/* Enable debugging if requested.  */
#if YYDEBUG

#ifndef YYFPRINTF
#include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#define YYFPRINTF fprintf
#endif

#define YYDPRINTF(Args)                                                        \
  do {                                                                         \
    if (GENLIB_yydebug)                                                        \
      YYFPRINTF Args;                                                          \
  } while (0)
/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int GENLIB_yydebug;
#else /* !YYDEBUG */
#define YYDPRINTF(Args)
#endif /* !YYDEBUG */

/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
#define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

#ifdef YYERROR_VERBOSE

#ifndef GENLIB_yystrlen
#if defined(__GLIBC__) && defined(_STRING_H)
#define GENLIB_yystrlen strlen
#else
/* Return the length of YYSTR.  */
static YYSIZE_T
#if defined(__STDC__) || defined(__cplusplus)
GENLIB_yystrlen(const char *GENLIB_yystr)
#else
    GENLIB_yystrlen(GENLIB_yystr) const char *GENLIB_yystr;
#endif
{
  register const char *GENLIB_yys = GENLIB_yystr;

  while (*GENLIB_yys++ != '\0')
    continue;

  return GENLIB_yys - GENLIB_yystr - 1;
}
#endif
#endif

#ifndef GENLIB_yystpcpy
#if defined(__GLIBC__) && defined(_STRING_H) && defined(_GNU_SOURCE)
#define GENLIB_yystpcpy stpcpy
#else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#if defined(__STDC__) || defined(__cplusplus)
GENLIB_yystpcpy(char *GENLIB_yydest, const char *GENLIB_yysrc)
#else
    GENLIB_yystpcpy(GENLIB_yydest, GENLIB_yysrc) char *GENLIB_yydest;
const char *GENLIB_yysrc;
#endif
{
  register char *GENLIB_yyd = GENLIB_yydest;
  register const char *GENLIB_yys = GENLIB_yysrc;

  while ((*GENLIB_yyd++ = *GENLIB_yys++) != '\0')
    continue;

  return GENLIB_yyd - 1;
}
#endif
#endif
#endif

#line 315 "/usr/share/bison/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into GENLIB_yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#if defined(__STDC__) || defined(__cplusplus)
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif
#else /* !YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* !YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int GENLIB_yyparse(void *);
#else

int GENLIB_yyparse(void);

#endif
#endif

/* YY_DECL_VARIABLES -- depending whether we use a pure parser,
   variables are global, or local to YYPARSE.  */

#define YY_DECL_NON_LSP_VARIABLES                                              \
  /* The lookahead symbol.  */                                                 \
  int GENLIB_yychar;                                                           \
                                                                               \
  /* The semantic value of the lookahead symbol. */                            \
  YYSTYPE GENLIB_yylval;                                                       \
                                                                               \
  /* Number of parse errors so far.  */                                        \
  int GENLIB_yynerrs;

#if YYLSP_NEEDED
#define YY_DECL_VARIABLES                                                      \
  YY_DECL_NON_LSP_VARIABLES                                                    \
                                                                               \
  /* Location data for the lookahead symbol.  */                               \
  YYLTYPE GENLIB_yylloc;
#else
#define YY_DECL_VARIABLES YY_DECL_NON_LSP_VARIABLES
#endif

/* If nonreentrant, generate the variables here. */

#if !YYPURE
YY_DECL_VARIABLES
#endif /* !YYPURE */

int GENLIB_yyparse(YYPARSE_PARAM_ARG) YYPARSE_PARAM_DECL {
  /* If reentrant, generate the variables here. */
#if YYPURE
  YY_DECL_VARIABLES
#endif /* !YYPURE */

  register int GENLIB_yystate;
  register int GENLIB_yyn;
  int GENLIB_yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int GENLIB_yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int GENLIB_yychar1 = 0;

  /* Three stacks and their tools:
     `GENLIB_yyss': related to states,
     `GENLIB_yyvs': related to semantic values,
     `GENLIB_yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow GENLIB_yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack. */
  short GENLIB_yyssa[YYINITDEPTH];
  short *GENLIB_yyss = GENLIB_yyssa;
  register short *GENLIB_yyssp;

  /* The semantic value stack.  */
  YYSTYPE GENLIB_yyvsa[YYINITDEPTH];
  YYSTYPE *GENLIB_yyvs = GENLIB_yyvsa;
  register YYSTYPE *GENLIB_yyvsp;

#if YYLSP_NEEDED
  /* The location stack.  */
  YYLTYPE GENLIB_yylsa[YYINITDEPTH];
  YYLTYPE *GENLIB_yyls = GENLIB_yylsa;
  YYLTYPE *GENLIB_yylsp;
#endif

#if YYLSP_NEEDED
#define YYPOPSTACK (GENLIB_yyvsp--, GENLIB_yyssp--, GENLIB_yylsp--)
#else
#define YYPOPSTACK (GENLIB_yyvsp--, GENLIB_yyssp--)
#endif

  YYSIZE_T GENLIB_yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE GENLIB_yyval;
#if YYLSP_NEEDED
  YYLTYPE GENLIB_yyloc;
#endif

  /* When reducing, the number of symbols on the RHS of the reduced
     rule. */
  int GENLIB_yylen;

  YYDPRINTF((stderr, "Starting parse\n"));

  GENLIB_yystate = 0;
  GENLIB_yyerrstatus = 0;
  GENLIB_yynerrs = 0;
  GENLIB_yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  GENLIB_yyssp = GENLIB_yyss;
  GENLIB_yyvsp = GENLIB_yyvs;
#if YYLSP_NEEDED
  GENLIB_yylsp = GENLIB_yyls;
#endif
  goto GENLIB_yysetstate;

  /*------------------------------------------------------------.
  | GENLIB_yynewstate -- Push a new state, which is found in GENLIB_yystate.  |
  `------------------------------------------------------------*/
GENLIB_yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  GENLIB_yyssp++;

GENLIB_yysetstate:
  *GENLIB_yyssp = GENLIB_yystate;

  if (GENLIB_yyssp >= GENLIB_yyss + GENLIB_yystacksize - 1) {
    /* Get the current used size of the three stacks, in elements.  */
    YYSIZE_T GENLIB_yysize = GENLIB_yyssp - GENLIB_yyss + 1;

#ifdef GENLIB_yyoverflow
    {
      /* Give user a chance to reallocate the stack. Use copies of
         these so that the &'s don't force the real ones into
         memory.  */
      YYSTYPE *GENLIB_yyvs1 = GENLIB_yyvs;
      short *GENLIB_yyss1 = GENLIB_yyss;

      /* Each stack pointer address is followed by the size of the
         data in use in that stack, in bytes.  */
#if YYLSP_NEEDED
      YYLTYPE *GENLIB_yyls1 = GENLIB_yyls;
      /* This used to be a conditional around just the two extra args,
         but that might be undefined if GENLIB_yyoverflow is a macro.  */
      GENLIB_yyoverflow("parser stack overflow", &GENLIB_yyss1,
                        GENLIB_yysize * sizeof(*GENLIB_yyssp), &GENLIB_yyvs1,
                        GENLIB_yysize * sizeof(*GENLIB_yyvsp), &GENLIB_yyls1,
                        GENLIB_yysize * sizeof(*GENLIB_yylsp),
                        &GENLIB_yystacksize);
      GENLIB_yyls = GENLIB_yyls1;
#else
      GENLIB_yyoverflow("parser stack overflow", &GENLIB_yyss1,
                        GENLIB_yysize * sizeof(*GENLIB_yyssp), &GENLIB_yyvs1,
                        GENLIB_yysize * sizeof(*GENLIB_yyvsp),
                        &GENLIB_yystacksize);
#endif
      GENLIB_yyss = GENLIB_yyss1;
      GENLIB_yyvs = GENLIB_yyvs1;
    }
#else /* no GENLIB_yyoverflow */
#ifndef YYSTACK_RELOCATE
    goto GENLIB_yyoverflowlab;
#else
    /* Extend the stack our own way.  */
    if (GENLIB_yystacksize >= YYMAXDEPTH)
      goto GENLIB_yyoverflowlab;
    GENLIB_yystacksize *= 2;
    if (GENLIB_yystacksize > YYMAXDEPTH)
      GENLIB_yystacksize = YYMAXDEPTH;

    {
      short *GENLIB_yyss1 = GENLIB_yyss;
      union GENLIB_yyalloc *GENLIB_yyptr =
          (union GENLIB_yyalloc *)YYSTACK_ALLOC(
              YYSTACK_BYTES(GENLIB_yystacksize));
      if (!GENLIB_yyptr)
        goto GENLIB_yyoverflowlab;
      YYSTACK_RELOCATE(GENLIB_yyss);
      YYSTACK_RELOCATE(GENLIB_yyvs);
#if YYLSP_NEEDED
      YYSTACK_RELOCATE(GENLIB_yyls);
#endif
#undef YYSTACK_RELOCATE
      if (GENLIB_yyss1 != GENLIB_yyssa)
        YYSTACK_FREE(GENLIB_yyss1);
    }
#endif
#endif /* no GENLIB_yyoverflow */

    GENLIB_yyssp = GENLIB_yyss + GENLIB_yysize - 1;
    GENLIB_yyvsp = GENLIB_yyvs + GENLIB_yysize - 1;
#if YYLSP_NEEDED
    GENLIB_yylsp = GENLIB_yyls + GENLIB_yysize - 1;
#endif

    YYDPRINTF((stderr, "Stack size increased to %lu\n",
               (unsigned long int)GENLIB_yystacksize));

    if (GENLIB_yyssp >= GENLIB_yyss + GENLIB_yystacksize - 1)
      YYABORT;
  }

  YYDPRINTF((stderr, "Entering state %d\n", GENLIB_yystate));

  goto GENLIB_yybackup;

  /*-----------.
  | GENLIB_yybackup.  |
  `-----------*/
GENLIB_yybackup:

  /* Do appropriate processing given the current state.  */
  /* Read a lookahead token if we need one and don't already have one.  */
  /* GENLIB_yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  GENLIB_yyn = GENLIB_yypact[GENLIB_yystate];
  if (GENLIB_yyn == YYFLAG)
    goto GENLIB_yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* GENLIB_yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (GENLIB_yychar == YYEMPTY) {
    YYDPRINTF((stderr, "Reading a token: "));
    GENLIB_yychar = YYLEX;
  }

  /* Convert token to internal form (in GENLIB_yychar1) for indexing tables with
   */

  if (GENLIB_yychar <= 0) /* This means end of input. */
  {
    GENLIB_yychar1 = 0;
    GENLIB_yychar = YYEOF; /* Don't call YYLEX any more */

    YYDPRINTF((stderr, "Now at end of input.\n"));
  } else {
    GENLIB_yychar1 = YYTRANSLATE(GENLIB_yychar);

#if YYDEBUG
    /* We have to keep this `#if YYDEBUG', since we use variables
   which are defined only if `YYDEBUG' is set.  */
    if (GENLIB_yydebug) {
      YYFPRINTF(stderr, "Next token is %d (%s", GENLIB_yychar,
                GENLIB_yytname[GENLIB_yychar1]);
      /* Give the individual parser a way to print the precise
         meaning of a token, for further debugging info.  */
#ifdef YYPRINT
      YYPRINT(stderr, GENLIB_yychar, GENLIB_yylval);
#endif
      YYFPRINTF(stderr, ")\n");
    }
#endif
  }

  GENLIB_yyn += GENLIB_yychar1;
  if (GENLIB_yyn < 0 || GENLIB_yyn > YYLAST ||
      GENLIB_yycheck[GENLIB_yyn] != GENLIB_yychar1)
    goto GENLIB_yydefault;

  GENLIB_yyn = GENLIB_yytable[GENLIB_yyn];

  /* GENLIB_yyn is what to do for this token type in this state.
     Negative => reduce, -GENLIB_yyn is rule number.
     Positive => shift, GENLIB_yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (GENLIB_yyn < 0) {
    if (GENLIB_yyn == YYFLAG)
      goto GENLIB_yyerrlab;
    GENLIB_yyn = -GENLIB_yyn;
    goto GENLIB_yyreduce;
  } else if (GENLIB_yyn == 0)
    goto GENLIB_yyerrlab;

  if (GENLIB_yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF((stderr, "Shifting token %d (%s), ", GENLIB_yychar,
             GENLIB_yytname[GENLIB_yychar1]));

  /* Discard the token being shifted unless it is eof.  */
  if (GENLIB_yychar != YYEOF)
    GENLIB_yychar = YYEMPTY;

  *++GENLIB_yyvsp = GENLIB_yylval;
#if YYLSP_NEEDED
  *++GENLIB_yylsp = GENLIB_yylloc;
#endif

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (GENLIB_yyerrstatus)
    GENLIB_yyerrstatus--;

  GENLIB_yystate = GENLIB_yyn;
  goto GENLIB_yynewstate;

  /*-----------------------------------------------------------.
  | GENLIB_yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
GENLIB_yydefault:
  GENLIB_yyn = GENLIB_yydefact[GENLIB_yystate];
  if (GENLIB_yyn == 0)
    goto GENLIB_yyerrlab;
  goto GENLIB_yyreduce;

  /*-----------------------------.
  | GENLIB_yyreduce -- Do a reduction.  |
  `-----------------------------*/
GENLIB_yyreduce:
  /* GENLIB_yyn is the number of a rule to reduce with.  */
  GENLIB_yylen = GENLIB_yyr2[GENLIB_yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to the semantic value of
     the lookahead token.  This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  GENLIB_yyval = GENLIB_yyvsp[1 - GENLIB_yylen];

#if YYLSP_NEEDED
  /* Similarly for the default location.  Let the user run additional
     commands if for instance locations are ranges.  */
  GENLIB_yyloc = GENLIB_yylsp[1 - GENLIB_yylen];
  YYLLOC_DEFAULT(GENLIB_yyloc, (GENLIB_yylsp - GENLIB_yylen), GENLIB_yylen);
#endif

#if YYDEBUG
  /* We have to keep this `#if YYDEBUG', since we use variables which
     are defined only if `YYDEBUG' is set.  */
  if (GENLIB_yydebug) {
    int GENLIB_yyi;

    YYFPRINTF(stderr, "Reducing via rule %d (line %d), ", GENLIB_yyn,
              GENLIB_yyrline[GENLIB_yyn]);

    /* Print the symbols being reduced, and their result.  */
    for (GENLIB_yyi = GENLIB_yyprhs[GENLIB_yyn]; GENLIB_yyrhs[GENLIB_yyi] > 0;
         GENLIB_yyi++)
      YYFPRINTF(stderr, "%s ", GENLIB_yytname[GENLIB_yyrhs[GENLIB_yyi]]);
    YYFPRINTF(stderr, " -> %s\n", GENLIB_yytname[GENLIB_yyr1[GENLIB_yyn]]);
  }
#endif

  switch (GENLIB_yyn) {

  case 2:
#line 67 "readlib.y"
  {
    global_fct = GENLIB_yyvsp[0].functionval;
  } break;
  case 5:
#line 75 "readlib.y"
  {
    if (!gl_convert_gate_to_blif(GENLIB_yyvsp[-3].strval,
                                 GENLIB_yyvsp[-2].realval,
                                 GENLIB_yyvsp[-1].functionval,
                                 GENLIB_yyvsp[0].pinval, global_use_nor))
      GENLIB_yyerror(NIL(char));
  } break;
  case 6:
#line 80 "readlib.y"
  {
    if (!gl_convert_latch_to_blif(
            GENLIB_yyvsp[-6].strval, GENLIB_yyvsp[-5].realval,
            GENLIB_yyvsp[-4].functionval, GENLIB_yyvsp[-3].pinval,
            global_use_nor, GENLIB_yyvsp[-2].latchval, GENLIB_yyvsp[-1].pinval,
            GENLIB_yyvsp[0].constrval))
      GENLIB_yyerror(NIL(char));
  } break;
  case 7:
#line 87 "readlib.y"
  {
    GENLIB_yyval.pinval = 0;
  } break;
  case 8:
#line 89 "readlib.y"
  {
    GENLIB_yyvsp[0].pinval->next = GENLIB_yyvsp[-1].pinval;
    GENLIB_yyval.pinval = GENLIB_yyvsp[0].pinval;
  } break;
  case 9:
#line 93 "readlib.y"
  {
    GENLIB_yyval.pinval = ALLOC(pin_info_t, 1);
    GENLIB_yyval.pinval->name = GENLIB_yyvsp[-7].strval;
    GENLIB_yyval.pinval->phase = GENLIB_yyvsp[-6].strval;
    GENLIB_yyval.pinval->value[0] = GENLIB_yyvsp[-5].realval;
    GENLIB_yyval.pinval->value[1] = GENLIB_yyvsp[-4].realval;
    GENLIB_yyval.pinval->value[2] = GENLIB_yyvsp[-3].realval;
    GENLIB_yyval.pinval->value[3] = GENLIB_yyvsp[-2].realval;
    GENLIB_yyval.pinval->value[4] = GENLIB_yyvsp[-1].realval;
    GENLIB_yyval.pinval->value[5] = GENLIB_yyvsp[0].realval;
    GENLIB_yyval.pinval->next = 0;
  } break;
  case 10:
#line 105 "readlib.y"
  {
    if (strcmp(GENLIB_yyvsp[0].strval, "INV") != 0) {
      if (strcmp(GENLIB_yyvsp[0].strval, "NONINV") != 0) {
        if (strcmp(GENLIB_yyvsp[0].strval, "UNKNOWN") != 0) {
          GENLIB_yyerror("bad pin phase");
        }
      }
    }
    GENLIB_yyval.strval = GENLIB_yyvsp[0].strval;
  } break;
  case 11:
#line 118 "readlib.y"
  {
    GENLIB_yyval.strval = GENLIB_yyvsp[0].strval;
  } break;
  case 12:
#line 120 "readlib.y"
  {
    GENLIB_yyval.strval = util_strsav(GENLIB_yytext);
  } break;
  case 13:
#line 124 "readlib.y"
  {
    GENLIB_yyval.functionval = ALLOC(function_t, 1);
    GENLIB_yyval.functionval->name = GENLIB_yyvsp[-3].strval;
    GENLIB_yyval.functionval->tree = GENLIB_yyvsp[-1].nodeval;
    GENLIB_yyval.functionval->next = 0;
  } break;
  case 14:
#line 133 "readlib.y"
  {
    GENLIB_yyval.nodeval = gl_alloc_node(2);
    GENLIB_yyval.nodeval->phase = 1;
    GENLIB_yyval.nodeval->sons[0] = GENLIB_yyvsp[-2].nodeval;
    GENLIB_yyval.nodeval->sons[1] = GENLIB_yyvsp[0].nodeval;
    GENLIB_yyval.nodeval->type = OR_NODE;
  } break;
  case 15:
#line 141 "readlib.y"
  {
    GENLIB_yyval.nodeval = gl_alloc_node(2);
    GENLIB_yyval.nodeval->phase = 1;
    GENLIB_yyval.nodeval->sons[0] = GENLIB_yyvsp[-2].nodeval;
    GENLIB_yyval.nodeval->sons[1] = GENLIB_yyvsp[0].nodeval;
    GENLIB_yyval.nodeval->type = AND_NODE;
  } break;
  case 16:
#line 149 "readlib.y"
  {
    GENLIB_yyval.nodeval = gl_alloc_node(2);
    GENLIB_yyval.nodeval->phase = 1;
    GENLIB_yyval.nodeval->sons[0] = GENLIB_yyvsp[-1].nodeval;
    GENLIB_yyval.nodeval->sons[1] = GENLIB_yyvsp[0].nodeval;
    GENLIB_yyval.nodeval->type = AND_NODE;
  } break;
  case 17:
#line 157 "readlib.y"
  {
    GENLIB_yyval.nodeval = GENLIB_yyvsp[-1].nodeval;
  } break;
  case 18:
#line 161 "readlib.y"
  {
    GENLIB_yyval.nodeval = GENLIB_yyvsp[0].nodeval;
    GENLIB_yyval.nodeval->phase = 0;
  } break;
  case 19:
#line 166 "readlib.y"
  {
    GENLIB_yyval.nodeval = GENLIB_yyvsp[-1].nodeval;
    GENLIB_yyval.nodeval->phase = 0;
  } break;
  case 20:
#line 171 "readlib.y"
  {
    GENLIB_yyval.nodeval = gl_alloc_leaf(GENLIB_yyvsp[0].strval);
    FREE(GENLIB_yyvsp[0].strval);
    GENLIB_yyval.nodeval->type = LEAF_NODE; /* hack */
    GENLIB_yyval.nodeval->phase = 1;
  } break;
  case 21:
#line 178 "readlib.y"
  {
    GENLIB_yyval.nodeval = gl_alloc_leaf("0");
    GENLIB_yyval.nodeval->phase = 1;
    GENLIB_yyval.nodeval->type = ZERO_NODE;
  } break;
  case 22:
#line 184 "readlib.y"
  {
    GENLIB_yyval.nodeval = gl_alloc_leaf("1");
    GENLIB_yyval.nodeval->phase = 1;
    GENLIB_yyval.nodeval->type = ONE_NODE;
  } break;
  case 23:
#line 192 "readlib.y"
  {
    GENLIB_yyval.strval = util_strsav(GENLIB_yytext);
  } break;
  case 24:
#line 195 "readlib.y"
  {
    GENLIB_yyval.strval = util_strsav(GENLIB_yytext);
  } break;
  case 25:
#line 199 "readlib.y"
  {
    GENLIB_yyval.realval = atof(GENLIB_yytext);
  } break;
  case 26:
#line 203 "readlib.y"
  {
    GENLIB_yyval.pinval = 0;
  } break;
  case 27:
#line 205 "readlib.y"
  {
    GENLIB_yyval.pinval = ALLOC(pin_info_t, 1);
    GENLIB_yyval.pinval->name = GENLIB_yyvsp[-6].strval;
    GENLIB_yyval.pinval->phase = util_strsav("UNKNOWN");
    GENLIB_yyval.pinval->value[0] = GENLIB_yyvsp[-5].realval;
    GENLIB_yyval.pinval->value[1] = GENLIB_yyvsp[-4].realval;
    GENLIB_yyval.pinval->value[2] = GENLIB_yyvsp[-3].realval;
    GENLIB_yyval.pinval->value[3] = GENLIB_yyvsp[-2].realval;
    GENLIB_yyval.pinval->value[4] = GENLIB_yyvsp[-1].realval;
    GENLIB_yyval.pinval->value[5] = GENLIB_yyvsp[0].realval;
    GENLIB_yyval.pinval->next = 0;
  } break;
  case 28:
#line 219 "readlib.y"
  {
    GENLIB_yyval.constrval = 0;
  } break;
  case 29:
#line 221 "readlib.y"
  {
    GENLIB_yyvsp[0].constrval->next = GENLIB_yyvsp[-1].constrval;
    GENLIB_yyval.constrval = GENLIB_yyvsp[0].constrval;
  } break;
  case 30:
#line 228 "readlib.y"
  {
    GENLIB_yyval.constrval = ALLOC(constraint_info_t, 1);
    GENLIB_yyval.constrval->name = GENLIB_yyvsp[-2].strval;
    GENLIB_yyval.constrval->setup = GENLIB_yyvsp[-1].realval;
    GENLIB_yyval.constrval->hold = GENLIB_yyvsp[0].realval;
    GENLIB_yyval.constrval->next = 0;
  } break;
  case 31:
#line 238 "readlib.y"
  {
    GENLIB_yyval.latchval = ALLOC(latch_info_t, 1);
    GENLIB_yyval.latchval->in = GENLIB_yyvsp[-2].strval;
    GENLIB_yyval.latchval->out = GENLIB_yyvsp[-1].strval;
    GENLIB_yyval.latchval->type = GENLIB_yyvsp[0].strval;
  } break;
  case 32:
#line 247 "readlib.y"
  {
    if (strcmp(GENLIB_yytext, "RISING_EDGE") == 0) {
      GENLIB_yyval.strval = util_strsav("re");
    } else if (strcmp(GENLIB_yytext, "FALLING_EDGE") == 0) {
      GENLIB_yyval.strval = util_strsav("fe");
    } else if (strcmp(GENLIB_yytext, "ACTIVE_HIGH") == 0) {
      GENLIB_yyval.strval = util_strsav("ah");
    } else if (strcmp(GENLIB_yytext, "ACTIVE_LOW") == 0) {
      GENLIB_yyval.strval = util_strsav("al");
    } else if (strcmp(GENLIB_yytext, "ASYNCH") == 0) {
      GENLIB_yyval.strval = util_strsav("as");
    } else {
      GENLIB_yyerror("latch type can only be re/fe/ah/al/as");
    }
  } break;
  }

#line 705 "/usr/share/bison/bison.simple"

  GENLIB_yyvsp -= GENLIB_yylen;
  GENLIB_yyssp -= GENLIB_yylen;
#if YYLSP_NEEDED
  GENLIB_yylsp -= GENLIB_yylen;
#endif

#if YYDEBUG
  if (GENLIB_yydebug) {
    short *GENLIB_yyssp1 = GENLIB_yyss - 1;
    YYFPRINTF(stderr, "state stack now");
    while (GENLIB_yyssp1 != GENLIB_yyssp)
      YYFPRINTF(stderr, " %d", *++GENLIB_yyssp1);
    YYFPRINTF(stderr, "\n");
  }
#endif

  *++GENLIB_yyvsp = GENLIB_yyval;
#if YYLSP_NEEDED
  *++GENLIB_yylsp = GENLIB_yyloc;
#endif

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  GENLIB_yyn = GENLIB_yyr1[GENLIB_yyn];

  GENLIB_yystate = GENLIB_yypgoto[GENLIB_yyn - YYNTBASE] + *GENLIB_yyssp;
  if (GENLIB_yystate >= 0 && GENLIB_yystate <= YYLAST &&
      GENLIB_yycheck[GENLIB_yystate] == *GENLIB_yyssp)
    GENLIB_yystate = GENLIB_yytable[GENLIB_yystate];
  else
    GENLIB_yystate = GENLIB_yydefgoto[GENLIB_yyn - YYNTBASE];

  goto GENLIB_yynewstate;

  /*------------------------------------.
  | GENLIB_yyerrlab -- here on detecting error |
  `------------------------------------*/
GENLIB_yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!GENLIB_yyerrstatus) {
    ++GENLIB_yynerrs;

#ifdef YYERROR_VERBOSE
    GENLIB_yyn = GENLIB_yypact[GENLIB_yystate];

    if (GENLIB_yyn > YYFLAG && GENLIB_yyn < YYLAST) {
      YYSIZE_T GENLIB_yysize = 0;
      char *GENLIB_yymsg;
      int GENLIB_yyx, GENLIB_yycount;

      GENLIB_yycount = 0;
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  */
      for (GENLIB_yyx = GENLIB_yyn < 0 ? -GENLIB_yyn : 0;
           GENLIB_yyx < (int)(sizeof(GENLIB_yytname) / sizeof(char *));
           GENLIB_yyx++)
        if (GENLIB_yycheck[GENLIB_yyx + GENLIB_yyn] == GENLIB_yyx)
          GENLIB_yysize += GENLIB_yystrlen(GENLIB_yytname[GENLIB_yyx]) + 15,
              GENLIB_yycount++;
      GENLIB_yysize += GENLIB_yystrlen("parse error, unexpected ") + 1;
      GENLIB_yysize +=
          GENLIB_yystrlen(GENLIB_yytname[YYTRANSLATE(GENLIB_yychar)]);
      GENLIB_yymsg = (char *)YYSTACK_ALLOC(GENLIB_yysize);
      if (GENLIB_yymsg != 0) {
        char *GENLIB_yyp =
            GENLIB_yystpcpy(GENLIB_yymsg, "parse error, unexpected ");
        GENLIB_yyp = GENLIB_yystpcpy(
            GENLIB_yyp, GENLIB_yytname[YYTRANSLATE(GENLIB_yychar)]);

        if (GENLIB_yycount < 5) {
          GENLIB_yycount = 0;
          for (GENLIB_yyx = GENLIB_yyn < 0 ? -GENLIB_yyn : 0;
               GENLIB_yyx < (int)(sizeof(GENLIB_yytname) / sizeof(char *));
               GENLIB_yyx++)
            if (GENLIB_yycheck[GENLIB_yyx + GENLIB_yyn] == GENLIB_yyx) {
              const char *GENLIB_yyq =
                  !GENLIB_yycount ? ", expecting " : " or ";
              GENLIB_yyp = GENLIB_yystpcpy(GENLIB_yyp, GENLIB_yyq);
              GENLIB_yyp =
                  GENLIB_yystpcpy(GENLIB_yyp, GENLIB_yytname[GENLIB_yyx]);
              GENLIB_yycount++;
            }
        }
        GENLIB_yyerror(GENLIB_yymsg);
        YYSTACK_FREE(GENLIB_yymsg);
      } else
        GENLIB_yyerror("parse error; also virtual memory exhausted");
    } else
#endif /* defined (YYERROR_VERBOSE) */
      GENLIB_yyerror("parse error");
  }
  goto GENLIB_yyerrlab1;

  /*--------------------------------------------------.
  | GENLIB_yyerrlab1 -- error raised explicitly by an action |
  `--------------------------------------------------*/
GENLIB_yyerrlab1:
  if (GENLIB_yyerrstatus == 3) {
    /* If just tried and failed to reuse lookahead token after an
   error, discard it.  */

    /* return failure if at end of input */
    if (GENLIB_yychar == YYEOF)
      YYABORT;
    YYDPRINTF((stderr, "Discarding token %d (%s).\n", GENLIB_yychar,
               GENLIB_yytname[GENLIB_yychar1]));
    GENLIB_yychar = YYEMPTY;
  }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */

  GENLIB_yyerrstatus = 3; /* Each real token shifted decrements this */

  goto GENLIB_yyerrhandle;

  /*-------------------------------------------------------------------.
  | GENLIB_yyerrdefault -- current state does not do anything special for the |
  | error token.                                                       |
  `-------------------------------------------------------------------*/
GENLIB_yyerrdefault:
#if 0
    /* This is wrong; only states that explicitly want error tokens
       should shift them.  */

    /* If its default is to accept any token, ok.  Otherwise pop it.  */
    GENLIB_yyn = GENLIB_yydefact[GENLIB_yystate];
    if (GENLIB_yyn)
      goto GENLIB_yydefault;
#endif

  /*---------------------------------------------------------------.
  | GENLIB_yyerrpop -- pop the current state because it cannot handle the |
  | error token                                                    |
  `---------------------------------------------------------------*/
GENLIB_yyerrpop:
  if (GENLIB_yyssp == GENLIB_yyss)
    YYABORT;
  GENLIB_yyvsp--;
  GENLIB_yystate = *--GENLIB_yyssp;
#if YYLSP_NEEDED
  GENLIB_yylsp--;
#endif

#if YYDEBUG
  if (GENLIB_yydebug) {
    short *GENLIB_yyssp1 = GENLIB_yyss - 1;
    YYFPRINTF(stderr, "Error: state stack now");
    while (GENLIB_yyssp1 != GENLIB_yyssp)
      YYFPRINTF(stderr, " %d", *++GENLIB_yyssp1);
    YYFPRINTF(stderr, "\n");
  }
#endif

  /*--------------.
  | GENLIB_yyerrhandle.  |
  `--------------*/
GENLIB_yyerrhandle:
  GENLIB_yyn = GENLIB_yypact[GENLIB_yystate];
  if (GENLIB_yyn == YYFLAG)
    goto GENLIB_yyerrdefault;

  GENLIB_yyn += YYTERROR;
  if (GENLIB_yyn < 0 || GENLIB_yyn > YYLAST ||
      GENLIB_yycheck[GENLIB_yyn] != YYTERROR)
    goto GENLIB_yyerrdefault;

  GENLIB_yyn = GENLIB_yytable[GENLIB_yyn];
  if (GENLIB_yyn < 0) {
    if (GENLIB_yyn == YYFLAG)
      goto GENLIB_yyerrpop;
    GENLIB_yyn = -GENLIB_yyn;
    goto GENLIB_yyreduce;
  } else if (GENLIB_yyn == 0)
    goto GENLIB_yyerrpop;

  if (GENLIB_yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF((stderr, "Shifting error token, "));

  *++GENLIB_yyvsp = GENLIB_yylval;
#if YYLSP_NEEDED
  *++GENLIB_yylsp = GENLIB_yylloc;
#endif

  GENLIB_yystate = GENLIB_yyn;
  goto GENLIB_yynewstate;

  /*-------------------------------------.
  | GENLIB_yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
GENLIB_yyacceptlab:
  GENLIB_yyresult = 0;
  goto GENLIB_yyreturn;

  /*-----------------------------------.
  | GENLIB_yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
GENLIB_yyabortlab:
  GENLIB_yyresult = 1;
  goto GENLIB_yyreturn;

  /*---------------------------------------------.
  | GENLIB_yyoverflowab -- parser overflow comes here.  |
  `---------------------------------------------*/
GENLIB_yyoverflowlab:
  GENLIB_yyerror("parser stack overflow");
  GENLIB_yyresult = 2;
  /* Fall through.  */

GENLIB_yyreturn:
#ifndef GENLIB_yyoverflow
  if (GENLIB_yyss != GENLIB_yyssa)
    YYSTACK_FREE(GENLIB_yyss);
#endif
  return GENLIB_yyresult;
}

#line 264 "readlib.y"

static jmp_buf jmpbuf;

int GENLIB_yyerror(errmsg) char *errmsg;
{
  if (errmsg != 0)
    read_error(errmsg);
  longjmp(jmpbuf, 1);
}

int gl_convert_genlib_to_blif(filename, use_nor) char *filename;
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

  global_fct = 0; /* not used here, just to check for error */
  global_use_nor = use_nor;

  if (setjmp(jmpbuf)) {
    return 0;
  } else {
    (void)GENLIB_yyparse();
    if (global_fct != 0)
      GENLIB_yyerror("syntax error");
    return 1;
  }
}

int genlib_parse_library(infp, infilename, outfp, use_nor) FILE *infp;
char *infilename;
FILE *outfp;
int use_nor;
{
  error_init();
  library_setup_file(infp, infilename);
  gl_out_file = outfp;
  global_fct = 0; /* not used here, just to check for error */
  global_use_nor = use_nor;

  if (setjmp(jmpbuf)) {
    return 0;
  } else {
    (void)GENLIB_yyparse();
    if (global_fct != 0)
      GENLIB_yyerror("syntax error");
    return 1;
  }
}

int gl_convert_expression_to_tree(string, tree, name) char *string;
tree_node_t **tree;
char **name;
{
  error_init();
  global_fct = 0;
  library_setup_string(string);

  if (setjmp(jmpbuf)) {
    return 0;
  } else {
    (void)GENLIB_yyparse();
    if (global_fct == 0)
      GENLIB_yyerror("syntax error");
    *tree = global_fct->tree;
    *name = global_fct->name;
    return 1;
  }
}
