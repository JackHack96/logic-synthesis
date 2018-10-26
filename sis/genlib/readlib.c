/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with GENLIB_yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 2 "readlib.y" /* yacc.c:339  */

/* file @(#)readlib.y	1.2                      */
/* last modified on 6/13/91 at 17:46:40   */
#include <setjmp.h>
#include "genlib_int.h"
#include "config.h"

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


#line 93 "y.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "y.tab.h".  */
#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int GENLIB_yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum GENLIB_yytokentype
  {
    OPR_OR = 258,
    OPR_AND = 259,
    CONST1 = 260,
    CONST0 = 261,
    IDENTIFIER = 262,
    LPAREN = 263,
    REAL = 264,
    OPR_NOT = 265,
    OPR_NOT_POST = 266,
    GATE = 267,
    PIN = 268,
    SEMI = 269,
    ASSIGN = 270,
    RPAREN = 271,
    LATCH = 272,
    CONTROL = 273,
    CONSTRAINT = 274,
    SEQ = 275
  };
#endif
/* Tokens.  */
#define OPR_OR 258
#define OPR_AND 259
#define CONST1 260
#define CONST0 261
#define IDENTIFIER 262
#define LPAREN 263
#define REAL 264
#define OPR_NOT 265
#define OPR_NOT_POST 266
#define GATE 267
#define PIN 268
#define SEMI 269
#define ASSIGN 270
#define RPAREN 271
#define LATCH 272
#define CONTROL 273
#define CONSTRAINT 274
#define SEQ 275

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 29 "readlib.y" /* yacc.c:355  */

    tree_node_t *nodeval;
    char *strval;
    double realval; 
    pin_info_t *pinval;
    function_t *functionval;
    latch_info_t *latchval;
    constraint_info_t *constrval;

#line 183 "y.tab.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE GENLIB_yylval;

int GENLIB_yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 200 "y.tab.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 GENLIB_yytype_uint8;
#else
typedef unsigned char GENLIB_yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 GENLIB_yytype_int8;
#else
typedef signed char GENLIB_yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 GENLIB_yytype_uint16;
#else
typedef unsigned short int GENLIB_yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 GENLIB_yytype_int16;
#else
typedef short int GENLIB_yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about GENLIB_yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined GENLIB_yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined GENLIB_yyoverflow || YYERROR_VERBOSE */


#if (! defined GENLIB_yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union GENLIB_yyalloc
{
  GENLIB_yytype_int16 GENLIB_yyss_alloc;
  YYSTYPE GENLIB_yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union GENLIB_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (GENLIB_yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T GENLIB_yynewbytes;                                            \
        YYCOPY (&GENLIB_yyptr->Stack_alloc, Stack, GENLIB_yysize);                    \
        Stack = &GENLIB_yyptr->Stack_alloc;                                    \
        GENLIB_yynewbytes = GENLIB_yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        GENLIB_yyptr += GENLIB_yynewbytes / sizeof (*GENLIB_yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T GENLIB_yyi;                         \
          for (GENLIB_yyi = 0; GENLIB_yyi < (Count); GENLIB_yyi++)   \
            (Dst)[GENLIB_yyi] = (Src)[GENLIB_yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  7
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   93

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  21
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  17
/* YYNRULES -- Number of rules.  */
#define YYNRULES  33
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  71

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by GENLIB_yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   275

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? GENLIB_yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by GENLIB_yylex, without out-of-bounds checking.  */
static const GENLIB_yytype_uint8 GENLIB_yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const GENLIB_yytype_uint8 GENLIB_yyrline[] =
{
       0,    57,    57,    58,    62,    63,    66,    71,    79,    80,
      84,    96,   109,   111,   115,   124,   132,   140,   148,   152,
     157,   162,   169,   175,   183,   186,   190,   195,   196,   211,
     212,   219,   229,   238
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const GENLIB_yytname[] =
{
  "$end", "error", "$undefined", "OPR_OR", "OPR_AND", "CONST1", "CONST0",
  "IDENTIFIER", "LPAREN", "REAL", "OPR_NOT", "OPR_NOT_POST", "GATE", "PIN",
  "SEMI", "ASSIGN", "RPAREN", "LATCH", "CONTROL", "CONSTRAINT", "SEQ",
  "$accept", "hack", "gates", "gate_info", "pins", "pin_info", "pin_phase",
  "pin_name", "function", "expr", "name", "real", "clock", "constraints",
  "constraint", "latch", "type", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const GENLIB_yytype_uint16 GENLIB_yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275
};
# endif

#define YYPACT_NINF -11

#define GENLIB_yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-11)))

#define YYTABLE_NINF -1

#define GENLIB_yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const GENLIB_yytype_int8 GENLIB_yypact[] =
{
      11,   -11,   -11,    13,    -7,   -11,    17,   -11,    11,    11,
     -11,    83,    26,    26,   -11,   -11,    83,    83,    65,   -11,
     -11,    11,    11,    20,   -11,    83,    83,   -11,   -11,    -8,
     -11,   -11,   -11,    76,    -8,    25,    -9,    10,   -11,    11,
      22,   -11,    11,   -11,    11,    11,   -11,    26,   -11,    34,
      26,    24,    26,   -11,   -11,    26,    10,   -11,    26,    26,
      26,    26,    26,    26,    26,    26,   -11,    26,    26,   -11,
     -11
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const GENLIB_yytype_uint8 GENLIB_yydefact[] =
{
       4,    24,    25,     0,     2,     3,     0,     1,     0,     0,
       5,     0,     0,     0,    23,    22,     0,     0,     0,    21,
      26,     0,     0,     0,    19,     0,     0,    20,    14,    17,
       8,     8,    18,    15,    16,     6,     0,     0,     9,     0,
      27,    13,     0,    12,     0,     0,    29,     0,    11,     0,
       0,     7,     0,    33,    32,     0,     0,    30,     0,     0,
       0,     0,     0,     0,     0,     0,    31,     0,     0,    10,
      28
};

  /* YYPGOTO[NTERM-NUM].  */
static const GENLIB_yytype_int8 GENLIB_yypgoto[] =
{
     -11,   -11,   -11,   -11,    16,   -11,   -11,    -6,    12,   -10,
       0,    -1,   -11,   -11,   -11,   -11,   -11
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const GENLIB_yytype_int8 GENLIB_yydefgoto[] =
{
      -1,     3,     4,    10,    35,    38,    47,    42,     5,    29,
      19,    21,    46,    51,    57,    40,    54
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const GENLIB_yytype_uint8 GENLIB_yytable[] =
{
       6,    18,    17,    27,    37,     8,    23,    24,    12,    13,
       9,    39,    22,     7,    41,    33,    34,     1,     1,     2,
       2,     6,     6,    25,    26,    14,    15,     1,    16,     2,
      17,    27,    11,    30,    31,    20,    32,    43,    37,    44,
      45,    53,    48,    56,    49,    50,    52,    36,     0,    55,
      60,    58,     0,     0,    59,     0,    43,    61,    62,    63,
      64,    65,    66,    67,    68,     0,    69,    70,    25,    26,
      14,    15,     1,    16,     2,    17,    27,     0,     0,    28,
      26,    14,    15,     1,    16,     2,    17,    27,    14,    15,
       1,    16,     2,    17
};

static const GENLIB_yytype_int8 GENLIB_yycheck[] =
{
       0,    11,    10,    11,    13,    12,    16,    17,     8,     9,
      17,    20,    13,     0,     4,    25,    26,     7,     7,     9,
       9,    21,    22,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    15,    21,    22,     9,    16,    37,    13,    39,
      18,     7,    42,    19,    44,    45,    47,    31,    -1,    50,
      56,    52,    -1,    -1,    55,    -1,    56,    58,    59,    60,
      61,    62,    63,    64,    65,    -1,    67,    68,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    -1,    -1,    14,
       4,     5,     6,     7,     8,     9,    10,    11,     5,     6,
       7,     8,     9,    10
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const GENLIB_yytype_uint8 GENLIB_yystos[] =
{
       0,     7,     9,    22,    23,    29,    31,     0,    12,    17,
      24,    15,    31,    31,     5,     6,     8,    10,    30,    31,
       9,    32,    32,    30,    30,     3,     4,    11,    14,    30,
      29,    29,    16,    30,    30,    25,    25,    13,    26,    20,
      36,     4,    28,    31,    31,    18,    33,    27,    31,    31,
      31,    34,    32,     7,    37,    32,    19,    35,    32,    32,
      28,    32,    32,    32,    32,    32,    32,    32,    32,    32,
      32
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const GENLIB_yytype_uint8 GENLIB_yyr1[] =
{
       0,    21,    22,    22,    23,    23,    24,    24,    25,    25,
      26,    27,    28,    28,    29,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    31,    31,    32,    33,    33,    34,
      34,    35,    36,    37
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const GENLIB_yytype_uint8 GENLIB_yyr2[] =
{
       0,     2,     1,     1,     0,     2,     5,     8,     0,     2,
       9,     1,     1,     1,     4,     3,     3,     2,     3,     2,
       2,     1,     1,     1,     1,     1,     1,     0,     8,     0,
       2,     4,     4,     1
};


#define GENLIB_yyerrok         (GENLIB_yyerrstatus = 0)
#define GENLIB_yyclearin       (GENLIB_yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto GENLIB_yyacceptlab
#define YYABORT         goto GENLIB_yyabortlab
#define YYERROR         goto GENLIB_yyerrorlab


#define YYRECOVERING()  (!!GENLIB_yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (GENLIB_yychar == YYEMPTY)                                        \
    {                                                           \
      GENLIB_yychar = (Token);                                         \
      GENLIB_yylval = (Value);                                         \
      YYPOPSTACK (GENLIB_yylen);                                       \
      GENLIB_yystate = *GENLIB_yyssp;                                         \
      goto GENLIB_yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      GENLIB_yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (GENLIB_yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (GENLIB_yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      GENLIB_yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
GENLIB_yy_symbol_value_print (FILE *GENLIB_yyoutput, int GENLIB_yytype, YYSTYPE const * const GENLIB_yyvaluep)
{
  FILE *GENLIB_yyo = GENLIB_yyoutput;
  YYUSE (GENLIB_yyo);
  if (!GENLIB_yyvaluep)
    return;
# ifdef YYPRINT
  if (GENLIB_yytype < YYNTOKENS)
    YYPRINT (GENLIB_yyoutput, GENLIB_yytoknum[GENLIB_yytype], *GENLIB_yyvaluep);
# endif
  YYUSE (GENLIB_yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
GENLIB_yy_symbol_print (FILE *GENLIB_yyoutput, int GENLIB_yytype, YYSTYPE const * const GENLIB_yyvaluep)
{
  YYFPRINTF (GENLIB_yyoutput, "%s %s (",
             GENLIB_yytype < YYNTOKENS ? "token" : "nterm", GENLIB_yytname[GENLIB_yytype]);

  GENLIB_yy_symbol_value_print (GENLIB_yyoutput, GENLIB_yytype, GENLIB_yyvaluep);
  YYFPRINTF (GENLIB_yyoutput, ")");
}

/*------------------------------------------------------------------.
| GENLIB_yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
GENLIB_yy_stack_print (GENLIB_yytype_int16 *GENLIB_yybottom, GENLIB_yytype_int16 *GENLIB_yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; GENLIB_yybottom <= GENLIB_yytop; GENLIB_yybottom++)
    {
      int GENLIB_yybot = *GENLIB_yybottom;
      YYFPRINTF (stderr, " %d", GENLIB_yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (GENLIB_yydebug)                                                  \
    GENLIB_yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
GENLIB_yy_reduce_print (GENLIB_yytype_int16 *GENLIB_yyssp, YYSTYPE *GENLIB_yyvsp, int GENLIB_yyrule)
{
  unsigned long int GENLIB_yylno = GENLIB_yyrline[GENLIB_yyrule];
  int GENLIB_yynrhs = GENLIB_yyr2[GENLIB_yyrule];
  int GENLIB_yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             GENLIB_yyrule - 1, GENLIB_yylno);
  /* The symbols being reduced.  */
  for (GENLIB_yyi = 0; GENLIB_yyi < GENLIB_yynrhs; GENLIB_yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", GENLIB_yyi + 1);
      GENLIB_yy_symbol_print (stderr,
                       GENLIB_yystos[GENLIB_yyssp[GENLIB_yyi + 1 - GENLIB_yynrhs]],
                       &(GENLIB_yyvsp[(GENLIB_yyi + 1) - (GENLIB_yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (GENLIB_yydebug)                          \
    GENLIB_yy_reduce_print (GENLIB_yyssp, GENLIB_yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int GENLIB_yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef GENLIB_yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define GENLIB_yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
GENLIB_yystrlen (const char *GENLIB_yystr)
{
  YYSIZE_T GENLIB_yylen;
  for (GENLIB_yylen = 0; GENLIB_yystr[GENLIB_yylen]; GENLIB_yylen++)
    continue;
  return GENLIB_yylen;
}
#  endif
# endif

# ifndef GENLIB_yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define GENLIB_yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
GENLIB_yystpcpy (char *GENLIB_yydest, const char *GENLIB_yysrc)
{
  char *GENLIB_yyd = GENLIB_yydest;
  const char *GENLIB_yys = GENLIB_yysrc;

  while ((*GENLIB_yyd++ = *GENLIB_yys++) != '\0')
    continue;

  return GENLIB_yyd - 1;
}
#  endif
# endif

# ifndef GENLIB_yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for GENLIB_yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from GENLIB_yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
GENLIB_yytnamerr (char *GENLIB_yyres, const char *GENLIB_yystr)
{
  if (*GENLIB_yystr == '"')
    {
      YYSIZE_T GENLIB_yyn = 0;
      char const *GENLIB_yyp = GENLIB_yystr;

      for (;;)
        switch (*++GENLIB_yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++GENLIB_yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (GENLIB_yyres)
              GENLIB_yyres[GENLIB_yyn] = *GENLIB_yyp;
            GENLIB_yyn++;
            break;

          case '"':
            if (GENLIB_yyres)
              GENLIB_yyres[GENLIB_yyn] = '\0';
            return GENLIB_yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! GENLIB_yyres)
    return GENLIB_yystrlen (GENLIB_yystr);

  return GENLIB_yystpcpy (GENLIB_yyres, GENLIB_yystr) - GENLIB_yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
GENLIB_yysyntax_error (YYSIZE_T *GENLIB_yymsg_alloc, char **GENLIB_yymsg,
                GENLIB_yytype_int16 *GENLIB_yyssp, int GENLIB_yytoken)
{
  YYSIZE_T GENLIB_yysize0 = GENLIB_yytnamerr (YY_NULLPTR, GENLIB_yytname[GENLIB_yytoken]);
  YYSIZE_T GENLIB_yysize = GENLIB_yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *GENLIB_yyformat = YY_NULLPTR;
  /* Arguments of GENLIB_yyformat. */
  char const *GENLIB_yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int GENLIB_yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in GENLIB_yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated GENLIB_yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (GENLIB_yytoken != YYEMPTY)
    {
      int GENLIB_yyn = GENLIB_yypact[*GENLIB_yyssp];
      GENLIB_yyarg[GENLIB_yycount++] = GENLIB_yytname[GENLIB_yytoken];
      if (!GENLIB_yypact_value_is_default (GENLIB_yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int GENLIB_yyxbegin = GENLIB_yyn < 0 ? -GENLIB_yyn : 0;
          /* Stay within bounds of both GENLIB_yycheck and GENLIB_yytname.  */
          int GENLIB_yychecklim = YYLAST - GENLIB_yyn + 1;
          int GENLIB_yyxend = GENLIB_yychecklim < YYNTOKENS ? GENLIB_yychecklim : YYNTOKENS;
          int GENLIB_yyx;

          for (GENLIB_yyx = GENLIB_yyxbegin; GENLIB_yyx < GENLIB_yyxend; ++GENLIB_yyx)
            if (GENLIB_yycheck[GENLIB_yyx + GENLIB_yyn] == GENLIB_yyx && GENLIB_yyx != YYTERROR
                && !GENLIB_yytable_value_is_error (GENLIB_yytable[GENLIB_yyx + GENLIB_yyn]))
              {
                if (GENLIB_yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    GENLIB_yycount = 1;
                    GENLIB_yysize = GENLIB_yysize0;
                    break;
                  }
                GENLIB_yyarg[GENLIB_yycount++] = GENLIB_yytname[GENLIB_yyx];
                {
                  YYSIZE_T GENLIB_yysize1 = GENLIB_yysize + GENLIB_yytnamerr (YY_NULLPTR, GENLIB_yytname[GENLIB_yyx]);
                  if (! (GENLIB_yysize <= GENLIB_yysize1
                         && GENLIB_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  GENLIB_yysize = GENLIB_yysize1;
                }
              }
        }
    }

  switch (GENLIB_yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        GENLIB_yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T GENLIB_yysize1 = GENLIB_yysize + GENLIB_yystrlen (GENLIB_yyformat);
    if (! (GENLIB_yysize <= GENLIB_yysize1 && GENLIB_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    GENLIB_yysize = GENLIB_yysize1;
  }

  if (*GENLIB_yymsg_alloc < GENLIB_yysize)
    {
      *GENLIB_yymsg_alloc = 2 * GENLIB_yysize;
      if (! (GENLIB_yysize <= *GENLIB_yymsg_alloc
             && *GENLIB_yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *GENLIB_yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *GENLIB_yyp = *GENLIB_yymsg;
    int GENLIB_yyi = 0;
    while ((*GENLIB_yyp = *GENLIB_yyformat) != '\0')
      if (*GENLIB_yyp == '%' && GENLIB_yyformat[1] == 's' && GENLIB_yyi < GENLIB_yycount)
        {
          GENLIB_yyp += GENLIB_yytnamerr (GENLIB_yyp, GENLIB_yyarg[GENLIB_yyi++]);
          GENLIB_yyformat += 2;
        }
      else
        {
          GENLIB_yyp++;
          GENLIB_yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
GENLIB_yydestruct (const char *GENLIB_yymsg, int GENLIB_yytype, YYSTYPE *GENLIB_yyvaluep)
{
  YYUSE (GENLIB_yyvaluep);
  if (!GENLIB_yymsg)
    GENLIB_yymsg = "Deleting";
  YY_SYMBOL_PRINT (GENLIB_yymsg, GENLIB_yytype, GENLIB_yyvaluep, GENLIB_yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (GENLIB_yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int GENLIB_yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE GENLIB_yylval;
/* Number of syntax errors so far.  */
int GENLIB_yynerrs;


/*----------.
| GENLIB_yyparse.  |
`----------*/

int
GENLIB_yyparse (void)
{
    int GENLIB_yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int GENLIB_yyerrstatus;

    /* The stacks and their tools:
       'GENLIB_yyss': related to states.
       'GENLIB_yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow GENLIB_yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    GENLIB_yytype_int16 GENLIB_yyssa[YYINITDEPTH];
    GENLIB_yytype_int16 *GENLIB_yyss;
    GENLIB_yytype_int16 *GENLIB_yyssp;

    /* The semantic value stack.  */
    YYSTYPE GENLIB_yyvsa[YYINITDEPTH];
    YYSTYPE *GENLIB_yyvs;
    YYSTYPE *GENLIB_yyvsp;

    YYSIZE_T GENLIB_yystacksize;

  int GENLIB_yyn;
  int GENLIB_yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int GENLIB_yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE GENLIB_yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char GENLIB_yymsgbuf[128];
  char *GENLIB_yymsg = GENLIB_yymsgbuf;
  YYSIZE_T GENLIB_yymsg_alloc = sizeof GENLIB_yymsgbuf;
#endif

#define YYPOPSTACK(N)   (GENLIB_yyvsp -= (N), GENLIB_yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int GENLIB_yylen = 0;

  GENLIB_yyssp = GENLIB_yyss = GENLIB_yyssa;
  GENLIB_yyvsp = GENLIB_yyvs = GENLIB_yyvsa;
  GENLIB_yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  GENLIB_yystate = 0;
  GENLIB_yyerrstatus = 0;
  GENLIB_yynerrs = 0;
  GENLIB_yychar = YYEMPTY; /* Cause a token to be read.  */
  goto GENLIB_yysetstate;

/*------------------------------------------------------------.
| GENLIB_yynewstate -- Push a new state, which is found in GENLIB_yystate.  |
`------------------------------------------------------------*/
 GENLIB_yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  GENLIB_yyssp++;

 GENLIB_yysetstate:
  *GENLIB_yyssp = GENLIB_yystate;

  if (GENLIB_yyss + GENLIB_yystacksize - 1 <= GENLIB_yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T GENLIB_yysize = GENLIB_yyssp - GENLIB_yyss + 1;

#ifdef GENLIB_yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *GENLIB_yyvs1 = GENLIB_yyvs;
        GENLIB_yytype_int16 *GENLIB_yyss1 = GENLIB_yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if GENLIB_yyoverflow is a macro.  */
        GENLIB_yyoverflow (YY_("memory exhausted"),
                    &GENLIB_yyss1, GENLIB_yysize * sizeof (*GENLIB_yyssp),
                    &GENLIB_yyvs1, GENLIB_yysize * sizeof (*GENLIB_yyvsp),
                    &GENLIB_yystacksize);

        GENLIB_yyss = GENLIB_yyss1;
        GENLIB_yyvs = GENLIB_yyvs1;
      }
#else /* no GENLIB_yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto GENLIB_yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= GENLIB_yystacksize)
        goto GENLIB_yyexhaustedlab;
      GENLIB_yystacksize *= 2;
      if (YYMAXDEPTH < GENLIB_yystacksize)
        GENLIB_yystacksize = YYMAXDEPTH;

      {
        GENLIB_yytype_int16 *GENLIB_yyss1 = GENLIB_yyss;
        union GENLIB_yyalloc *GENLIB_yyptr =
          (union GENLIB_yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (GENLIB_yystacksize));
        if (! GENLIB_yyptr)
          goto GENLIB_yyexhaustedlab;
        YYSTACK_RELOCATE (GENLIB_yyss_alloc, GENLIB_yyss);
        YYSTACK_RELOCATE (GENLIB_yyvs_alloc, GENLIB_yyvs);
#  undef YYSTACK_RELOCATE
        if (GENLIB_yyss1 != GENLIB_yyssa)
          YYSTACK_FREE (GENLIB_yyss1);
      }
# endif
#endif /* no GENLIB_yyoverflow */

      GENLIB_yyssp = GENLIB_yyss + GENLIB_yysize - 1;
      GENLIB_yyvsp = GENLIB_yyvs + GENLIB_yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) GENLIB_yystacksize));

      if (GENLIB_yyss + GENLIB_yystacksize - 1 <= GENLIB_yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", GENLIB_yystate));

  if (GENLIB_yystate == YYFINAL)
    YYACCEPT;

  goto GENLIB_yybackup;

/*-----------.
| GENLIB_yybackup.  |
`-----------*/
GENLIB_yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  GENLIB_yyn = GENLIB_yypact[GENLIB_yystate];
  if (GENLIB_yypact_value_is_default (GENLIB_yyn))
    goto GENLIB_yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (GENLIB_yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      GENLIB_yychar = GENLIB_yylex ();
    }

  if (GENLIB_yychar <= YYEOF)
    {
      GENLIB_yychar = GENLIB_yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      GENLIB_yytoken = YYTRANSLATE (GENLIB_yychar);
      YY_SYMBOL_PRINT ("Next token is", GENLIB_yytoken, &GENLIB_yylval, &GENLIB_yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  GENLIB_yyn += GENLIB_yytoken;
  if (GENLIB_yyn < 0 || YYLAST < GENLIB_yyn || GENLIB_yycheck[GENLIB_yyn] != GENLIB_yytoken)
    goto GENLIB_yydefault;
  GENLIB_yyn = GENLIB_yytable[GENLIB_yyn];
  if (GENLIB_yyn <= 0)
    {
      if (GENLIB_yytable_value_is_error (GENLIB_yyn))
        goto GENLIB_yyerrlab;
      GENLIB_yyn = -GENLIB_yyn;
      goto GENLIB_yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (GENLIB_yyerrstatus)
    GENLIB_yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", GENLIB_yytoken, &GENLIB_yylval, &GENLIB_yylloc);

  /* Discard the shifted token.  */
  GENLIB_yychar = YYEMPTY;

  GENLIB_yystate = GENLIB_yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++GENLIB_yyvsp = GENLIB_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  GENLIB_yyval = GENLIB_yyvsp[1-GENLIB_yylen];


  YY_REDUCE_PRINT (GENLIB_yyn);
  switch (GENLIB_yyn)
    {
        case 3:
#line 59 "readlib.y" /* yacc.c:1646  */
    {global_fct = (GENLIB_yyvsp[0].functionval); }
#line 1323 "y.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 67 "readlib.y" /* yacc.c:1646  */
    { 
			if (! gl_convert_gate_to_blif((GENLIB_yyvsp[-3].strval),(GENLIB_yyvsp[-2].realval),(GENLIB_yyvsp[-1].functionval),(GENLIB_yyvsp[0].pinval),global_use_nor))
			    GENLIB_yyerror(NIL(char));
		    }
#line 1332 "y.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 72 "readlib.y" /* yacc.c:1646  */
    {
		      if (! gl_convert_latch_to_blif((GENLIB_yyvsp[-6].strval),(GENLIB_yyvsp[-5].realval),(GENLIB_yyvsp[-4].functionval),(GENLIB_yyvsp[-3].pinval),global_use_nor,(GENLIB_yyvsp[-2].latchval),(GENLIB_yyvsp[-1].pinval),(GENLIB_yyvsp[0].constrval)))
			GENLIB_yyerror(NIL(char));
		    }
#line 1341 "y.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 79 "readlib.y" /* yacc.c:1646  */
    { (GENLIB_yyval.pinval) = 0; }
#line 1347 "y.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 81 "readlib.y" /* yacc.c:1646  */
    { (GENLIB_yyvsp[0].pinval)->next = (GENLIB_yyvsp[-1].pinval); (GENLIB_yyval.pinval) = (GENLIB_yyvsp[0].pinval);}
#line 1353 "y.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 85 "readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_yyval.pinval) = ALLOC(pin_info_t, 1);
			(GENLIB_yyval.pinval)->name = (GENLIB_yyvsp[-7].strval);
			(GENLIB_yyval.pinval)->phase = (GENLIB_yyvsp[-6].strval);
			(GENLIB_yyval.pinval)->value[0] = (GENLIB_yyvsp[-5].realval); (GENLIB_yyval.pinval)->value[1] = (GENLIB_yyvsp[-4].realval);
			(GENLIB_yyval.pinval)->value[2] = (GENLIB_yyvsp[-3].realval); (GENLIB_yyval.pinval)->value[3] = (GENLIB_yyvsp[-2].realval);
			(GENLIB_yyval.pinval)->value[4] = (GENLIB_yyvsp[-1].realval); (GENLIB_yyval.pinval)->value[5] = (GENLIB_yyvsp[0].realval);
			(GENLIB_yyval.pinval)->next = 0;
		    }
#line 1367 "y.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 97 "readlib.y" /* yacc.c:1646  */
    {
			if (strcmp((GENLIB_yyvsp[0].strval), "INV") != 0) {
			    if (strcmp((GENLIB_yyvsp[0].strval), "NONINV") != 0) {
				if (strcmp((GENLIB_yyvsp[0].strval), "UNKNOWN") != 0) {
				    GENLIB_yyerror("bad pin phase");
				}
			    }
			}
			(GENLIB_yyval.strval) = (GENLIB_yyvsp[0].strval);
		    }
#line 1382 "y.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 110 "readlib.y" /* yacc.c:1646  */
    { (GENLIB_yyval.strval) = (GENLIB_yyvsp[0].strval); }
#line 1388 "y.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 112 "readlib.y" /* yacc.c:1646  */
    { (GENLIB_yyval.strval) = util_strsav(GENLIB_yytext); }
#line 1394 "y.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 116 "readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_yyval.functionval) = ALLOC(function_t, 1);
			(GENLIB_yyval.functionval)->name = (GENLIB_yyvsp[-3].strval);
			(GENLIB_yyval.functionval)->tree = (GENLIB_yyvsp[-1].nodeval);
			(GENLIB_yyval.functionval)->next = 0;
		    }
#line 1405 "y.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 125 "readlib.y" /* yacc.c:1646  */
    { 
			(GENLIB_yyval.nodeval) = gl_alloc_node(2);
			(GENLIB_yyval.nodeval)->phase = 1;
			(GENLIB_yyval.nodeval)->sons[0] = (GENLIB_yyvsp[-2].nodeval);
			(GENLIB_yyval.nodeval)->sons[1] = (GENLIB_yyvsp[0].nodeval);
			(GENLIB_yyval.nodeval)->type = OR_NODE;
		    }
#line 1417 "y.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 133 "readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_yyval.nodeval) = gl_alloc_node(2);
			(GENLIB_yyval.nodeval)->phase = 1;
			(GENLIB_yyval.nodeval)->sons[0] = (GENLIB_yyvsp[-2].nodeval);
			(GENLIB_yyval.nodeval)->sons[1] = (GENLIB_yyvsp[0].nodeval);
			(GENLIB_yyval.nodeval)->type = AND_NODE;
		    }
#line 1429 "y.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 141 "readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_yyval.nodeval) = gl_alloc_node(2);
			(GENLIB_yyval.nodeval)->phase = 1;
			(GENLIB_yyval.nodeval)->sons[0] = (GENLIB_yyvsp[-1].nodeval);
			(GENLIB_yyval.nodeval)->sons[1] = (GENLIB_yyvsp[0].nodeval);
			(GENLIB_yyval.nodeval)->type = AND_NODE;
		    }
#line 1441 "y.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 149 "readlib.y" /* yacc.c:1646  */
    { 
			(GENLIB_yyval.nodeval) = (GENLIB_yyvsp[-1].nodeval); 
		    }
#line 1449 "y.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 153 "readlib.y" /* yacc.c:1646  */
    { 
			(GENLIB_yyval.nodeval) = (GENLIB_yyvsp[0].nodeval); 
			(GENLIB_yyval.nodeval)->phase = 0; 
		    }
#line 1458 "y.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 158 "readlib.y" /* yacc.c:1646  */
    { 
			(GENLIB_yyval.nodeval) = (GENLIB_yyvsp[-1].nodeval); 
			(GENLIB_yyval.nodeval)->phase = 0; 
		    }
#line 1467 "y.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 163 "readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_yyval.nodeval) = gl_alloc_leaf((GENLIB_yyvsp[0].strval));
			FREE((GENLIB_yyvsp[0].strval));
			(GENLIB_yyval.nodeval)->type = LEAF_NODE;	/* hack */
			(GENLIB_yyval.nodeval)->phase = 1;
		    }
#line 1478 "y.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 170 "readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_yyval.nodeval) = gl_alloc_leaf("0");
			(GENLIB_yyval.nodeval)->phase = 1;
			(GENLIB_yyval.nodeval)->type = ZERO_NODE;
		    }
#line 1488 "y.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 176 "readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_yyval.nodeval) = gl_alloc_leaf("1");
			(GENLIB_yyval.nodeval)->phase = 1;
			(GENLIB_yyval.nodeval)->type = ONE_NODE;
		    }
#line 1498 "y.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 184 "readlib.y" /* yacc.c:1646  */
    { (GENLIB_yyval.strval) = util_strsav(GENLIB_yytext); }
#line 1504 "y.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 187 "readlib.y" /* yacc.c:1646  */
    { (GENLIB_yyval.strval) = util_strsav(GENLIB_yytext); }
#line 1510 "y.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 191 "readlib.y" /* yacc.c:1646  */
    { (GENLIB_yyval.realval) = atof(GENLIB_yytext); }
#line 1516 "y.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 195 "readlib.y" /* yacc.c:1646  */
    { (GENLIB_yyval.pinval) = 0;}
#line 1522 "y.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 197 "readlib.y" /* yacc.c:1646  */
    { (GENLIB_yyval.pinval) = ALLOC(pin_info_t, 1);
		  (GENLIB_yyval.pinval)->name = (GENLIB_yyvsp[-6].strval);
		  (GENLIB_yyval.pinval)->phase = util_strsav("UNKNOWN");
		  (GENLIB_yyval.pinval)->value[0] = (GENLIB_yyvsp[-5].realval);
		  (GENLIB_yyval.pinval)->value[1] = (GENLIB_yyvsp[-4].realval);
		  (GENLIB_yyval.pinval)->value[2] = (GENLIB_yyvsp[-3].realval);
		  (GENLIB_yyval.pinval)->value[3] = (GENLIB_yyvsp[-2].realval);
		  (GENLIB_yyval.pinval)->value[4] = (GENLIB_yyvsp[-1].realval);
		  (GENLIB_yyval.pinval)->value[5] = (GENLIB_yyvsp[0].realval);
		  (GENLIB_yyval.pinval)->next = 0;
		}
#line 1538 "y.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 211 "readlib.y" /* yacc.c:1646  */
    { (GENLIB_yyval.constrval) = 0; }
#line 1544 "y.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 213 "readlib.y" /* yacc.c:1646  */
    {
		  (GENLIB_yyvsp[0].constrval)->next = (GENLIB_yyvsp[-1].constrval);
		  (GENLIB_yyval.constrval) = (GENLIB_yyvsp[0].constrval);
		}
#line 1553 "y.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 220 "readlib.y" /* yacc.c:1646  */
    {
		  (GENLIB_yyval.constrval) = ALLOC(constraint_info_t, 1);
		  (GENLIB_yyval.constrval)->name = (GENLIB_yyvsp[-2].strval);
		  (GENLIB_yyval.constrval)->setup = (GENLIB_yyvsp[-1].realval);
		  (GENLIB_yyval.constrval)->hold = (GENLIB_yyvsp[0].realval);
		  (GENLIB_yyval.constrval)->next = 0;
		}
#line 1565 "y.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 230 "readlib.y" /* yacc.c:1646  */
    { 
		  (GENLIB_yyval.latchval) = ALLOC(latch_info_t, 1);
		  (GENLIB_yyval.latchval)->in = (GENLIB_yyvsp[-2].strval);
		  (GENLIB_yyval.latchval)->out = (GENLIB_yyvsp[-1].strval);
		  (GENLIB_yyval.latchval)->type = (GENLIB_yyvsp[0].strval);
		}
#line 1576 "y.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 239 "readlib.y" /* yacc.c:1646  */
    { 
		  if (strcmp(GENLIB_yytext, "RISING_EDGE") == 0) {
		    (GENLIB_yyval.strval) = util_strsav("re");
		  } else if (strcmp(GENLIB_yytext, "FALLING_EDGE") == 0) {
		    (GENLIB_yyval.strval) = util_strsav("fe");
		  } else if (strcmp(GENLIB_yytext, "ACTIVE_HIGH") == 0) {
		    (GENLIB_yyval.strval) = util_strsav("ah");
		  } else if (strcmp(GENLIB_yytext, "ACTIVE_LOW") == 0) {
		    (GENLIB_yyval.strval) = util_strsav("al");
		  } else if (strcmp(GENLIB_yytext, "ASYNCH") == 0) {
		    (GENLIB_yyval.strval) = util_strsav("as");
		  } else {
		    GENLIB_yyerror("latch type can only be re/fe/ah/al/as");
		  }
		}
#line 1596 "y.tab.c" /* yacc.c:1646  */
    break;


#line 1600 "y.tab.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter GENLIB_yychar, and that requires
     that GENLIB_yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of GENLIB_yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering GENLIB_yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", GENLIB_yyr1[GENLIB_yyn], &GENLIB_yyval, &GENLIB_yyloc);

  YYPOPSTACK (GENLIB_yylen);
  GENLIB_yylen = 0;
  YY_STACK_PRINT (GENLIB_yyss, GENLIB_yyssp);

  *++GENLIB_yyvsp = GENLIB_yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  GENLIB_yyn = GENLIB_yyr1[GENLIB_yyn];

  GENLIB_yystate = GENLIB_yypgoto[GENLIB_yyn - YYNTOKENS] + *GENLIB_yyssp;
  if (0 <= GENLIB_yystate && GENLIB_yystate <= YYLAST && GENLIB_yycheck[GENLIB_yystate] == *GENLIB_yyssp)
    GENLIB_yystate = GENLIB_yytable[GENLIB_yystate];
  else
    GENLIB_yystate = GENLIB_yydefgoto[GENLIB_yyn - YYNTOKENS];

  goto GENLIB_yynewstate;


/*--------------------------------------.
| GENLIB_yyerrlab -- here on detecting error.  |
`--------------------------------------*/
GENLIB_yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  GENLIB_yytoken = GENLIB_yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (GENLIB_yychar);

  /* If not already recovering from an error, report this error.  */
  if (!GENLIB_yyerrstatus)
    {
      ++GENLIB_yynerrs;
#if ! YYERROR_VERBOSE
      GENLIB_yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR GENLIB_yysyntax_error (&GENLIB_yymsg_alloc, &GENLIB_yymsg, \
                                        GENLIB_yyssp, GENLIB_yytoken)
      {
        char const *GENLIB_yymsgp = YY_("syntax error");
        int GENLIB_yysyntax_error_status;
        GENLIB_yysyntax_error_status = YYSYNTAX_ERROR;
        if (GENLIB_yysyntax_error_status == 0)
          GENLIB_yymsgp = GENLIB_yymsg;
        else if (GENLIB_yysyntax_error_status == 1)
          {
            if (GENLIB_yymsg != GENLIB_yymsgbuf)
              YYSTACK_FREE (GENLIB_yymsg);
            GENLIB_yymsg = (char *) YYSTACK_ALLOC (GENLIB_yymsg_alloc);
            if (!GENLIB_yymsg)
              {
                GENLIB_yymsg = GENLIB_yymsgbuf;
                GENLIB_yymsg_alloc = sizeof GENLIB_yymsgbuf;
                GENLIB_yysyntax_error_status = 2;
              }
            else
              {
                GENLIB_yysyntax_error_status = YYSYNTAX_ERROR;
                GENLIB_yymsgp = GENLIB_yymsg;
              }
          }
        GENLIB_yyerror (GENLIB_yymsgp);
        if (GENLIB_yysyntax_error_status == 2)
          goto GENLIB_yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (GENLIB_yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (GENLIB_yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (GENLIB_yychar == YYEOF)
            YYABORT;
        }
      else
        {
          GENLIB_yydestruct ("Error: discarding",
                      GENLIB_yytoken, &GENLIB_yylval);
          GENLIB_yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto GENLIB_yyerrlab1;


/*---------------------------------------------------.
| GENLIB_yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
GENLIB_yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label GENLIB_yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto GENLIB_yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (GENLIB_yylen);
  GENLIB_yylen = 0;
  YY_STACK_PRINT (GENLIB_yyss, GENLIB_yyssp);
  GENLIB_yystate = *GENLIB_yyssp;
  goto GENLIB_yyerrlab1;


/*-------------------------------------------------------------.
| GENLIB_yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
GENLIB_yyerrlab1:
  GENLIB_yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      GENLIB_yyn = GENLIB_yypact[GENLIB_yystate];
      if (!GENLIB_yypact_value_is_default (GENLIB_yyn))
        {
          GENLIB_yyn += YYTERROR;
          if (0 <= GENLIB_yyn && GENLIB_yyn <= YYLAST && GENLIB_yycheck[GENLIB_yyn] == YYTERROR)
            {
              GENLIB_yyn = GENLIB_yytable[GENLIB_yyn];
              if (0 < GENLIB_yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (GENLIB_yyssp == GENLIB_yyss)
        YYABORT;


      GENLIB_yydestruct ("Error: popping",
                  GENLIB_yystos[GENLIB_yystate], GENLIB_yyvsp);
      YYPOPSTACK (1);
      GENLIB_yystate = *GENLIB_yyssp;
      YY_STACK_PRINT (GENLIB_yyss, GENLIB_yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++GENLIB_yyvsp = GENLIB_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", GENLIB_yystos[GENLIB_yyn], GENLIB_yyvsp, GENLIB_yylsp);

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

#if !defined GENLIB_yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| GENLIB_yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
GENLIB_yyexhaustedlab:
  GENLIB_yyerror (YY_("memory exhausted"));
  GENLIB_yyresult = 2;
  /* Fall through.  */
#endif

GENLIB_yyreturn:
  if (GENLIB_yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      GENLIB_yytoken = YYTRANSLATE (GENLIB_yychar);
      GENLIB_yydestruct ("Cleanup: discarding lookahead",
                  GENLIB_yytoken, &GENLIB_yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (GENLIB_yylen);
  YY_STACK_PRINT (GENLIB_yyss, GENLIB_yyssp);
  while (GENLIB_yyssp != GENLIB_yyss)
    {
      GENLIB_yydestruct ("Cleanup: popping",
                  GENLIB_yystos[*GENLIB_yyssp], GENLIB_yyvsp);
      YYPOPSTACK (1);
    }
#ifndef GENLIB_yyoverflow
  if (GENLIB_yyss != GENLIB_yyssa)
    YYSTACK_FREE (GENLIB_yyss);
#endif
#if YYERROR_VERBOSE
  if (GENLIB_yymsg != GENLIB_yymsgbuf)
    YYSTACK_FREE (GENLIB_yymsg);
#endif
  return GENLIB_yyresult;
}
#line 256 "readlib.y" /* yacc.c:1906  */

static jmp_buf jmpbuf;

int
GENLIB_yyerror(errmsg)
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
	(void) GENLIB_yyparse();
	if (global_fct != 0) GENLIB_yyerror("syntax error");
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
	(void) GENLIB_yyparse();
	if (global_fct != 0) GENLIB_yyerror("syntax error");
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
	(void) GENLIB_yyparse();
	if (global_fct == 0) GENLIB_yyerror("syntax error");
	*tree = global_fct->tree;
	*name = global_fct->name;
	return 1;
    }
}
