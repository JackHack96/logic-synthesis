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

/* All symbols defined below should begin with GENLIB_GENLIB_GENLIB_GENLIB_yy or YY, to avoid
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
#line 2 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:339  */

/* file @(#)readlib.y	1.2                      */
/* last modified on 6/13/91 at 17:46:40   */
#include <setjmp.h>
#include "genlib_int.h"

#undef GENLIB_GENLIB_GENLIB_GENLIB_yywrap 
static int input();
static int unput();
static int GENLIB_GENLIB_GENLIB_GENLIB_yywrap();

FILE *gl_out_file;
extern char *GENLIB_GENLIB_GENLIB_GENLIB_yytext;

static int global_use_nor;
static function_t *global_fct;

extern int library_setup_string(char *);
extern int library_setup_file(FILE *, char *);


#line 88 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:339  */

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
   by #include "readlib.h".  */
#ifndef YY_YY_HOME_MATTEO_DROPBOX_SIS_SIS_GENLIB_READLIB_H_INCLUDED
# define YY_YY_HOME_MATTEO_DROPBOX_SIS_SIS_GENLIB_READLIB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int GENLIB_GENLIB_GENLIB_GENLIB_yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum GENLIB_GENLIB_GENLIB_GENLIB_yytokentype
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

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 24 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:355  */

    tree_node_t *nodeval;
    char *strval;
    double realval; 
    pin_info_t *pinval;
    function_t *functionval;
    latch_info_t *latchval;
    constraint_info_t *constrval;

#line 159 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE GENLIB_GENLIB_GENLIB_GENLIB_yylval;

int GENLIB_GENLIB_GENLIB_GENLIB_yyparse (void);

#endif /* !YY_YY_HOME_MATTEO_DROPBOX_SIS_SIS_GENLIB_READLIB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 176 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint8;
#else
typedef unsigned char GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 GENLIB_GENLIB_GENLIB_GENLIB_yytype_int8;
#else
typedef signed char GENLIB_GENLIB_GENLIB_GENLIB_yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint16;
#else
typedef unsigned short int GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16;
#else
typedef short int GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16;
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
/* Suppress an incorrect diagnostic about GENLIB_GENLIB_GENLIB_GENLIB_yylval being uninitialized.  */
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


#if ! defined GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow || YYERROR_VERBOSE */


#if (! defined GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union GENLIB_GENLIB_GENLIB_GENLIB_yyalloc
{
  GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16 GENLIB_GENLIB_GENLIB_GENLIB_yyss_alloc;
  YYSTYPE GENLIB_GENLIB_GENLIB_GENLIB_yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union GENLIB_GENLIB_GENLIB_GENLIB_yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16) + sizeof (YYSTYPE)) \
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
        YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yynewbytes;                                            \
        YYCOPY (&GENLIB_GENLIB_GENLIB_GENLIB_yyptr->Stack_alloc, Stack, GENLIB_GENLIB_GENLIB_GENLIB_yysize);                    \
        Stack = &GENLIB_GENLIB_GENLIB_GENLIB_yyptr->Stack_alloc;                                    \
        GENLIB_GENLIB_GENLIB_GENLIB_yynewbytes = GENLIB_GENLIB_GENLIB_GENLIB_yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        GENLIB_GENLIB_GENLIB_GENLIB_yyptr += GENLIB_GENLIB_GENLIB_GENLIB_yynewbytes / sizeof (*GENLIB_GENLIB_GENLIB_GENLIB_yyptr);                          \
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
          YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yyi;                         \
          for (GENLIB_GENLIB_GENLIB_GENLIB_yyi = 0; GENLIB_GENLIB_GENLIB_GENLIB_yyi < (Count); GENLIB_GENLIB_GENLIB_GENLIB_yyi++)   \
            (Dst)[GENLIB_GENLIB_GENLIB_GENLIB_yyi] = (Src)[GENLIB_GENLIB_GENLIB_GENLIB_yyi];            \
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
   by GENLIB_GENLIB_GENLIB_GENLIB_yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   275

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? GENLIB_GENLIB_GENLIB_GENLIB_yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by GENLIB_GENLIB_GENLIB_GENLIB_yylex, without out-of-bounds checking.  */
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint8 GENLIB_GENLIB_GENLIB_GENLIB_yytranslate[] =
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
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint8 GENLIB_GENLIB_GENLIB_GENLIB_yyrline[] =
{
       0,    52,    52,    53,    57,    58,    61,    66,    74,    75,
      79,    91,   104,   106,   110,   119,   127,   135,   143,   147,
     152,   157,   164,   170,   178,   181,   185,   190,   191,   206,
     207,   214,   224,   233
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const GENLIB_GENLIB_GENLIB_GENLIB_yytname[] =
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
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint16 GENLIB_GENLIB_GENLIB_GENLIB_yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275
};
# endif

#define YYPACT_NINF -11

#define GENLIB_GENLIB_GENLIB_GENLIB_yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-11)))

#define YYTABLE_NINF -1

#define GENLIB_GENLIB_GENLIB_GENLIB_yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_int8 GENLIB_GENLIB_GENLIB_GENLIB_yypact[] =
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
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint8 GENLIB_GENLIB_GENLIB_GENLIB_yydefact[] =
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
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_int8 GENLIB_GENLIB_GENLIB_GENLIB_yypgoto[] =
{
     -11,   -11,   -11,   -11,    16,   -11,   -11,    -6,    12,   -10,
       0,    -1,   -11,   -11,   -11,   -11,   -11
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_int8 GENLIB_GENLIB_GENLIB_GENLIB_yydefgoto[] =
{
      -1,     3,     4,    10,    35,    38,    47,    42,     5,    29,
      19,    21,    46,    51,    57,    40,    54
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint8 GENLIB_GENLIB_GENLIB_GENLIB_yytable[] =
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

static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_int8 GENLIB_GENLIB_GENLIB_GENLIB_yycheck[] =
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
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint8 GENLIB_GENLIB_GENLIB_GENLIB_yystos[] =
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
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint8 GENLIB_GENLIB_GENLIB_GENLIB_yyr1[] =
{
       0,    21,    22,    22,    23,    23,    24,    24,    25,    25,
      26,    27,    28,    28,    29,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    31,    31,    32,    33,    33,    34,
      34,    35,    36,    37
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const GENLIB_GENLIB_GENLIB_GENLIB_yytype_uint8 GENLIB_GENLIB_GENLIB_GENLIB_yyr2[] =
{
       0,     2,     1,     1,     0,     2,     5,     8,     0,     2,
       9,     1,     1,     1,     4,     3,     3,     2,     3,     2,
       2,     1,     1,     1,     1,     1,     1,     0,     8,     0,
       2,     4,     4,     1
};


#define GENLIB_GENLIB_GENLIB_GENLIB_yyerrok         (GENLIB_GENLIB_GENLIB_GENLIB_yyerrstatus = 0)
#define GENLIB_GENLIB_GENLIB_GENLIB_yyclearin       (GENLIB_GENLIB_GENLIB_GENLIB_yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto GENLIB_GENLIB_GENLIB_GENLIB_yyacceptlab
#define YYABORT         goto GENLIB_GENLIB_GENLIB_GENLIB_yyabortlab
#define YYERROR         goto GENLIB_GENLIB_GENLIB_GENLIB_yyerrorlab


#define YYRECOVERING()  (!!GENLIB_GENLIB_GENLIB_GENLIB_yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (GENLIB_GENLIB_GENLIB_GENLIB_yychar == YYEMPTY)                                        \
    {                                                           \
      GENLIB_GENLIB_GENLIB_GENLIB_yychar = (Token);                                         \
      GENLIB_GENLIB_GENLIB_GENLIB_yylval = (Value);                                         \
      YYPOPSTACK (GENLIB_GENLIB_GENLIB_GENLIB_yylen);                                       \
      GENLIB_GENLIB_GENLIB_GENLIB_yystate = *GENLIB_GENLIB_GENLIB_GENLIB_yyssp;                                         \
      goto GENLIB_GENLIB_GENLIB_GENLIB_yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      GENLIB_GENLIB_GENLIB_GENLIB_yyerror (YY_("syntax error: cannot back up")); \
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
  if (GENLIB_GENLIB_GENLIB_GENLIB_yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (GENLIB_GENLIB_GENLIB_GENLIB_yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      GENLIB_GENLIB_GENLIB_GENLIB_yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
GENLIB_GENLIB_GENLIB_GENLIB_yy_symbol_value_print (FILE *GENLIB_GENLIB_GENLIB_GENLIB_yyoutput, int GENLIB_GENLIB_GENLIB_GENLIB_yytype, YYSTYPE const * const GENLIB_GENLIB_GENLIB_GENLIB_yyvaluep)
{
  FILE *GENLIB_GENLIB_GENLIB_GENLIB_yyo = GENLIB_GENLIB_GENLIB_GENLIB_yyoutput;
  YYUSE (GENLIB_GENLIB_GENLIB_GENLIB_yyo);
  if (!GENLIB_GENLIB_GENLIB_GENLIB_yyvaluep)
    return;
# ifdef YYPRINT
  if (GENLIB_GENLIB_GENLIB_GENLIB_yytype < YYNTOKENS)
    YYPRINT (GENLIB_GENLIB_GENLIB_GENLIB_yyoutput, GENLIB_GENLIB_GENLIB_GENLIB_yytoknum[GENLIB_GENLIB_GENLIB_GENLIB_yytype], *GENLIB_GENLIB_GENLIB_GENLIB_yyvaluep);
# endif
  YYUSE (GENLIB_GENLIB_GENLIB_GENLIB_yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
GENLIB_GENLIB_GENLIB_GENLIB_yy_symbol_print (FILE *GENLIB_GENLIB_GENLIB_GENLIB_yyoutput, int GENLIB_GENLIB_GENLIB_GENLIB_yytype, YYSTYPE const * const GENLIB_GENLIB_GENLIB_GENLIB_yyvaluep)
{
  YYFPRINTF (GENLIB_GENLIB_GENLIB_GENLIB_yyoutput, "%s %s (",
             GENLIB_GENLIB_GENLIB_GENLIB_yytype < YYNTOKENS ? "token" : "nterm", GENLIB_GENLIB_GENLIB_GENLIB_yytname[GENLIB_GENLIB_GENLIB_GENLIB_yytype]);

  GENLIB_GENLIB_GENLIB_GENLIB_yy_symbol_value_print (GENLIB_GENLIB_GENLIB_GENLIB_yyoutput, GENLIB_GENLIB_GENLIB_GENLIB_yytype, GENLIB_GENLIB_GENLIB_GENLIB_yyvaluep);
  YYFPRINTF (GENLIB_GENLIB_GENLIB_GENLIB_yyoutput, ")");
}

/*------------------------------------------------------------------.
| GENLIB_GENLIB_GENLIB_GENLIB_yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
GENLIB_GENLIB_GENLIB_GENLIB_yy_stack_print (GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16 *GENLIB_GENLIB_GENLIB_GENLIB_yybottom, GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16 *GENLIB_GENLIB_GENLIB_GENLIB_yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; GENLIB_GENLIB_GENLIB_GENLIB_yybottom <= GENLIB_GENLIB_GENLIB_GENLIB_yytop; GENLIB_GENLIB_GENLIB_GENLIB_yybottom++)
    {
      int GENLIB_GENLIB_GENLIB_GENLIB_yybot = *GENLIB_GENLIB_GENLIB_GENLIB_yybottom;
      YYFPRINTF (stderr, " %d", GENLIB_GENLIB_GENLIB_GENLIB_yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (GENLIB_GENLIB_GENLIB_GENLIB_yydebug)                                                  \
    GENLIB_GENLIB_GENLIB_GENLIB_yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
GENLIB_GENLIB_GENLIB_GENLIB_yy_reduce_print (GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16 *GENLIB_GENLIB_GENLIB_GENLIB_yyssp, YYSTYPE *GENLIB_GENLIB_GENLIB_GENLIB_yyvsp, int GENLIB_GENLIB_GENLIB_GENLIB_yyrule)
{
  unsigned long int GENLIB_GENLIB_GENLIB_GENLIB_yylno = GENLIB_GENLIB_GENLIB_GENLIB_yyrline[GENLIB_GENLIB_GENLIB_GENLIB_yyrule];
  int GENLIB_GENLIB_GENLIB_GENLIB_yynrhs = GENLIB_GENLIB_GENLIB_GENLIB_yyr2[GENLIB_GENLIB_GENLIB_GENLIB_yyrule];
  int GENLIB_GENLIB_GENLIB_GENLIB_yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             GENLIB_GENLIB_GENLIB_GENLIB_yyrule - 1, GENLIB_GENLIB_GENLIB_GENLIB_yylno);
  /* The symbols being reduced.  */
  for (GENLIB_GENLIB_GENLIB_GENLIB_yyi = 0; GENLIB_GENLIB_GENLIB_GENLIB_yyi < GENLIB_GENLIB_GENLIB_GENLIB_yynrhs; GENLIB_GENLIB_GENLIB_GENLIB_yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", GENLIB_GENLIB_GENLIB_GENLIB_yyi + 1);
      GENLIB_GENLIB_GENLIB_GENLIB_yy_symbol_print (stderr,
                       GENLIB_GENLIB_GENLIB_GENLIB_yystos[GENLIB_GENLIB_GENLIB_GENLIB_yyssp[GENLIB_GENLIB_GENLIB_GENLIB_yyi + 1 - GENLIB_GENLIB_GENLIB_GENLIB_yynrhs]],
                       &(GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[(GENLIB_GENLIB_GENLIB_GENLIB_yyi + 1) - (GENLIB_GENLIB_GENLIB_GENLIB_yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (GENLIB_GENLIB_GENLIB_GENLIB_yydebug)                          \
    GENLIB_GENLIB_GENLIB_GENLIB_yy_reduce_print (GENLIB_GENLIB_GENLIB_GENLIB_yyssp, GENLIB_GENLIB_GENLIB_GENLIB_yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int GENLIB_GENLIB_GENLIB_GENLIB_yydebug;
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

# ifndef GENLIB_GENLIB_GENLIB_GENLIB_yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define GENLIB_GENLIB_GENLIB_GENLIB_yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
GENLIB_GENLIB_GENLIB_GENLIB_yystrlen (const char *GENLIB_GENLIB_GENLIB_GENLIB_yystr)
{
  YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yylen;
  for (GENLIB_GENLIB_GENLIB_GENLIB_yylen = 0; GENLIB_GENLIB_GENLIB_GENLIB_yystr[GENLIB_GENLIB_GENLIB_GENLIB_yylen]; GENLIB_GENLIB_GENLIB_GENLIB_yylen++)
    continue;
  return GENLIB_GENLIB_GENLIB_GENLIB_yylen;
}
#  endif
# endif

# ifndef GENLIB_GENLIB_GENLIB_GENLIB_yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define GENLIB_GENLIB_GENLIB_GENLIB_yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
GENLIB_GENLIB_GENLIB_GENLIB_yystpcpy (char *GENLIB_GENLIB_GENLIB_GENLIB_yydest, const char *GENLIB_GENLIB_GENLIB_GENLIB_yysrc)
{
  char *GENLIB_GENLIB_GENLIB_GENLIB_yyd = GENLIB_GENLIB_GENLIB_GENLIB_yydest;
  const char *GENLIB_GENLIB_GENLIB_GENLIB_yys = GENLIB_GENLIB_GENLIB_GENLIB_yysrc;

  while ((*GENLIB_GENLIB_GENLIB_GENLIB_yyd++ = *GENLIB_GENLIB_GENLIB_GENLIB_yys++) != '\0')
    continue;

  return GENLIB_GENLIB_GENLIB_GENLIB_yyd - 1;
}
#  endif
# endif

# ifndef GENLIB_GENLIB_GENLIB_GENLIB_yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for GENLIB_GENLIB_GENLIB_GENLIB_yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from GENLIB_GENLIB_GENLIB_GENLIB_yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
GENLIB_GENLIB_GENLIB_GENLIB_yytnamerr (char *GENLIB_GENLIB_GENLIB_GENLIB_yyres, const char *GENLIB_GENLIB_GENLIB_GENLIB_yystr)
{
  if (*GENLIB_GENLIB_GENLIB_GENLIB_yystr == '"')
    {
      YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yyn = 0;
      char const *GENLIB_GENLIB_GENLIB_GENLIB_yyp = GENLIB_GENLIB_GENLIB_GENLIB_yystr;

      for (;;)
        switch (*++GENLIB_GENLIB_GENLIB_GENLIB_yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++GENLIB_GENLIB_GENLIB_GENLIB_yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (GENLIB_GENLIB_GENLIB_GENLIB_yyres)
              GENLIB_GENLIB_GENLIB_GENLIB_yyres[GENLIB_GENLIB_GENLIB_GENLIB_yyn] = *GENLIB_GENLIB_GENLIB_GENLIB_yyp;
            GENLIB_GENLIB_GENLIB_GENLIB_yyn++;
            break;

          case '"':
            if (GENLIB_GENLIB_GENLIB_GENLIB_yyres)
              GENLIB_GENLIB_GENLIB_GENLIB_yyres[GENLIB_GENLIB_GENLIB_GENLIB_yyn] = '\0';
            return GENLIB_GENLIB_GENLIB_GENLIB_yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! GENLIB_GENLIB_GENLIB_GENLIB_yyres)
    return GENLIB_GENLIB_GENLIB_GENLIB_yystrlen (GENLIB_GENLIB_GENLIB_GENLIB_yystr);

  return GENLIB_GENLIB_GENLIB_GENLIB_yystpcpy (GENLIB_GENLIB_GENLIB_GENLIB_yyres, GENLIB_GENLIB_GENLIB_GENLIB_yystr) - GENLIB_GENLIB_GENLIB_GENLIB_yyres;
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
GENLIB_GENLIB_GENLIB_GENLIB_yysyntax_error (YYSIZE_T *GENLIB_GENLIB_GENLIB_GENLIB_yymsg_alloc, char **GENLIB_GENLIB_GENLIB_GENLIB_yymsg,
                GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16 *GENLIB_GENLIB_GENLIB_GENLIB_yyssp, int GENLIB_GENLIB_GENLIB_GENLIB_yytoken)
{
  YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yysize0 = GENLIB_GENLIB_GENLIB_GENLIB_yytnamerr (YY_NULLPTR, GENLIB_GENLIB_GENLIB_GENLIB_yytname[GENLIB_GENLIB_GENLIB_GENLIB_yytoken]);
  YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yysize = GENLIB_GENLIB_GENLIB_GENLIB_yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *GENLIB_GENLIB_GENLIB_GENLIB_yyformat = YY_NULLPTR;
  /* Arguments of GENLIB_GENLIB_GENLIB_GENLIB_yyformat. */
  char const *GENLIB_GENLIB_GENLIB_GENLIB_yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int GENLIB_GENLIB_GENLIB_GENLIB_yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in GENLIB_GENLIB_GENLIB_GENLIB_yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated GENLIB_GENLIB_GENLIB_GENLIB_yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (GENLIB_GENLIB_GENLIB_GENLIB_yytoken != YYEMPTY)
    {
      int GENLIB_GENLIB_GENLIB_GENLIB_yyn = GENLIB_GENLIB_GENLIB_GENLIB_yypact[*GENLIB_GENLIB_GENLIB_GENLIB_yyssp];
      GENLIB_GENLIB_GENLIB_GENLIB_yyarg[GENLIB_GENLIB_GENLIB_GENLIB_yycount++] = GENLIB_GENLIB_GENLIB_GENLIB_yytname[GENLIB_GENLIB_GENLIB_GENLIB_yytoken];
      if (!GENLIB_GENLIB_GENLIB_GENLIB_yypact_value_is_default (GENLIB_GENLIB_GENLIB_GENLIB_yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int GENLIB_GENLIB_GENLIB_GENLIB_yyxbegin = GENLIB_GENLIB_GENLIB_GENLIB_yyn < 0 ? -GENLIB_GENLIB_GENLIB_GENLIB_yyn : 0;
          /* Stay within bounds of both GENLIB_GENLIB_GENLIB_GENLIB_yycheck and GENLIB_GENLIB_GENLIB_GENLIB_yytname.  */
          int GENLIB_GENLIB_GENLIB_GENLIB_yychecklim = YYLAST - GENLIB_GENLIB_GENLIB_GENLIB_yyn + 1;
          int GENLIB_GENLIB_GENLIB_GENLIB_yyxend = GENLIB_GENLIB_GENLIB_GENLIB_yychecklim < YYNTOKENS ? GENLIB_GENLIB_GENLIB_GENLIB_yychecklim : YYNTOKENS;
          int GENLIB_GENLIB_GENLIB_GENLIB_yyx;

          for (GENLIB_GENLIB_GENLIB_GENLIB_yyx = GENLIB_GENLIB_GENLIB_GENLIB_yyxbegin; GENLIB_GENLIB_GENLIB_GENLIB_yyx < GENLIB_GENLIB_GENLIB_GENLIB_yyxend; ++GENLIB_GENLIB_GENLIB_GENLIB_yyx)
            if (GENLIB_GENLIB_GENLIB_GENLIB_yycheck[GENLIB_GENLIB_GENLIB_GENLIB_yyx + GENLIB_GENLIB_GENLIB_GENLIB_yyn] == GENLIB_GENLIB_GENLIB_GENLIB_yyx && GENLIB_GENLIB_GENLIB_GENLIB_yyx != YYTERROR
                && !GENLIB_GENLIB_GENLIB_GENLIB_yytable_value_is_error (GENLIB_GENLIB_GENLIB_GENLIB_yytable[GENLIB_GENLIB_GENLIB_GENLIB_yyx + GENLIB_GENLIB_GENLIB_GENLIB_yyn]))
              {
                if (GENLIB_GENLIB_GENLIB_GENLIB_yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    GENLIB_GENLIB_GENLIB_GENLIB_yycount = 1;
                    GENLIB_GENLIB_GENLIB_GENLIB_yysize = GENLIB_GENLIB_GENLIB_GENLIB_yysize0;
                    break;
                  }
                GENLIB_GENLIB_GENLIB_GENLIB_yyarg[GENLIB_GENLIB_GENLIB_GENLIB_yycount++] = GENLIB_GENLIB_GENLIB_GENLIB_yytname[GENLIB_GENLIB_GENLIB_GENLIB_yyx];
                {
                  YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yysize1 = GENLIB_GENLIB_GENLIB_GENLIB_yysize + GENLIB_GENLIB_GENLIB_GENLIB_yytnamerr (YY_NULLPTR, GENLIB_GENLIB_GENLIB_GENLIB_yytname[GENLIB_GENLIB_GENLIB_GENLIB_yyx]);
                  if (! (GENLIB_GENLIB_GENLIB_GENLIB_yysize <= GENLIB_GENLIB_GENLIB_GENLIB_yysize1
                         && GENLIB_GENLIB_GENLIB_GENLIB_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  GENLIB_GENLIB_GENLIB_GENLIB_yysize = GENLIB_GENLIB_GENLIB_GENLIB_yysize1;
                }
              }
        }
    }

  switch (GENLIB_GENLIB_GENLIB_GENLIB_yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        GENLIB_GENLIB_GENLIB_GENLIB_yyformat = S;                       \
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
    YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yysize1 = GENLIB_GENLIB_GENLIB_GENLIB_yysize + GENLIB_GENLIB_GENLIB_GENLIB_yystrlen (GENLIB_GENLIB_GENLIB_GENLIB_yyformat);
    if (! (GENLIB_GENLIB_GENLIB_GENLIB_yysize <= GENLIB_GENLIB_GENLIB_GENLIB_yysize1 && GENLIB_GENLIB_GENLIB_GENLIB_yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    GENLIB_GENLIB_GENLIB_GENLIB_yysize = GENLIB_GENLIB_GENLIB_GENLIB_yysize1;
  }

  if (*GENLIB_GENLIB_GENLIB_GENLIB_yymsg_alloc < GENLIB_GENLIB_GENLIB_GENLIB_yysize)
    {
      *GENLIB_GENLIB_GENLIB_GENLIB_yymsg_alloc = 2 * GENLIB_GENLIB_GENLIB_GENLIB_yysize;
      if (! (GENLIB_GENLIB_GENLIB_GENLIB_yysize <= *GENLIB_GENLIB_GENLIB_GENLIB_yymsg_alloc
             && *GENLIB_GENLIB_GENLIB_GENLIB_yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *GENLIB_GENLIB_GENLIB_GENLIB_yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *GENLIB_GENLIB_GENLIB_GENLIB_yyp = *GENLIB_GENLIB_GENLIB_GENLIB_yymsg;
    int GENLIB_GENLIB_GENLIB_GENLIB_yyi = 0;
    while ((*GENLIB_GENLIB_GENLIB_GENLIB_yyp = *GENLIB_GENLIB_GENLIB_GENLIB_yyformat) != '\0')
      if (*GENLIB_GENLIB_GENLIB_GENLIB_yyp == '%' && GENLIB_GENLIB_GENLIB_GENLIB_yyformat[1] == 's' && GENLIB_GENLIB_GENLIB_GENLIB_yyi < GENLIB_GENLIB_GENLIB_GENLIB_yycount)
        {
          GENLIB_GENLIB_GENLIB_GENLIB_yyp += GENLIB_GENLIB_GENLIB_GENLIB_yytnamerr (GENLIB_GENLIB_GENLIB_GENLIB_yyp, GENLIB_GENLIB_GENLIB_GENLIB_yyarg[GENLIB_GENLIB_GENLIB_GENLIB_yyi++]);
          GENLIB_GENLIB_GENLIB_GENLIB_yyformat += 2;
        }
      else
        {
          GENLIB_GENLIB_GENLIB_GENLIB_yyp++;
          GENLIB_GENLIB_GENLIB_GENLIB_yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
GENLIB_GENLIB_GENLIB_GENLIB_yydestruct (const char *GENLIB_GENLIB_GENLIB_GENLIB_yymsg, int GENLIB_GENLIB_GENLIB_GENLIB_yytype, YYSTYPE *GENLIB_GENLIB_GENLIB_GENLIB_yyvaluep)
{
  YYUSE (GENLIB_GENLIB_GENLIB_GENLIB_yyvaluep);
  if (!GENLIB_GENLIB_GENLIB_GENLIB_yymsg)
    GENLIB_GENLIB_GENLIB_GENLIB_yymsg = "Deleting";
  YY_SYMBOL_PRINT (GENLIB_GENLIB_GENLIB_GENLIB_yymsg, GENLIB_GENLIB_GENLIB_GENLIB_yytype, GENLIB_GENLIB_GENLIB_GENLIB_yyvaluep, GENLIB_GENLIB_GENLIB_GENLIB_yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (GENLIB_GENLIB_GENLIB_GENLIB_yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int GENLIB_GENLIB_GENLIB_GENLIB_yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE GENLIB_GENLIB_GENLIB_GENLIB_yylval;
/* Number of syntax errors so far.  */
int GENLIB_GENLIB_GENLIB_GENLIB_yynerrs;


/*----------.
| GENLIB_GENLIB_GENLIB_GENLIB_yyparse.  |
`----------*/

int
GENLIB_GENLIB_GENLIB_GENLIB_yyparse (void)
{
    int GENLIB_GENLIB_GENLIB_GENLIB_yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int GENLIB_GENLIB_GENLIB_GENLIB_yyerrstatus;

    /* The stacks and their tools:
       'GENLIB_GENLIB_GENLIB_GENLIB_yyss': related to states.
       'GENLIB_GENLIB_GENLIB_GENLIB_yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16 GENLIB_GENLIB_GENLIB_GENLIB_yyssa[YYINITDEPTH];
    GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16 *GENLIB_GENLIB_GENLIB_GENLIB_yyss;
    GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16 *GENLIB_GENLIB_GENLIB_GENLIB_yyssp;

    /* The semantic value stack.  */
    YYSTYPE GENLIB_GENLIB_GENLIB_GENLIB_yyvsa[YYINITDEPTH];
    YYSTYPE *GENLIB_GENLIB_GENLIB_GENLIB_yyvs;
    YYSTYPE *GENLIB_GENLIB_GENLIB_GENLIB_yyvsp;

    YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yystacksize;

  int GENLIB_GENLIB_GENLIB_GENLIB_yyn;
  int GENLIB_GENLIB_GENLIB_GENLIB_yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int GENLIB_GENLIB_GENLIB_GENLIB_yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE GENLIB_GENLIB_GENLIB_GENLIB_yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char GENLIB_GENLIB_GENLIB_GENLIB_yymsgbuf[128];
  char *GENLIB_GENLIB_GENLIB_GENLIB_yymsg = GENLIB_GENLIB_GENLIB_GENLIB_yymsgbuf;
  YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yymsg_alloc = sizeof GENLIB_GENLIB_GENLIB_GENLIB_yymsgbuf;
#endif

#define YYPOPSTACK(N)   (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp -= (N), GENLIB_GENLIB_GENLIB_GENLIB_yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int GENLIB_GENLIB_GENLIB_GENLIB_yylen = 0;

  GENLIB_GENLIB_GENLIB_GENLIB_yyssp = GENLIB_GENLIB_GENLIB_GENLIB_yyss = GENLIB_GENLIB_GENLIB_GENLIB_yyssa;
  GENLIB_GENLIB_GENLIB_GENLIB_yyvsp = GENLIB_GENLIB_GENLIB_GENLIB_yyvs = GENLIB_GENLIB_GENLIB_GENLIB_yyvsa;
  GENLIB_GENLIB_GENLIB_GENLIB_yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  GENLIB_GENLIB_GENLIB_GENLIB_yystate = 0;
  GENLIB_GENLIB_GENLIB_GENLIB_yyerrstatus = 0;
  GENLIB_GENLIB_GENLIB_GENLIB_yynerrs = 0;
  GENLIB_GENLIB_GENLIB_GENLIB_yychar = YYEMPTY; /* Cause a token to be read.  */
  goto GENLIB_GENLIB_GENLIB_GENLIB_yysetstate;

/*------------------------------------------------------------.
| GENLIB_GENLIB_GENLIB_GENLIB_yynewstate -- Push a new state, which is found in GENLIB_GENLIB_GENLIB_GENLIB_yystate.  |
`------------------------------------------------------------*/
 GENLIB_GENLIB_GENLIB_GENLIB_yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  GENLIB_GENLIB_GENLIB_GENLIB_yyssp++;

 GENLIB_GENLIB_GENLIB_GENLIB_yysetstate:
  *GENLIB_GENLIB_GENLIB_GENLIB_yyssp = GENLIB_GENLIB_GENLIB_GENLIB_yystate;

  if (GENLIB_GENLIB_GENLIB_GENLIB_yyss + GENLIB_GENLIB_GENLIB_GENLIB_yystacksize - 1 <= GENLIB_GENLIB_GENLIB_GENLIB_yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T GENLIB_GENLIB_GENLIB_GENLIB_yysize = GENLIB_GENLIB_GENLIB_GENLIB_yyssp - GENLIB_GENLIB_GENLIB_GENLIB_yyss + 1;

#ifdef GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *GENLIB_GENLIB_GENLIB_GENLIB_yyvs1 = GENLIB_GENLIB_GENLIB_GENLIB_yyvs;
        GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16 *GENLIB_GENLIB_GENLIB_GENLIB_yyss1 = GENLIB_GENLIB_GENLIB_GENLIB_yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow is a macro.  */
        GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow (YY_("memory exhausted"),
                    &GENLIB_GENLIB_GENLIB_GENLIB_yyss1, GENLIB_GENLIB_GENLIB_GENLIB_yysize * sizeof (*GENLIB_GENLIB_GENLIB_GENLIB_yyssp),
                    &GENLIB_GENLIB_GENLIB_GENLIB_yyvs1, GENLIB_GENLIB_GENLIB_GENLIB_yysize * sizeof (*GENLIB_GENLIB_GENLIB_GENLIB_yyvsp),
                    &GENLIB_GENLIB_GENLIB_GENLIB_yystacksize);

        GENLIB_GENLIB_GENLIB_GENLIB_yyss = GENLIB_GENLIB_GENLIB_GENLIB_yyss1;
        GENLIB_GENLIB_GENLIB_GENLIB_yyvs = GENLIB_GENLIB_GENLIB_GENLIB_yyvs1;
      }
#else /* no GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto GENLIB_GENLIB_GENLIB_GENLIB_yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= GENLIB_GENLIB_GENLIB_GENLIB_yystacksize)
        goto GENLIB_GENLIB_GENLIB_GENLIB_yyexhaustedlab;
      GENLIB_GENLIB_GENLIB_GENLIB_yystacksize *= 2;
      if (YYMAXDEPTH < GENLIB_GENLIB_GENLIB_GENLIB_yystacksize)
        GENLIB_GENLIB_GENLIB_GENLIB_yystacksize = YYMAXDEPTH;

      {
        GENLIB_GENLIB_GENLIB_GENLIB_yytype_int16 *GENLIB_GENLIB_GENLIB_GENLIB_yyss1 = GENLIB_GENLIB_GENLIB_GENLIB_yyss;
        union GENLIB_GENLIB_GENLIB_GENLIB_yyalloc *GENLIB_GENLIB_GENLIB_GENLIB_yyptr =
          (union GENLIB_GENLIB_GENLIB_GENLIB_yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (GENLIB_GENLIB_GENLIB_GENLIB_yystacksize));
        if (! GENLIB_GENLIB_GENLIB_GENLIB_yyptr)
          goto GENLIB_GENLIB_GENLIB_GENLIB_yyexhaustedlab;
        YYSTACK_RELOCATE (GENLIB_GENLIB_GENLIB_GENLIB_yyss_alloc, GENLIB_GENLIB_GENLIB_GENLIB_yyss);
        YYSTACK_RELOCATE (GENLIB_GENLIB_GENLIB_GENLIB_yyvs_alloc, GENLIB_GENLIB_GENLIB_GENLIB_yyvs);
#  undef YYSTACK_RELOCATE
        if (GENLIB_GENLIB_GENLIB_GENLIB_yyss1 != GENLIB_GENLIB_GENLIB_GENLIB_yyssa)
          YYSTACK_FREE (GENLIB_GENLIB_GENLIB_GENLIB_yyss1);
      }
# endif
#endif /* no GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow */

      GENLIB_GENLIB_GENLIB_GENLIB_yyssp = GENLIB_GENLIB_GENLIB_GENLIB_yyss + GENLIB_GENLIB_GENLIB_GENLIB_yysize - 1;
      GENLIB_GENLIB_GENLIB_GENLIB_yyvsp = GENLIB_GENLIB_GENLIB_GENLIB_yyvs + GENLIB_GENLIB_GENLIB_GENLIB_yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) GENLIB_GENLIB_GENLIB_GENLIB_yystacksize));

      if (GENLIB_GENLIB_GENLIB_GENLIB_yyss + GENLIB_GENLIB_GENLIB_GENLIB_yystacksize - 1 <= GENLIB_GENLIB_GENLIB_GENLIB_yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", GENLIB_GENLIB_GENLIB_GENLIB_yystate));

  if (GENLIB_GENLIB_GENLIB_GENLIB_yystate == YYFINAL)
    YYACCEPT;

  goto GENLIB_GENLIB_GENLIB_GENLIB_yybackup;

/*-----------.
| GENLIB_GENLIB_GENLIB_GENLIB_yybackup.  |
`-----------*/
GENLIB_GENLIB_GENLIB_GENLIB_yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  GENLIB_GENLIB_GENLIB_GENLIB_yyn = GENLIB_GENLIB_GENLIB_GENLIB_yypact[GENLIB_GENLIB_GENLIB_GENLIB_yystate];
  if (GENLIB_GENLIB_GENLIB_GENLIB_yypact_value_is_default (GENLIB_GENLIB_GENLIB_GENLIB_yyn))
    goto GENLIB_GENLIB_GENLIB_GENLIB_yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (GENLIB_GENLIB_GENLIB_GENLIB_yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      GENLIB_GENLIB_GENLIB_GENLIB_yychar = GENLIB_GENLIB_GENLIB_GENLIB_yylex ();
    }

  if (GENLIB_GENLIB_GENLIB_GENLIB_yychar <= YYEOF)
    {
      GENLIB_GENLIB_GENLIB_GENLIB_yychar = GENLIB_GENLIB_GENLIB_GENLIB_yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      GENLIB_GENLIB_GENLIB_GENLIB_yytoken = YYTRANSLATE (GENLIB_GENLIB_GENLIB_GENLIB_yychar);
      YY_SYMBOL_PRINT ("Next token is", GENLIB_GENLIB_GENLIB_GENLIB_yytoken, &GENLIB_GENLIB_GENLIB_GENLIB_yylval, &GENLIB_GENLIB_GENLIB_GENLIB_yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  GENLIB_GENLIB_GENLIB_GENLIB_yyn += GENLIB_GENLIB_GENLIB_GENLIB_yytoken;
  if (GENLIB_GENLIB_GENLIB_GENLIB_yyn < 0 || YYLAST < GENLIB_GENLIB_GENLIB_GENLIB_yyn || GENLIB_GENLIB_GENLIB_GENLIB_yycheck[GENLIB_GENLIB_GENLIB_GENLIB_yyn] != GENLIB_GENLIB_GENLIB_GENLIB_yytoken)
    goto GENLIB_GENLIB_GENLIB_GENLIB_yydefault;
  GENLIB_GENLIB_GENLIB_GENLIB_yyn = GENLIB_GENLIB_GENLIB_GENLIB_yytable[GENLIB_GENLIB_GENLIB_GENLIB_yyn];
  if (GENLIB_GENLIB_GENLIB_GENLIB_yyn <= 0)
    {
      if (GENLIB_GENLIB_GENLIB_GENLIB_yytable_value_is_error (GENLIB_GENLIB_GENLIB_GENLIB_yyn))
        goto GENLIB_GENLIB_GENLIB_GENLIB_yyerrlab;
      GENLIB_GENLIB_GENLIB_GENLIB_yyn = -GENLIB_GENLIB_GENLIB_GENLIB_yyn;
      goto GENLIB_GENLIB_GENLIB_GENLIB_yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (GENLIB_GENLIB_GENLIB_GENLIB_yyerrstatus)
    GENLIB_GENLIB_GENLIB_GENLIB_yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", GENLIB_GENLIB_GENLIB_GENLIB_yytoken, &GENLIB_GENLIB_GENLIB_GENLIB_yylval, &GENLIB_GENLIB_GENLIB_GENLIB_yylloc);

  /* Discard the shifted token.  */
  GENLIB_GENLIB_GENLIB_GENLIB_yychar = YYEMPTY;

  GENLIB_GENLIB_GENLIB_GENLIB_yystate = GENLIB_GENLIB_GENLIB_GENLIB_yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++GENLIB_GENLIB_GENLIB_GENLIB_yyvsp = GENLIB_GENLIB_GENLIB_GENLIB_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto GENLIB_GENLIB_GENLIB_GENLIB_yynewstate;


/*-----------------------------------------------------------.
| GENLIB_GENLIB_GENLIB_GENLIB_yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
GENLIB_GENLIB_GENLIB_GENLIB_yydefault:
  GENLIB_GENLIB_GENLIB_GENLIB_yyn = GENLIB_GENLIB_GENLIB_GENLIB_yydefact[GENLIB_GENLIB_GENLIB_GENLIB_yystate];
  if (GENLIB_GENLIB_GENLIB_GENLIB_yyn == 0)
    goto GENLIB_GENLIB_GENLIB_GENLIB_yyerrlab;
  goto GENLIB_GENLIB_GENLIB_GENLIB_yyreduce;


/*-----------------------------.
| GENLIB_GENLIB_GENLIB_GENLIB_yyreduce -- Do a reduction.  |
`-----------------------------*/
GENLIB_GENLIB_GENLIB_GENLIB_yyreduce:
  /* GENLIB_GENLIB_GENLIB_GENLIB_yyn is the number of a rule to reduce with.  */
  GENLIB_GENLIB_GENLIB_GENLIB_yylen = GENLIB_GENLIB_GENLIB_GENLIB_yyr2[GENLIB_GENLIB_GENLIB_GENLIB_yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  GENLIB_GENLIB_GENLIB_GENLIB_yyval = GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[1-GENLIB_GENLIB_GENLIB_GENLIB_yylen];


  YY_REDUCE_PRINT (GENLIB_GENLIB_GENLIB_GENLIB_yyn);
  switch (GENLIB_GENLIB_GENLIB_GENLIB_yyn)
    {
        case 3:
#line 54 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {global_fct = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].functionval); }
#line 1299 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 6:
#line 62 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { 
			if (! gl_convert_gate_to_blif((GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-3].strval),(GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-2].realval),(GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].functionval),(GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].pinval),global_use_nor))
			    GENLIB_GENLIB_GENLIB_GENLIB_yyerror(NIL(char));
		    }
#line 1308 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 7:
#line 67 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
		      if (! gl_convert_latch_to_blif((GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-6].strval),(GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-5].realval),(GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-4].functionval),(GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-3].pinval),global_use_nor,(GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-2].latchval),(GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].pinval),(GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].constrval)))
			GENLIB_GENLIB_GENLIB_GENLIB_yyerror(NIL(char));
		    }
#line 1317 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 8:
#line 74 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval) = 0; }
#line 1323 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 9:
#line 76 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].pinval)->next = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].pinval); (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval) = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].pinval);}
#line 1329 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 10:
#line 80 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval) = ALLOC(pin_info_t, 1);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->name = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-7].strval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->phase = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-6].strval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[0] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-5].realval); (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[1] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-4].realval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[2] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-3].realval); (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[3] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-2].realval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[4] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].realval); (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[5] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].realval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->next = 0;
		    }
#line 1343 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 11:
#line 92 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
			if (strcmp((GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].strval), "INV") != 0) {
			    if (strcmp((GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].strval), "NONINV") != 0) {
				if (strcmp((GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].strval), "UNKNOWN") != 0) {
				    GENLIB_GENLIB_GENLIB_GENLIB_yyerror("bad pin phase");
				}
			    }
			}
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.strval) = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].strval);
		    }
#line 1358 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 12:
#line 105 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { (GENLIB_GENLIB_GENLIB_GENLIB_yyval.strval) = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].strval); }
#line 1364 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 13:
#line 107 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { (GENLIB_GENLIB_GENLIB_GENLIB_yyval.strval) = util_strsav(GENLIB_GENLIB_GENLIB_GENLIB_yytext); }
#line 1370 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 14:
#line 111 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.functionval) = ALLOC(function_t, 1);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.functionval)->name = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-3].strval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.functionval)->tree = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].nodeval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.functionval)->next = 0;
		    }
#line 1381 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 15:
#line 120 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { 
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval) = gl_alloc_node(2);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->phase = 1;
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->sons[0] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-2].nodeval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->sons[1] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].nodeval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->type = OR_NODE;
		    }
#line 1393 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 16:
#line 128 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval) = gl_alloc_node(2);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->phase = 1;
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->sons[0] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-2].nodeval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->sons[1] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].nodeval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->type = AND_NODE;
		    }
#line 1405 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 17:
#line 136 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval) = gl_alloc_node(2);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->phase = 1;
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->sons[0] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].nodeval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->sons[1] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].nodeval);
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->type = AND_NODE;
		    }
#line 1417 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 18:
#line 144 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { 
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval) = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].nodeval); 
		    }
#line 1425 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 19:
#line 148 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { 
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval) = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].nodeval); 
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->phase = 0; 
		    }
#line 1434 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 20:
#line 153 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { 
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval) = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].nodeval); 
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->phase = 0; 
		    }
#line 1443 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 21:
#line 158 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval) = gl_alloc_leaf((GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].strval));
			FREE((GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].strval));
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->type = LEAF_NODE;	/* hack */
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->phase = 1;
		    }
#line 1454 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 22:
#line 165 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval) = gl_alloc_leaf("0");
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->phase = 1;
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->type = ZERO_NODE;
		    }
#line 1464 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 23:
#line 171 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval) = gl_alloc_leaf("1");
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->phase = 1;
			(GENLIB_GENLIB_GENLIB_GENLIB_yyval.nodeval)->type = ONE_NODE;
		    }
#line 1474 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 24:
#line 179 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { (GENLIB_GENLIB_GENLIB_GENLIB_yyval.strval) = util_strsav(GENLIB_GENLIB_GENLIB_GENLIB_yytext); }
#line 1480 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 25:
#line 182 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { (GENLIB_GENLIB_GENLIB_GENLIB_yyval.strval) = util_strsav(GENLIB_GENLIB_GENLIB_GENLIB_yytext); }
#line 1486 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 26:
#line 186 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { (GENLIB_GENLIB_GENLIB_GENLIB_yyval.realval) = atof(GENLIB_GENLIB_GENLIB_GENLIB_yytext); }
#line 1492 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 27:
#line 190 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval) = 0;}
#line 1498 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 28:
#line 192 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval) = ALLOC(pin_info_t, 1);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->name = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-6].strval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->phase = util_strsav("UNKNOWN");
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[0] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-5].realval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[1] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-4].realval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[2] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-3].realval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[3] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-2].realval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[4] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].realval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->value[5] = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].realval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.pinval)->next = 0;
		}
#line 1514 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 29:
#line 206 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { (GENLIB_GENLIB_GENLIB_GENLIB_yyval.constrval) = 0; }
#line 1520 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 30:
#line 208 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].constrval)->next = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].constrval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.constrval) = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].constrval);
		}
#line 1529 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 31:
#line 215 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    {
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.constrval) = ALLOC(constraint_info_t, 1);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.constrval)->name = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-2].strval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.constrval)->setup = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].realval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.constrval)->hold = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].realval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.constrval)->next = 0;
		}
#line 1541 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 32:
#line 225 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { 
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.latchval) = ALLOC(latch_info_t, 1);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.latchval)->in = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-2].strval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.latchval)->out = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[-1].strval);
		  (GENLIB_GENLIB_GENLIB_GENLIB_yyval.latchval)->type = (GENLIB_GENLIB_GENLIB_GENLIB_yyvsp[0].strval);
		}
#line 1552 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;

  case 33:
#line 234 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1646  */
    { 
		  if (strcmp(GENLIB_GENLIB_GENLIB_GENLIB_yytext, "RISING_EDGE") == 0) {
		    (GENLIB_GENLIB_GENLIB_GENLIB_yyval.strval) = util_strsav("re");
		  } else if (strcmp(GENLIB_GENLIB_GENLIB_GENLIB_yytext, "FALLING_EDGE") == 0) {
		    (GENLIB_GENLIB_GENLIB_GENLIB_yyval.strval) = util_strsav("fe");
		  } else if (strcmp(GENLIB_GENLIB_GENLIB_GENLIB_yytext, "ACTIVE_HIGH") == 0) {
		    (GENLIB_GENLIB_GENLIB_GENLIB_yyval.strval) = util_strsav("ah");
		  } else if (strcmp(GENLIB_GENLIB_GENLIB_GENLIB_yytext, "ACTIVE_LOW") == 0) {
		    (GENLIB_GENLIB_GENLIB_GENLIB_yyval.strval) = util_strsav("al");
		  } else if (strcmp(GENLIB_GENLIB_GENLIB_GENLIB_yytext, "ASYNCH") == 0) {
		    (GENLIB_GENLIB_GENLIB_GENLIB_yyval.strval) = util_strsav("as");
		  } else {
		    GENLIB_GENLIB_GENLIB_GENLIB_yyerror("latch type can only be re/fe/ah/al/as");
		  }
		}
#line 1572 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
    break;


#line 1576 "/home/matteo/Dropbox/sis/sis/genlib/readlib.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter GENLIB_GENLIB_GENLIB_GENLIB_yychar, and that requires
     that GENLIB_GENLIB_GENLIB_GENLIB_yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of GENLIB_GENLIB_GENLIB_GENLIB_yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering GENLIB_GENLIB_GENLIB_GENLIB_yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", GENLIB_GENLIB_GENLIB_GENLIB_yyr1[GENLIB_GENLIB_GENLIB_GENLIB_yyn], &GENLIB_GENLIB_GENLIB_GENLIB_yyval, &GENLIB_GENLIB_GENLIB_GENLIB_yyloc);

  YYPOPSTACK (GENLIB_GENLIB_GENLIB_GENLIB_yylen);
  GENLIB_GENLIB_GENLIB_GENLIB_yylen = 0;
  YY_STACK_PRINT (GENLIB_GENLIB_GENLIB_GENLIB_yyss, GENLIB_GENLIB_GENLIB_GENLIB_yyssp);

  *++GENLIB_GENLIB_GENLIB_GENLIB_yyvsp = GENLIB_GENLIB_GENLIB_GENLIB_yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  GENLIB_GENLIB_GENLIB_GENLIB_yyn = GENLIB_GENLIB_GENLIB_GENLIB_yyr1[GENLIB_GENLIB_GENLIB_GENLIB_yyn];

  GENLIB_GENLIB_GENLIB_GENLIB_yystate = GENLIB_GENLIB_GENLIB_GENLIB_yypgoto[GENLIB_GENLIB_GENLIB_GENLIB_yyn - YYNTOKENS] + *GENLIB_GENLIB_GENLIB_GENLIB_yyssp;
  if (0 <= GENLIB_GENLIB_GENLIB_GENLIB_yystate && GENLIB_GENLIB_GENLIB_GENLIB_yystate <= YYLAST && GENLIB_GENLIB_GENLIB_GENLIB_yycheck[GENLIB_GENLIB_GENLIB_GENLIB_yystate] == *GENLIB_GENLIB_GENLIB_GENLIB_yyssp)
    GENLIB_GENLIB_GENLIB_GENLIB_yystate = GENLIB_GENLIB_GENLIB_GENLIB_yytable[GENLIB_GENLIB_GENLIB_GENLIB_yystate];
  else
    GENLIB_GENLIB_GENLIB_GENLIB_yystate = GENLIB_GENLIB_GENLIB_GENLIB_yydefgoto[GENLIB_GENLIB_GENLIB_GENLIB_yyn - YYNTOKENS];

  goto GENLIB_GENLIB_GENLIB_GENLIB_yynewstate;


/*--------------------------------------.
| GENLIB_GENLIB_GENLIB_GENLIB_yyerrlab -- here on detecting error.  |
`--------------------------------------*/
GENLIB_GENLIB_GENLIB_GENLIB_yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  GENLIB_GENLIB_GENLIB_GENLIB_yytoken = GENLIB_GENLIB_GENLIB_GENLIB_yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (GENLIB_GENLIB_GENLIB_GENLIB_yychar);

  /* If not already recovering from an error, report this error.  */
  if (!GENLIB_GENLIB_GENLIB_GENLIB_yyerrstatus)
    {
      ++GENLIB_GENLIB_GENLIB_GENLIB_yynerrs;
#if ! YYERROR_VERBOSE
      GENLIB_GENLIB_GENLIB_GENLIB_yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR GENLIB_GENLIB_GENLIB_GENLIB_yysyntax_error (&GENLIB_GENLIB_GENLIB_GENLIB_yymsg_alloc, &GENLIB_GENLIB_GENLIB_GENLIB_yymsg, \
                                        GENLIB_GENLIB_GENLIB_GENLIB_yyssp, GENLIB_GENLIB_GENLIB_GENLIB_yytoken)
      {
        char const *GENLIB_GENLIB_GENLIB_GENLIB_yymsgp = YY_("syntax error");
        int GENLIB_GENLIB_GENLIB_GENLIB_yysyntax_error_status;
        GENLIB_GENLIB_GENLIB_GENLIB_yysyntax_error_status = YYSYNTAX_ERROR;
        if (GENLIB_GENLIB_GENLIB_GENLIB_yysyntax_error_status == 0)
          GENLIB_GENLIB_GENLIB_GENLIB_yymsgp = GENLIB_GENLIB_GENLIB_GENLIB_yymsg;
        else if (GENLIB_GENLIB_GENLIB_GENLIB_yysyntax_error_status == 1)
          {
            if (GENLIB_GENLIB_GENLIB_GENLIB_yymsg != GENLIB_GENLIB_GENLIB_GENLIB_yymsgbuf)
              YYSTACK_FREE (GENLIB_GENLIB_GENLIB_GENLIB_yymsg);
            GENLIB_GENLIB_GENLIB_GENLIB_yymsg = (char *) YYSTACK_ALLOC (GENLIB_GENLIB_GENLIB_GENLIB_yymsg_alloc);
            if (!GENLIB_GENLIB_GENLIB_GENLIB_yymsg)
              {
                GENLIB_GENLIB_GENLIB_GENLIB_yymsg = GENLIB_GENLIB_GENLIB_GENLIB_yymsgbuf;
                GENLIB_GENLIB_GENLIB_GENLIB_yymsg_alloc = sizeof GENLIB_GENLIB_GENLIB_GENLIB_yymsgbuf;
                GENLIB_GENLIB_GENLIB_GENLIB_yysyntax_error_status = 2;
              }
            else
              {
                GENLIB_GENLIB_GENLIB_GENLIB_yysyntax_error_status = YYSYNTAX_ERROR;
                GENLIB_GENLIB_GENLIB_GENLIB_yymsgp = GENLIB_GENLIB_GENLIB_GENLIB_yymsg;
              }
          }
        GENLIB_GENLIB_GENLIB_GENLIB_yyerror (GENLIB_GENLIB_GENLIB_GENLIB_yymsgp);
        if (GENLIB_GENLIB_GENLIB_GENLIB_yysyntax_error_status == 2)
          goto GENLIB_GENLIB_GENLIB_GENLIB_yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (GENLIB_GENLIB_GENLIB_GENLIB_yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (GENLIB_GENLIB_GENLIB_GENLIB_yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (GENLIB_GENLIB_GENLIB_GENLIB_yychar == YYEOF)
            YYABORT;
        }
      else
        {
          GENLIB_GENLIB_GENLIB_GENLIB_yydestruct ("Error: discarding",
                      GENLIB_GENLIB_GENLIB_GENLIB_yytoken, &GENLIB_GENLIB_GENLIB_GENLIB_yylval);
          GENLIB_GENLIB_GENLIB_GENLIB_yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto GENLIB_GENLIB_GENLIB_GENLIB_yyerrlab1;


/*---------------------------------------------------.
| GENLIB_GENLIB_GENLIB_GENLIB_yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
GENLIB_GENLIB_GENLIB_GENLIB_yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label GENLIB_GENLIB_GENLIB_GENLIB_yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto GENLIB_GENLIB_GENLIB_GENLIB_yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (GENLIB_GENLIB_GENLIB_GENLIB_yylen);
  GENLIB_GENLIB_GENLIB_GENLIB_yylen = 0;
  YY_STACK_PRINT (GENLIB_GENLIB_GENLIB_GENLIB_yyss, GENLIB_GENLIB_GENLIB_GENLIB_yyssp);
  GENLIB_GENLIB_GENLIB_GENLIB_yystate = *GENLIB_GENLIB_GENLIB_GENLIB_yyssp;
  goto GENLIB_GENLIB_GENLIB_GENLIB_yyerrlab1;


/*-------------------------------------------------------------.
| GENLIB_GENLIB_GENLIB_GENLIB_yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
GENLIB_GENLIB_GENLIB_GENLIB_yyerrlab1:
  GENLIB_GENLIB_GENLIB_GENLIB_yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      GENLIB_GENLIB_GENLIB_GENLIB_yyn = GENLIB_GENLIB_GENLIB_GENLIB_yypact[GENLIB_GENLIB_GENLIB_GENLIB_yystate];
      if (!GENLIB_GENLIB_GENLIB_GENLIB_yypact_value_is_default (GENLIB_GENLIB_GENLIB_GENLIB_yyn))
        {
          GENLIB_GENLIB_GENLIB_GENLIB_yyn += YYTERROR;
          if (0 <= GENLIB_GENLIB_GENLIB_GENLIB_yyn && GENLIB_GENLIB_GENLIB_GENLIB_yyn <= YYLAST && GENLIB_GENLIB_GENLIB_GENLIB_yycheck[GENLIB_GENLIB_GENLIB_GENLIB_yyn] == YYTERROR)
            {
              GENLIB_GENLIB_GENLIB_GENLIB_yyn = GENLIB_GENLIB_GENLIB_GENLIB_yytable[GENLIB_GENLIB_GENLIB_GENLIB_yyn];
              if (0 < GENLIB_GENLIB_GENLIB_GENLIB_yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (GENLIB_GENLIB_GENLIB_GENLIB_yyssp == GENLIB_GENLIB_GENLIB_GENLIB_yyss)
        YYABORT;


      GENLIB_GENLIB_GENLIB_GENLIB_yydestruct ("Error: popping",
                  GENLIB_GENLIB_GENLIB_GENLIB_yystos[GENLIB_GENLIB_GENLIB_GENLIB_yystate], GENLIB_GENLIB_GENLIB_GENLIB_yyvsp);
      YYPOPSTACK (1);
      GENLIB_GENLIB_GENLIB_GENLIB_yystate = *GENLIB_GENLIB_GENLIB_GENLIB_yyssp;
      YY_STACK_PRINT (GENLIB_GENLIB_GENLIB_GENLIB_yyss, GENLIB_GENLIB_GENLIB_GENLIB_yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++GENLIB_GENLIB_GENLIB_GENLIB_yyvsp = GENLIB_GENLIB_GENLIB_GENLIB_yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", GENLIB_GENLIB_GENLIB_GENLIB_yystos[GENLIB_GENLIB_GENLIB_GENLIB_yyn], GENLIB_GENLIB_GENLIB_GENLIB_yyvsp, GENLIB_GENLIB_GENLIB_GENLIB_yylsp);

  GENLIB_GENLIB_GENLIB_GENLIB_yystate = GENLIB_GENLIB_GENLIB_GENLIB_yyn;
  goto GENLIB_GENLIB_GENLIB_GENLIB_yynewstate;


/*-------------------------------------.
| GENLIB_GENLIB_GENLIB_GENLIB_yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
GENLIB_GENLIB_GENLIB_GENLIB_yyacceptlab:
  GENLIB_GENLIB_GENLIB_GENLIB_yyresult = 0;
  goto GENLIB_GENLIB_GENLIB_GENLIB_yyreturn;

/*-----------------------------------.
| GENLIB_GENLIB_GENLIB_GENLIB_yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
GENLIB_GENLIB_GENLIB_GENLIB_yyabortlab:
  GENLIB_GENLIB_GENLIB_GENLIB_yyresult = 1;
  goto GENLIB_GENLIB_GENLIB_GENLIB_yyreturn;

#if !defined GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| GENLIB_GENLIB_GENLIB_GENLIB_yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
GENLIB_GENLIB_GENLIB_GENLIB_yyexhaustedlab:
  GENLIB_GENLIB_GENLIB_GENLIB_yyerror (YY_("memory exhausted"));
  GENLIB_GENLIB_GENLIB_GENLIB_yyresult = 2;
  /* Fall through.  */
#endif

GENLIB_GENLIB_GENLIB_GENLIB_yyreturn:
  if (GENLIB_GENLIB_GENLIB_GENLIB_yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      GENLIB_GENLIB_GENLIB_GENLIB_yytoken = YYTRANSLATE (GENLIB_GENLIB_GENLIB_GENLIB_yychar);
      GENLIB_GENLIB_GENLIB_GENLIB_yydestruct ("Cleanup: discarding lookahead",
                  GENLIB_GENLIB_GENLIB_GENLIB_yytoken, &GENLIB_GENLIB_GENLIB_GENLIB_yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (GENLIB_GENLIB_GENLIB_GENLIB_yylen);
  YY_STACK_PRINT (GENLIB_GENLIB_GENLIB_GENLIB_yyss, GENLIB_GENLIB_GENLIB_GENLIB_yyssp);
  while (GENLIB_GENLIB_GENLIB_GENLIB_yyssp != GENLIB_GENLIB_GENLIB_GENLIB_yyss)
    {
      GENLIB_GENLIB_GENLIB_GENLIB_yydestruct ("Cleanup: popping",
                  GENLIB_GENLIB_GENLIB_GENLIB_yystos[*GENLIB_GENLIB_GENLIB_GENLIB_yyssp], GENLIB_GENLIB_GENLIB_GENLIB_yyvsp);
      YYPOPSTACK (1);
    }
#ifndef GENLIB_GENLIB_GENLIB_GENLIB_yyoverflow
  if (GENLIB_GENLIB_GENLIB_GENLIB_yyss != GENLIB_GENLIB_GENLIB_GENLIB_yyssa)
    YYSTACK_FREE (GENLIB_GENLIB_GENLIB_GENLIB_yyss);
#endif
#if YYERROR_VERBOSE
  if (GENLIB_GENLIB_GENLIB_GENLIB_yymsg != GENLIB_GENLIB_GENLIB_GENLIB_yymsgbuf)
    YYSTACK_FREE (GENLIB_GENLIB_GENLIB_GENLIB_yymsg);
#endif
  return GENLIB_GENLIB_GENLIB_GENLIB_yyresult;
}
#line 251 "/home/matteo/Dropbox/sis/sis/genlib/readlib.y" /* yacc.c:1906  */

static jmp_buf jmpbuf;

int
GENLIB_GENLIB_GENLIB_GENLIB_yyerror(errmsg)
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
	(void) GENLIB_GENLIB_GENLIB_GENLIB_yyparse();
	if (global_fct != 0) GENLIB_GENLIB_GENLIB_GENLIB_yyerror("syntax error");
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
	(void) GENLIB_GENLIB_GENLIB_GENLIB_yyparse();
	if (global_fct != 0) GENLIB_GENLIB_GENLIB_GENLIB_yyerror("syntax error");
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
	(void) GENLIB_GENLIB_GENLIB_GENLIB_yyparse();
	if (global_fct == 0) GENLIB_GENLIB_GENLIB_GENLIB_yyerror("syntax error");
	*tree = global_fct->tree;
	*name = global_fct->name;
	return 1;
    }
}
